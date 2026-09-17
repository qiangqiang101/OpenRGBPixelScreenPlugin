/*---------------------------------------------------------*\
| DeviceUpdateVTableHook.h                                 |
|                                                           |
|   Process-local RGBController device-send vtable hook     |
|                                                           |
|   This file is part of the OpenRGB Pixel Screen Plugin    |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#include <fstream>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace pixelscreen
{

/*---------------------------------------------------------*\
| Hooks the three final RGBController device-output virtuals|
| without changing the controller object or OpenRGB itself. |
|                                                           |
| The original vtable is patched rather than replacing an   |
| object's vfptr.  This preserves RTTI and any private       |
| virtuals added by a concrete controller.  A registry keyed |
| by `this` decides which controller instances are filtered; |
| unregistered instances sharing the same vtable are simply |
| forwarded to the original implementation.                 |
\*---------------------------------------------------------*/
template<typename Controller,
         std::size_t DeviceUpdateLEDsSlot,
         std::size_t DeviceUpdateZoneLEDsSlot,
         std::size_t DeviceUpdateSingleLEDSlot>
class DeviceUpdateVTableHook final
{
private:
#if defined(_WIN32) && defined(_M_IX86)
    using NoArgumentFunction  = void (__thiscall *)(Controller*);
    using OneArgumentFunction = void (__thiscall *)(Controller*, int);
#else
    using NoArgumentFunction  = void (*)(Controller*);
    using OneArgumentFunction = void (*)(Controller*, int);
#endif

public:
    class OriginalCall
    {
    public:
        void Invoke(Controller* controller) const
        {
            if (!controller || !function_)
            {
                return;
            }

            if (method_index_ == 0)
            {
                NoArgumentFunction function;
                std::memcpy(&function, &function_, sizeof(function));
                function(controller);
            }
            else
            {
                OneArgumentFunction function;
                std::memcpy(&function, &function_, sizeof(function));
                function(controller, argument_);
            }
        }

    private:
        friend class DeviceUpdateVTableHook;

        OriginalCall(std::size_t method_index, void* function, int argument) noexcept
            : method_index_(method_index), function_(function), argument_(argument)
        {
        }

        std::size_t method_index_;
        void*       function_;
        int         argument_;
    };

    using AroundCallback = void (*)(void* callback_arg,
                                    Controller* controller,
                                    const OriginalCall& original_call);

    static bool Install(Controller* controller, void* callback_arg, AroundCallback callback)
    {
        if (!controller || !callback || !EnsureModulePinned())
        {
            return false;
        }

        void*** object_vfptr = reinterpret_cast<void***>(controller);
        if (!IsReadableAddress(object_vfptr))
        {
            return false;
        }

        void** vtable = *object_vfptr;
        if (!vtable)
        {
            return false;
        }

        auto context          = std::make_shared<HookContext>();
        context->callback_arg = callback_arg;
        context->callback     = callback;
        context->vtable       = vtable;

        std::lock_guard<std::mutex> lock(registry_mutex_);

        auto existing_context = controller_hooks_.find(controller);
        if (existing_context != controller_hooks_.end())
        {
            return existing_context->second->callback_arg == callback_arg
                && existing_context->second->callback == callback;
        }

        VTablePatch& patch = vtable_patches_[vtable];
        patch.vtable = vtable;

        if (patch.reference_count == 0)
        {
            std::array<bool, 3> changed = {{false, false, false}};
            std::array<void*, 3> current_functions = {{nullptr, nullptr, nullptr}};

            /* Validate the complete slot set before making the first write.
             * This rejects API5 objects that only implement the public
             * interface and do not carry RGBController's private tail. */
            for (std::size_t method_index = 0; method_index < method_slots_.size(); ++method_index)
            {
                void** slot_address = vtable + method_slots_[method_index];
                if (!IsReadableAddress(slot_address))
                {
                    return false;
                }

                void* const hook_address = HookAddress(method_index);
                void* const current      = *slot_address;
                current_functions[method_index] = current;

                /* A delayed call or another well-behaved hook may have
                 * restored our entry after a previous uninstall.  In that
                 * case retain the original function saved in the registry. */
                if (current == hook_address)
                {
                    if (!patch.originals[method_index])
                    {
                        return false;
                    }
                    continue;
                }

                if (!IsFunctionAddress(current))
                {
                    return false;
                }
            }

            for (std::size_t method_index = 0; method_index < method_slots_.size(); ++method_index)
            {
                void* const hook_address = HookAddress(method_index);
                void* const current      = current_functions[method_index];
                if (current == hook_address)
                {
                    continue;
                }

                patch.originals[method_index] = current;
                if (!ReplacePointer(vtable + method_slots_[method_index], current, hook_address))
                {
                    RollBackPatch(patch, changed);
                    return false;
                }

                changed[method_index] = true;
            }
        }
        else
        {
            /* All registered objects sharing a vtable must see the same
             * chain.  Refuse a partial installation if another component
             * replaced one of our live entries without chaining it. */
            for (std::size_t method_index = 0; method_index < method_slots_.size(); ++method_index)
            {
                void** slot_address = vtable + method_slots_[method_index];
                if (!IsReadableAddress(slot_address) || *slot_address != HookAddress(method_index))
                {
                    return false;
                }
            }
        }

        ++patch.reference_count;
        controller_hooks_.emplace(controller, std::move(context));
        return true;
    }

    /* Disables callbacks first, restores all no-longer-used vtables, then
     * waits only for callbacks which could still reference callback_arg.
     * The module is intentionally pinned until process exit so a device
     * thread that fetched a hook pointer immediately before restoration can
     * still enter the forwarding thunk safely. */
    static void Uninstall(void* callback_arg)
    {
        UninstallMatching(callback_arg, nullptr);
    }

    static void UninstallController(Controller* controller, void* callback_arg)
    {
        if (controller)
        {
            UninstallMatching(callback_arg, controller);
        }
    }

private:
    static void UninstallMatching(void* callback_arg, Controller* controller_filter)
    {
        std::vector<std::shared_ptr<HookContext>> removed_contexts;
        std::unordered_map<void**, std::size_t> reference_decrements;

        {
            std::lock_guard<std::mutex> lock(registry_mutex_);

            for (auto iterator = controller_hooks_.begin(); iterator != controller_hooks_.end();)
            {
                const std::shared_ptr<HookContext>& context = iterator->second;
                if (context->callback_arg != callback_arg
                    || (controller_filter && iterator->first != controller_filter))
                {
                    ++iterator;
                    continue;
                }

                context->enabled.store(false, std::memory_order_release);
                ++reference_decrements[context->vtable];
                removed_contexts.push_back(context);
                iterator = controller_hooks_.erase(iterator);
            }

            for (const auto& decrement : reference_decrements)
            {
                auto patch_iterator = vtable_patches_.find(decrement.first);
                if (patch_iterator == vtable_patches_.end())
                {
                    continue;
                }

                VTablePatch& patch = patch_iterator->second;
                patch.reference_count = decrement.second >= patch.reference_count
                                      ? 0
                                      : patch.reference_count - decrement.second;

                if (patch.reference_count != 0)
                {
                    continue;
                }

                for (std::size_t method_index = 0; method_index < method_slots_.size(); ++method_index)
                {
                    void** slot_address = patch.vtable + method_slots_[method_index];
                    if (!patch.originals[method_index] || !IsReadableAddress(slot_address))
                    {
                        continue;
                    }

                    void* const hook_address = HookAddress(method_index);
                    if (*slot_address == hook_address)
                    {
                        ReplacePointer(slot_address, hook_address, patch.originals[method_index]);
                    }
                }
            }
        }

        for (const std::shared_ptr<HookContext>& context : removed_contexts)
        {
            std::unique_lock<std::mutex> lock(context->idle_mutex);
            context->idle_condition.wait(lock, [&context]
            {
                return context->active_calls.load(std::memory_order_acquire) == 0;
            });
        }
    }

    struct HookContext
    {
        void*                     callback_arg = nullptr;
        AroundCallback            callback = nullptr;
        void**                    vtable = nullptr;
        std::atomic<bool>         enabled{true};
        std::atomic<unsigned int> active_calls{0};
        std::mutex                send_mutex;
        std::mutex                idle_mutex;
        std::condition_variable   idle_condition;
    };

    struct VTablePatch
    {
        void**                vtable = nullptr;
        std::array<void*, 3>  originals = {{nullptr, nullptr, nullptr}};
        std::size_t           reference_count = 0;
    };

    class ActiveCallGuard
    {
    public:
        explicit ActiveCallGuard(std::shared_ptr<HookContext> context)
            : context_(std::move(context))
        {
        }

        ~ActiveCallGuard()
        {
            if (!context_)
            {
                return;
            }

            // Pair the predicate transition with the waiter's mutex; otherwise
            // the last notification can be lost between its check and wait.
            std::lock_guard<std::mutex> lock(context_->idle_mutex);
            if (context_->active_calls.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                context_->idle_condition.notify_all();
            }
        }

    private:
        std::shared_ptr<HookContext> context_;
    };

    class RecursionGuard
    {
    public:
        explicit RecursionGuard(Controller* controller) : controller_(controller)
        {
            active_controllers_.push_back(controller_);
        }

        ~RecursionGuard()
        {
            auto iterator = std::find(active_controllers_.rbegin(), active_controllers_.rend(), controller_);
            if (iterator != active_controllers_.rend())
            {
                active_controllers_.erase(std::next(iterator).base());
            }
        }

    private:
        Controller* controller_;
    };

    template<typename Function>
    static void* FunctionAddress(Function function) noexcept
    {
        static_assert(sizeof(Function) == sizeof(void*), "Unsupported function pointer representation");
        void* address = nullptr;
        std::memcpy(&address, &function, sizeof(address));
        return address;
    }

    static void* HookAddress(std::size_t method_index) noexcept
    {
        switch (method_index)
        {
        case 0:
            return FunctionAddress(&HookAllLEDs);
        case 1:
            return FunctionAddress(&HookZoneLEDs);
        default:
            return FunctionAddress(&HookSingleLED);
        }
    }

    static void RollBackPatch(VTablePatch& patch, const std::array<bool, 3>& changed)
    {
        for (std::size_t method_index = 0; method_index < method_slots_.size(); ++method_index)
        {
            if (!changed[method_index] || !patch.originals[method_index])
            {
                continue;
            }

            void** slot_address = patch.vtable + method_slots_[method_index];
            if (IsReadableAddress(slot_address) && *slot_address == HookAddress(method_index))
            {
                ReplacePointer(slot_address, HookAddress(method_index), patch.originals[method_index]);
            }
        }
    }

    static bool IsReentrant(Controller* controller)
    {
        return std::find(active_controllers_.begin(), active_controllers_.end(), controller)
            != active_controllers_.end();
    }

    static void Dispatch(Controller* controller, std::size_t method_index, int argument)
    {
        if (!controller)
        {
            return;
        }

        std::shared_ptr<HookContext> context;
        void* original = nullptr;

        {
            std::lock_guard<std::mutex> lock(registry_mutex_);
            void** vtable = *reinterpret_cast<void***>(controller);
            auto patch_iterator = vtable_patches_.find(vtable);
            if (patch_iterator != vtable_patches_.end())
            {
                original = patch_iterator->second.originals[method_index];
            }

            auto context_iterator = controller_hooks_.find(controller);
            if (context_iterator != controller_hooks_.end())
            {
                context = context_iterator->second;
                context->active_calls.fetch_add(1, std::memory_order_acq_rel);
            }
        }

        if (!context)
        {
            if (original)
            {
                OriginalCall(method_index, original, argument).Invoke(controller);
            }
            return;
        }

        ActiveCallGuard active_guard(context);
        if (!original)
        {
            return;
        }

        OriginalCall original_call(method_index, original, argument);

        if (!context->enabled.load(std::memory_order_acquire) || IsReentrant(controller))
        {
            original_call.Invoke(controller);
            return;
        }

        std::unique_lock<std::mutex> send_lock(context->send_mutex);
        if (!context->enabled.load(std::memory_order_acquire))
        {
            original_call.Invoke(controller);
            return;
        }

        RecursionGuard recursion_guard(controller);
        context->callback(context->callback_arg, controller, original_call);
    }

#if defined(_WIN32) && defined(_M_IX86)
    static void __fastcall HookAllLEDs(Controller* controller, void*)
    {
        Dispatch(controller, 0, -1);
    }

    static void __fastcall HookZoneLEDs(Controller* controller, void*, int zone)
    {
        Dispatch(controller, 1, zone);
    }

    static void __fastcall HookSingleLED(Controller* controller, void*, int led)
    {
        Dispatch(controller, 2, led);
    }
#else
    static void HookAllLEDs(Controller* controller)
    {
        Dispatch(controller, 0, -1);
    }

    static void HookZoneLEDs(Controller* controller, int zone)
    {
        Dispatch(controller, 1, zone);
    }

    static void HookSingleLED(Controller* controller, int led)
    {
        Dispatch(controller, 2, led);
    }
#endif

    static bool EnsureModulePinned()
    {
        std::call_once(pin_once_, []
        {
#if defined(_WIN32)
            HMODULE module = nullptr;
            void* const address = HookAddress(0);
            pin_succeeded_ = GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCWSTR>(address),
                &module) != FALSE;
            pinned_module_ = module;
#elif defined(__linux__)
            Dl_info module_info = {};
            void* const address = HookAddress(0);
            if (dladdr(address, &module_info) != 0 && module_info.dli_fname)
            {
#ifdef RTLD_NODELETE
                pinned_module_ = dlopen(module_info.dli_fname, RTLD_NOW | RTLD_NODELETE);
#else
                pinned_module_ = dlopen(module_info.dli_fname, RTLD_NOW);
#endif
            }
            pin_succeeded_ = pinned_module_ != nullptr;
#else
            pin_succeeded_ = false;
#endif
        });

        return pin_succeeded_;
    }

    static bool IsReadableAddress(const void* address) noexcept
    {
        if (!address)
        {
            return false;
        }

#if defined(_WIN32)
        MEMORY_BASIC_INFORMATION information = {};
        if (VirtualQuery(address, &information, sizeof(information)) != sizeof(information)
            || information.State != MEM_COMMIT
            || (information.Protect & PAGE_GUARD)
            || (information.Protect & PAGE_NOACCESS))
        {
            return false;
        }
        return true;
#elif defined(__linux__)
        return LinuxProtection(address) != 0;
#else
        return false;
#endif
    }

    static bool IsFunctionAddress(const void* address) noexcept
    {
        if (!address)
        {
            return false;
        }

#if defined(_WIN32)
        MEMORY_BASIC_INFORMATION information = {};
        if (VirtualQuery(address, &information, sizeof(information)) != sizeof(information)
            || information.State != MEM_COMMIT
            || (information.Protect & PAGE_GUARD))
        {
            return false;
        }

        const DWORD protection = information.Protect & 0xFF;
        return protection == PAGE_EXECUTE
            || protection == PAGE_EXECUTE_READ
            || protection == PAGE_EXECUTE_READWRITE
            || protection == PAGE_EXECUTE_WRITECOPY;
#elif defined(__linux__)
        Dl_info information = {};
        return dladdr(address, &information) != 0 && information.dli_fbase != nullptr;
#else
        return false;
#endif
    }

    static bool ReplacePointer(void** address, void* expected, void* replacement)
    {
#if defined(_WIN32)
        DWORD old_protection = 0;
        if (!VirtualProtect(address, sizeof(void*), PAGE_READWRITE, &old_protection))
        {
            return false;
        }

        void* const observed = InterlockedCompareExchangePointer(
            reinterpret_cast<PVOID volatile*>(address), replacement, expected);

        DWORD ignored = 0;
        VirtualProtect(address, sizeof(void*), old_protection, &ignored);
        FlushInstructionCache(GetCurrentProcess(), address, sizeof(void*));
        return observed == expected;
#elif defined(__linux__)
        const long page_size = sysconf(_SC_PAGESIZE);
        const int old_protection = LinuxProtection(address);
        if (page_size <= 0 || old_protection == 0)
        {
            return false;
        }

        const std::uintptr_t page_mask = static_cast<std::uintptr_t>(page_size - 1);
        void* const page = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(address) & ~page_mask);
        if (mprotect(page, static_cast<std::size_t>(page_size), old_protection | PROT_WRITE) != 0)
        {
            return false;
        }

        void* expected_copy = expected;
        const bool replaced = __atomic_compare_exchange_n(
            address,
            &expected_copy,
            replacement,
            false,
            __ATOMIC_SEQ_CST,
            __ATOMIC_SEQ_CST);

        mprotect(page, static_cast<std::size_t>(page_size), old_protection);
        return replaced;
#else
        (void)address;
        (void)expected;
        (void)replacement;
        return false;
#endif
    }

#if defined(__linux__)
    static int LinuxProtection(const void* address) noexcept
    {
        std::ifstream maps("/proc/self/maps");
        if (!maps.is_open())
        {
            return 0;
        }

        const std::uintptr_t target = reinterpret_cast<std::uintptr_t>(address);
        std::string line;
        while (std::getline(maps, line))
        {
            unsigned long long begin = 0;
            unsigned long long end = 0;
            char permissions[5] = {};
            if (std::sscanf(line.c_str(), "%llx-%llx %4s", &begin, &end, permissions) != 3)
            {
                continue;
            }

            if (target < begin || target >= end)
            {
                continue;
            }

            int protection = 0;
            if (permissions[0] == 'r') protection |= PROT_READ;
            if (permissions[1] == 'w') protection |= PROT_WRITE;
            if (permissions[2] == 'x') protection |= PROT_EXEC;
            return protection;
        }

        return 0;
    }
#endif

    inline static std::mutex registry_mutex_;
    inline static constexpr std::array<std::size_t, 3> method_slots_ = {{
        DeviceUpdateLEDsSlot,
        DeviceUpdateZoneLEDsSlot,
        DeviceUpdateSingleLEDSlot
    }};
    inline static std::unordered_map<Controller*, std::shared_ptr<HookContext>> controller_hooks_;
    inline static std::unordered_map<void**, VTablePatch> vtable_patches_;
    inline static thread_local std::vector<Controller*> active_controllers_;
    inline static std::once_flag pin_once_;
    inline static bool pin_succeeded_ = false;

#if defined(_WIN32)
    inline static HMODULE pinned_module_ = nullptr;
#elif defined(__linux__)
    inline static void* pinned_module_ = nullptr;
#endif
};

} // namespace pixelscreen

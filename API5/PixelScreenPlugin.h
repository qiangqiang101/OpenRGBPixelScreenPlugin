/*---------------------------------------------------------*\
| OpenRGBMatrixTextPlugin.h                                 |
|                                                           |
|   OpenRGB Matrix Text Plugin                              |
|                                                           |
|   This file is part of the OpenRGB Matrix Text Plugin     |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <QMenu>
#include <QObject>
#include <QWidget>
#include <QTimer>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <shared_mutex>
#include "OpenRGBPluginInterface.h"
#include "RGBControllerInterface.h"
#include "LogManager.h"
#include "../common/DeviceUpdateVTableHook.h"

// Font structure representing a character's LED matrix layout
struct Glyph
{
    std::vector<std::vector<unsigned char>> grid;
    unsigned int width = 0;
    unsigned int height = 0;
};

// Target matrix zone structure
struct MatrixZoneTarget
{
    RGBControllerInterface* controller;
    unsigned int zone_idx;
    std::string display_name;
};

// Per-device matrix settings structure
struct DeviceMatrixSettings
{
    bool enabled = false;
    int display_mode = 1;       // 0: Time, 1: Custom Text, 3: Pixel Art, 4: Sensor Data
    std::string font_size = "Medium"; // "Small", "Medium", "Large", "Chinese"
    std::string custom_text = "OpenRGB";
    std::string time_format = "hh:mm tt";
    std::string sensor_format = "CPU: [CPU\\Load\\CPU Total]";
    std::string lhm_url = "http://127.0.0.1:8085/data.json";
    int sensor_update_interval = 1000; // ms: 250, 500, 1000, 2000
    std::string pixel_art_json = "[ [1, 0, 0, 1], [0, 1, 1, 0], [0, 1, 1, 0], [1, 0, 0, 1] ]"; // 2D Pixel Art Matrix JSON
    std::string scroll_direction = "Left"; // "Off", "Left", "Right", "Ping-Pong"
    int scroll_speed = 50;      // 1 to 100
    int fps = 20;               // 1 to 60 FPS
    unsigned char text_r = 255;
    unsigned char text_g = 255;
    unsigned char text_b = 255;
    bool invert_color = false;
    int padding_x = 0;
    int padding_y = 0;
    int text_align = 0; // 0: Start, 1: Center, 2: End

    // Runtime state
    float scroll_offset = 0.0f;
    int ping_pong_direction = -1;
    std::chrono::steady_clock::time_point last_animation_update;
    double animation_accumulator = 0.0;
};

struct MatrixTextSettings
{
    std::map<std::string, DeviceMatrixSettings> device_settings;

    DeviceMatrixSettings& GetForDevice(const std::string& display_name)
    {
        return device_settings[display_name];
    }
};

class PixelScreenTab;
class HardwareSensorManager;

/* MSVC emits one deleting-destructor entry; the Itanium ABI used by
 * GCC/Clang emits complete and deleting destructor entries.  These slots
 * are verified against the API5 RGBController declaration bundled here. */
#if defined(_MSC_VER)
using PixelScreenDeviceUpdateHook = pixelscreen::DeviceUpdateVTableHook<RGBControllerInterface, 119, 120, 121>;
#else
using PixelScreenDeviceUpdateHook = pixelscreen::DeviceUpdateVTableHook<RGBControllerInterface, 120, 121, 122>;
#endif

class PixelScreenPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID FILE "PixelScreenPlugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    ~PixelScreenPlugin();

    /*-----------------------------------------------------*\
    | Plugin Information                                    |
    \*-----------------------------------------------------*/
    virtual OpenRGBPluginInfo   GetPluginInfo()                                                                 override;
    virtual unsigned int        GetPluginAPIVersion()                                                           override;

    /*-----------------------------------------------------*\
    | Plugin Functionality                                  |
    \*-----------------------------------------------------*/
    virtual void                Load(OpenRGBPluginAPIInterface* api_interface_ptr)                              override;
    virtual QWidget*            GetWidget()                                                                     override;
    virtual QMenu*              GetTrayMenu()                                                                   override;
    virtual void                OnProfileAboutToLoad()                                                          override;
    virtual void                OnProfileLoad(nlohmann::json profile_data)                                      override;
    virtual nlohmann::json      OnProfileSave()                                                                 override;
    virtual void                Unload()                                                                        override;
    virtual unsigned char*      OnSDKCommand(unsigned int pkt_id, unsigned char* data, unsigned int* data_size) override;

    /*-----------------------------------------------------*\
    | Update Signals                                        |
    \*-----------------------------------------------------*/
    void                        ProfileManagerUpdated(unsigned int update_reason)                               override;
    void                        ResourceManagerUpdated(unsigned int update_reason)                              override;
    void                        SettingsManagerUpdated(unsigned int update_reason)                              override;

    DeviceMatrixSettings        GetDeviceSettings(const std::string& display_name);
    std::vector<std::string>    GetMatrixZoneNames();
    HardwareSensorManager*      GetSensorManager() const noexcept { return sensor_manager; }
    void                        SetSensorUpdateInterval(int interval);

    template<typename Update>
    void UpdateDeviceSettings(const std::string& display_name, Update update)
    {
        {
            std::lock_guard<std::mutex> lock(settings_mutex);
            update(settings.GetForDevice(display_name));
        }
        SaveSettings();
    }

public slots:
    void                        UpdateControllers();

private slots:
    void                        OnSensorTimerTimeout();

public:
    /*-----------------------------------------------------*\
    | Plugin Global Variables                               |
    \*-----------------------------------------------------*/
    static OpenRGBPluginAPIInterface*   api;

private:
    /*-----------------------------------------------------*\
    | User interface widget                                 |
    \*-----------------------------------------------------*/
    PixelScreenTab*                     ui = nullptr;
    HardwareSensorManager*              sensor_manager = nullptr;
    QTimer*                             sensor_timer = nullptr;
    MatrixTextSettings                  settings;
    std::mutex                          settings_mutex;
    std::vector<MatrixZoneTarget>       matrix_zones;
    std::shared_mutex                   matrix_zones_mutex;
    std::vector<RGBControllerInterface*> hooked_controllers;
    
    // Font databases loaded from JSON
    std::map<std::string, Glyph>        small_letters;
    std::map<std::string, Glyph>        letters;
    std::map<std::string, Glyph>        large_letters;
    std::map<std::string, Glyph>        digits;
    std::map<std::string, Glyph>        small_digits;
    std::map<std::string, Glyph>        large_digits;
    std::map<std::string, Glyph>        zh_font;

    void                                LoadFonts();
    void                                LoadSettings();
    void                                SaveSettings();

    std::vector<std::string>            SplitUTF8(const std::string& str);
    int                                 GetSpacing(const std::string& ch, const std::string& font_size, bool time);
    Glyph                               GetGlyph(const std::string& ch, const std::string& font_size, bool time);
    std::string                         FormatDateTime(const std::string& format);
    void                                AdvanceAnimation(DeviceMatrixSettings& dev_s);
    void                                OverlayTextOnBuffer(const MatrixZoneTarget& target,
                                                            DeviceMatrixSettings& dev_s,
                                                            RGBColor* colors,
                                                            std::size_t color_count);
    static void                         OnDeviceUpdateHook(void* callback_arg,
                                                           RGBControllerInterface* controller,
                                                           const PixelScreenDeviceUpdateHook::OriginalCall& original_call);
    void                                SendOverlayFrame(RGBControllerInterface* controller,
                                                         const PixelScreenDeviceUpdateHook::OriginalCall& original_call);
};

/*---------------------------------------------------------*\
| LogManager logging macros                                 |
\*---------------------------------------------------------*/
#undef  LogAppend
#define LogAppend(level, ...)   PixelScreenPlugin::api->LogEntry(__FILE__, __LINE__, level, __VA_ARGS__)

#undef  LOG_FATAL
#undef  LOG_ERROR
#undef  LOG_WARNING
#undef  LOG_WARN
#undef  LOG_INFO
#define LOG_INFO(...)    LogAppend(LL_INFO, __VA_ARGS__)
#define LOG_WARN(...)    LogAppend(LL_WARNING, __VA_ARGS__)
#define LOG_WARNING(...) LogAppend(LL_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)   LogAppend(LL_ERROR, __VA_ARGS__)
#define LOG_FATAL(...)   LogAppend(LL_FATAL, __VA_ARGS__)

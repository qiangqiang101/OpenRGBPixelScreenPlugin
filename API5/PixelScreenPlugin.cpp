/*---------------------------------------------------------*\
| OpenRGBMatrixTextPlugin.cpp                               |
|                                                           |
|   OpenRGB Matrix Text Plugin                              |
|                                                           |
|   This file is part of the OpenRGB Matrix Text Plugin     |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
| \*---------------------------------------------------------*/

#include "PixelScreenPlugin.h"
#include "HardwareSensorManager.h"
#include "PixelScreenTab.h"
#include <QFile>
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QMetaObject>
#include <fstream>
#include <cmath>
#include <limits>

#include <algorithm>
OpenRGBPluginAPIInterface* PixelScreenPlugin::api = nullptr;

PixelScreenPlugin::~PixelScreenPlugin()
{
    PixelScreenDeviceUpdateHook::Uninstall(this);
}


OpenRGBPluginInfo PixelScreenPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info;

    info.Name           = "Pixel Screen Plugin";
    info.Description    = "Render custom scrolling text, clock, and pixel art on OpenRGB matrices";
    info.Version        = VERSION_STRING;
    info.Commit         = GIT_COMMIT_ID;
    info.URL            = "https://gitlab.com/OpenRGBDevelopers/OpenRGBPixelScreenPlugin";

    info.Label          = "Pixel Screen";
    info.Location       = OPENRGB_PLUGIN_LOCATION_TOP;

    if (!info.Icon.load(":/images/OpenRGBPixelScreenPlugin.png")) {
        LOG_WARNING("[PixelScreenPlugin] Icon not found – using empty pixmap");
    }

    return(info);
}

unsigned int PixelScreenPlugin::GetPluginAPIVersion()
{
    return(OPENRGB_PLUGIN_API_VERSION);
}

/*---------------------------------------------------------*\
| Plugin Functionality                                      |
\*---------------------------------------------------------*/
void PixelScreenPlugin::Load(OpenRGBPluginAPIInterface* api_interface_ptr)
{
    api = api_interface_ptr;

    LOG_INFO("[PixelScreenPlugin] Loading version %s (%s), build date %s\n", VERSION_STRING, GIT_COMMIT_ID, BUILDDATE_STRING);

    // Load fonts and configurations
    LoadFonts();
    LOG_INFO("[PixelScreenPlugin] LoadFonts completed\n");

    LoadSettings();
    LOG_INFO("[PixelScreenPlugin] LoadSettings completed\n");

    // Rebuild active device matrix zones list
    UpdateControllers();
    LOG_INFO("[PixelScreenPlugin] UpdateControllers completed\n");

    // Initialize hardware sensor manager BEFORE creating UI so
    // DeviceSettingsPage constructors can connect to its signals
    sensor_manager = new HardwareSensorManager(this);
    sensor_timer = new QTimer(this);
    sensor_timer->setInterval(1000);
    connect(sensor_timer, &QTimer::timeout, this, &PixelScreenPlugin::OnSensorTimerTimeout);
    sensor_timer->start();
    LOG_INFO("[PixelScreenPlugin] HardwareSensorManager initialized\n");

    // Create settings tab UI (DeviceSettingsPage constructors will find sensor_manager ready)
    ui = new PixelScreenTab(this);
    ui->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    LOG_INFO("[PixelScreenPlugin] PixelScreenTab created\n");

    OnSensorTimerTimeout(); // fetch only if enabled && Sensor Data mode
    LOG_INFO("[PixelScreenPlugin] Load sequence finished successfully\n");
}

QWidget* PixelScreenPlugin::GetWidget()
{
    return ui;
}

QMenu* PixelScreenPlugin::GetTrayMenu()
{
    return nullptr;
}

void PixelScreenPlugin::Unload()
{
    LOG_INFO("[PixelScreenPlugin] Unloading\n");

    // Stop all new render callbacks and wait for in-flight device sends
    // before any plugin-owned state can be destroyed.
    PixelScreenDeviceUpdateHook::Uninstall(this);
    hooked_controllers.clear();

    if (sensor_timer)
    {
        sensor_timer->stop();
        sensor_timer->deleteLater();
        sensor_timer = nullptr;
    }
    if (sensor_manager)
    {
        sensor_manager->onAboutToQuit();
        sensor_manager->deleteLater();
        sensor_manager = nullptr;
    }

    SaveSettings();

}


void PixelScreenPlugin::OnProfileAboutToLoad()
{
}

void PixelScreenPlugin::OnProfileLoad(nlohmann::json /*profile_data*/)
{
}

nlohmann::json PixelScreenPlugin::OnProfileSave()
{
    nlohmann::json profile_json;
    return profile_json;
}

unsigned char* PixelScreenPlugin::OnSDKCommand(unsigned int /*pkt_id*/, unsigned char* /*data*/, unsigned int* /*data_size*/)
{
    return nullptr;
}

/*---------------------------------------------------------*\
| Update Signals                                            |
\*---------------------------------------------------------*/
void PixelScreenPlugin::ProfileManagerUpdated(unsigned int /*update_reason*/)
{
}

void PixelScreenPlugin::ResourceManagerUpdated(unsigned int /*update_reason*/)
{
    QMetaObject::invokeMethod(this, "UpdateControllers", Qt::QueuedConnection);
}

void PixelScreenPlugin::SettingsManagerUpdated(unsigned int /*update_reason*/)
{
}


/*---------------------------------------------------------*\
| Controller Zones Filtering                                |
\*---------------------------------------------------------*/
void PixelScreenPlugin::UpdateControllers()
{
    std::vector<MatrixZoneTarget> new_matrix_zones;
    std::vector<RGBControllerInterface*> controllers_to_hook;

    for (RGBControllerInterface* controller : api->GetRGBControllers())
    {
        if (!controller) continue;
        
        bool has_matrix_zone = false;

        for (unsigned int zone_idx = 0; zone_idx < controller->GetZoneCount(); zone_idx++)
        {
            if (controller->GetZoneType(zone_idx) == ZONE_TYPE_MATRIX)
            {
                MatrixZoneTarget target;
                target.controller = controller;
                target.zone_idx = zone_idx;
                target.display_name = controller->GetName() + " - " + controller->GetZoneName(zone_idx);
                
                new_matrix_zones.push_back(std::move(target));
                has_matrix_zone = true;
            }
        }

        if (has_matrix_zone)
        {
            controllers_to_hook.push_back(controller);
        }
    }

    std::vector<RGBControllerInterface*> installed_controllers;
    std::vector<RGBControllerInterface*> old_controllers;
    {
        std::unique_lock<std::shared_mutex> lock(matrix_zones_mutex);
        matrix_zones.swap(new_matrix_zones);

        installed_controllers.reserve(controllers_to_hook.size());
        for (RGBControllerInterface* controller : controllers_to_hook)
        {
            if (PixelScreenDeviceUpdateHook::Install(controller, this, OnDeviceUpdateHook))
            {
                installed_controllers.push_back(controller);
            }
            else
            {
                LOG_ERROR("[PixelScreenPlugin] Failed to hook device output for %s\n", controller->GetName().c_str());
            }
        }

        old_controllers.swap(hooked_controllers);
        hooked_controllers = installed_controllers;
    }

    for (RGBControllerInterface* old_controller : old_controllers)
    {
        if (std::find(installed_controllers.begin(), installed_controllers.end(), old_controller)
            == installed_controllers.end())
        {
            PixelScreenDeviceUpdateHook::UninstallController(old_controller, this);
        }
    }

    if (ui)
    {
        QMetaObject::invokeMethod(ui, "UpdateDeviceList", Qt::QueuedConnection);
    }
}

DeviceMatrixSettings PixelScreenPlugin::GetDeviceSettings(const std::string& display_name)
{
    std::lock_guard<std::mutex> lock(settings_mutex);
    return settings.GetForDevice(display_name);
}

std::vector<std::string> PixelScreenPlugin::GetMatrixZoneNames()
{
    std::shared_lock<std::shared_mutex> lock(matrix_zones_mutex);
    std::vector<std::string> names;
    names.reserve(matrix_zones.size());
    for (const MatrixZoneTarget& target : matrix_zones)
    {
        names.push_back(target.display_name);
    }
    return names;
}

void PixelScreenPlugin::SetSensorUpdateInterval(int interval)
{
    if (sensor_timer)
    {
        sensor_timer->setInterval(interval);
    }
}

/*---------------------------------------------------------*\
| Font Loading and Configuration File Management            |
\*---------------------------------------------------------*/
void PixelScreenPlugin::LoadFonts()
{
    // 1. Read WLED_Text.json
    QFile file(":/resources/WLED_Text.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::string raw_data = file.readAll().toStdString();
        file.close();

        try
        {
            nlohmann::json json_data = nlohmann::json::parse(raw_data);

            auto parse_font_map = [](const nlohmann::json& font_json, std::map<std::string, Glyph>& font_map)
            {
                font_map.clear();
                for (auto& el : font_json.items())
                {
                    if (el.key().empty()) continue;
                    std::string k = el.key();
                    Glyph g;
                    for (auto& row : el.value())
                    {
                        std::vector<unsigned char> row_data;
                        for (auto& cell : row)
                        {
                            row_data.push_back(cell.get<unsigned char>());
                        }
                        g.grid.push_back(row_data);
                    }
                    if (!g.grid.empty())
                    {
                        g.height = static_cast<unsigned int>(g.grid.size());
                        g.width = static_cast<unsigned int>(g.grid[0].size());
                    }
                    font_map[k] = g;
                }
            };

            if (json_data.contains("SMALL_LETTERS")) parse_font_map(json_data["SMALL_LETTERS"], small_letters);
            if (json_data.contains("LETTERS")) parse_font_map(json_data["LETTERS"], letters);
            if (json_data.contains("LARGE_LETTERS")) parse_font_map(json_data["LARGE_LETTERS"], large_letters);
            if (json_data.contains("DIGITS")) parse_font_map(json_data["DIGITS"], digits);
            if (json_data.contains("SMALL_DIGITS")) parse_font_map(json_data["SMALL_DIGITS"], small_digits);
            if (json_data.contains("LARGE_DIGITS")) parse_font_map(json_data["LARGE_DIGITS"], large_digits);

            LOG_INFO("[OpenRGBMatrixTextPlugin] Loaded standard fonts: SMALL_LETTERS(%lu), LETTERS(%lu), LARGE_LETTERS(%lu)\n",
                     small_letters.size(), letters.size(), large_letters.size());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[OpenRGBMatrixTextPlugin] Failed to parse standard fonts JSON: %s\n", e.what());
        }
    }
    else
    {
        LOG_ERROR("[OpenRGBMatrixTextPlugin] Failed to open standard fonts resource file\n");
    }

    // 2. Read WLED_Text_ZH.json
    QFile zh_file(":/resources/WLED_Text_ZH.json");
    if (zh_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::string raw_data = zh_file.readAll().toStdString();
        zh_file.close();

        try
        {
            nlohmann::json json_data = nlohmann::json::parse(raw_data);
            zh_font.clear();

            for (auto& el : json_data.items())
            {
                std::string key = el.key();
                Glyph g;
                for (auto& row : el.value())
                {
                    std::vector<unsigned char> row_data;
                    for (auto& cell : row)
                    {
                        row_data.push_back(cell.get<unsigned char>());
                    }
                    g.grid.push_back(row_data);
                }
                if (!g.grid.empty())
                {
                    g.height = static_cast<unsigned int>(g.grid.size());
                    g.width = static_cast<unsigned int>(g.grid[0].size());
                }
                zh_font[key] = g;
            }

            LOG_INFO("[PixelScreenPlugin] Loaded Chinese fonts: ZH_FONT(%lu)\n", zh_font.size());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[PixelScreenPlugin] Failed to parse Chinese fonts JSON: %s\n", e.what());
        }
    }
    else
    {
        LOG_ERROR("[PixelScreenPlugin] Failed to open Chinese fonts resource file\n");
    }
}

void PixelScreenPlugin::LoadSettings()
{
    std::lock_guard<std::mutex> settings_lock(settings_mutex);

    std::string settings_path = (api->GetConfigurationDirectory() / "plugins" / "settings" / "PixelScreenSettings.json").string();
    QFile file(QString::fromStdString(settings_path));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::string raw_data = file.readAll().toStdString();
        file.close();

        try
        {
            nlohmann::json root = nlohmann::json::parse(raw_data);
            if (root.contains("devices") && root["devices"].is_object())
            {
                settings.device_settings.clear();
                for (auto& el : root["devices"].items())
                {
                    std::string dev_name = el.key();
                    nlohmann::json j = el.value();
                    DeviceMatrixSettings dev_s;

                    if (j.contains("enabled")) dev_s.enabled = j["enabled"];
                    if (j.contains("display_mode")) dev_s.display_mode = j["display_mode"];
                    if (j.contains("font_size")) dev_s.font_size = j["font_size"];
                    if (j.contains("custom_text")) dev_s.custom_text = j["custom_text"];
                    if (j.contains("time_format")) dev_s.time_format = j["time_format"];
                    if (j.contains("pixel_art_json")) dev_s.pixel_art_json = j["pixel_art_json"];
                    if (j.contains("scroll_direction")) dev_s.scroll_direction = j["scroll_direction"];
                    if (j.contains("scroll_speed")) dev_s.scroll_speed = j["scroll_speed"];
                    if (j.contains("fps")) dev_s.fps = j["fps"];
                    if (j.contains("text_r")) dev_s.text_r = j["text_r"];
                    if (j.contains("text_g")) dev_s.text_g = j["text_g"];
                    if (j.contains("text_b")) dev_s.text_b = j["text_b"];
                    if (j.contains("invert_color")) dev_s.invert_color = j["invert_color"];
                    if (j.contains("padding_x")) dev_s.padding_x = j["padding_x"];
                    if (j.contains("padding_y")) dev_s.padding_y = j["padding_y"];
                    if (j.contains("text_align")) dev_s.text_align = j["text_align"];
                    if (j.contains("sensor_format")) dev_s.sensor_format = j["sensor_format"].get<std::string>();
                    if (j.contains("lhm_url")) dev_s.lhm_url = j["lhm_url"].get<std::string>();
                    if (j.contains("sensor_update_interval")) dev_s.sensor_update_interval = j["sensor_update_interval"];

                    settings.device_settings[dev_name] = dev_s;
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[PixelScreenPlugin] Failed to parse settings JSON: %s\n", e.what());
        }
    }
}

void PixelScreenPlugin::SaveSettings()
{
    std::lock_guard<std::mutex> settings_lock(settings_mutex);

    // Create folders if they do not exist
    std::string settings_dir = (api->GetConfigurationDirectory() / "plugins" / "settings").string();
    QDir().mkpath(QString::fromStdString(settings_dir));

    std::string settings_path = (api->GetConfigurationDirectory() / "plugins" / "settings" / "PixelScreenSettings.json").string();
    std::ofstream file(settings_path, std::ios::out | std::ios::binary);
    if (file)
    {
        nlohmann::json root;
        nlohmann::json devices_json;

        for (const auto& pair : settings.device_settings)
        {
            nlohmann::json j;
            const auto& dev_s = pair.second;

            j["enabled"] = dev_s.enabled;
            j["display_mode"] = dev_s.display_mode;
            j["font_size"] = dev_s.font_size;
            j["custom_text"] = dev_s.custom_text;
            j["time_format"] = dev_s.time_format;
            j["pixel_art_json"] = dev_s.pixel_art_json;
            j["scroll_direction"] = dev_s.scroll_direction;
            j["scroll_speed"] = dev_s.scroll_speed;
            j["fps"] = dev_s.fps;
            j["text_r"] = dev_s.text_r;
            j["text_g"] = dev_s.text_g;
            j["text_b"] = dev_s.text_b;
            j["invert_color"] = dev_s.invert_color;
            j["padding_x"] = dev_s.padding_x;
            j["padding_y"] = dev_s.padding_y;
            j["text_align"] = dev_s.text_align;
            j["sensor_format"] = dev_s.sensor_format;
            j["lhm_url"] = dev_s.lhm_url;
            j["sensor_update_interval"] = dev_s.sensor_update_interval;

            devices_json[pair.first] = j;
        }

        root["devices"] = devices_json;
        file << root.dump(4);
        file.close();
    }
}

/*---------------------------------------------------------*\
| UTF-8 Parsing and Render Processing                       |
\*---------------------------------------------------------*/
std::vector<std::string> PixelScreenPlugin::SplitUTF8(const std::string& str)
{
    std::vector<std::string> chars;
    for (size_t i = 0; i < str.length();)
    {
        unsigned char c = str[i];
        size_t len = 1;
        if (c >= 0xf0) len = 4;
        else if (c >= 0xe0) len = 3;
        else if (c >= 0xc0) len = 2;
        
        if (i + len > str.length()) len = str.length() - i;
        chars.push_back(str.substr(i, len));
        i += len;
    }
    return chars;
}

int PixelScreenPlugin::GetSpacing(const std::string& ch, const std::string& font_size, bool time)
{
    bool is_chinese = ch.length() > 1;
    if (font_size == "Chinese")
    {
        if (is_chinese)
        {
            return 9;
        }
        else
        {
            char digit = ch[0];
            switch (digit)
            {
                case '|': return 2;
                case 'i': case 'l': case '`': case '(': case ')': case ';': case ':': case '\'': case ',': case '.': case ' ': return 3;
                case 'I': case '!': case '[': case ']': case '1': return 4;
                case 'f': case 'h': case 'j': case 'k': case 'n': case 't': case 'u': case 'x':
                case 'y': case 'Z': case 'z': case '~': case '$': case '{': case '}': case '<': case '>': return 5;
                default: return 6;
            }
        }
    }
    else if (font_size == "Medium")
    {
        if (time)
        {
            char digit = ch[0];
            switch (digit)
            {
                case ':': case ';': case '.': return 2;
                case ' ': return 1;
                default: return 5;
            }
        }
        else
        {
            char digit = ch[0];
            switch (digit)
            {
                case ' ': return 1;
                case '!': case '|': case ':': case '\'': case '.': return 2;
                case '`': case '(': case ')': case '[': case ']': case ';': case ',': case '1': return 3;
                case 'a': case 'c': case 'I': case 'i': case 'j': case 'L': case 'l': case 'r':
                case 'Y': case '$': case '^': case '*': case '-': case '=': case '+': case '{':
                case '}': case '\\': case '"': case '<': case '>': case '/': case '?': return 4;
                case 'T': case 'W': case '@': case '#': case '%': case '&': return 6;
                default: return 5;
            }
        }
    }
    else if (font_size == "Large")
    {
        if (time)
        {
            char digit = ch[0];
            switch (digit)
            {
                case ':': case ';': case '.': return 3;
                case ' ': return 2;
                default: return 6;
            }
        }
        else
        {
            char digit = ch[0];
            switch (digit)
            {
                case '|': return 2;
                case 'i': case 'l': case '`': case '(': case ')': case ';': case ':': case '\'': case ',': case '.': case ' ': return 3;
                case 'I': case '!': case '[': case ']': case '1': return 4;
                case 'f': case 'h': case 'j': case 'k': case 'n': case 't': case 'u': case 'x':
                case 'y': case 'Z': case 'z': case '~': case '$': case '{': case '}': case '<': case '>': return 5;
                default: return 6;
            }
        }
    }
    else // Small
    {
        if (time)
        {
            char digit = ch[0];
            switch (digit)
            {
                case ':': case ';': case '.': return 2;
                case ' ': return 1;
                default: return 4;
            }
        }
        else
        {
            char digit = ch[0];
            switch (digit)
            {
                case ' ': return 1;
                case 'i': case 'l': case '!': case '|': case ':': case '.': return 2;
                case 'j': case 'r': case '1': case '`': case '(': case ')': case '[': case ']':
                case ';': case '\'': case ',': return 3;
                case '~': return 5;
                default: return 4;
            }
        }
    }
    return 6;
}

Glyph PixelScreenPlugin::GetGlyph(const std::string& ch, const std::string& font_size, bool /*time*/)
{
    auto search_maps = [&](const std::map<std::string, Glyph>& primary, const std::map<std::string, Glyph>& secondary) -> Glyph
    {
        auto it = primary.find(ch);
        if (it != primary.end()) return it->second;
        it = secondary.find(ch);
        if (it != secondary.end()) return it->second;

        // Try uppercase/lowercase if single char ASCII
        if (ch.length() == 1)
        {
            char c = ch[0];
            if (c >= 'a' && c <= 'z')
            {
                std::string upper_str(1, static_cast<char>(c - 32));
                it = primary.find(upper_str);
                if (it != primary.end()) return it->second;
                it = secondary.find(upper_str);
                if (it != secondary.end()) return it->second;
            }
            else if (c >= 'A' && c <= 'Z')
            {
                std::string lower_str(1, static_cast<char>(c + 32));
                it = primary.find(lower_str);
                if (it != primary.end()) return it->second;
                it = secondary.find(lower_str);
                if (it != secondary.end()) return it->second;
            }
        }

        Glyph blank;
        blank.width = 4;
        blank.height = 8;
        blank.grid.assign(8, std::vector<unsigned char>(4, 0));
        return blank;
    };

    if (font_size == "Chinese")
    {
        if (ch.length() > 1)
        {
            auto it = zh_font.find(ch);
            if (it != zh_font.end()) return it->second;
        }
        return search_maps(large_letters, large_digits);
    }
    else if (font_size == "Large")
    {
        return search_maps(large_letters, large_digits);
    }
    else if (font_size == "Small")
    {
        return search_maps(small_letters, small_digits);
    }
    else // Medium
    {
        return search_maps(letters, digits);
    }
}

std::string PixelScreenPlugin::FormatDateTime(const std::string& format)
{
    QDateTime now = QDateTime::currentDateTime();
    QString qformat = QString::fromStdString(format);
    
    if (qformat.contains("tt"))
    {
        qformat.replace("tt", "ap");
    }
    else if (qformat.contains("t"))
    {
        qformat.replace("t", "a");
    }

    if (qformat.contains("aa"))
    {
        qformat.replace("aa", "ap");
    }
    else if (qformat.contains("AA"))
    {
        qformat.replace("AA", "AP");
    }
    
    QString result = now.toString(qformat);
    std::string str = result.toLower().toStdString();

    // Capitalize the first letter of each word
    bool cap_next = true;
    for (char& c : str)
    {
        if (std::isalpha((unsigned char)c))
        {
            if (cap_next) c = std::toupper((unsigned char)c);
            cap_next = false;
        }
        else
        {
            cap_next = true;
        }
    }

    // Convert "Am" -> "AM" and "Pm" -> "PM" when they stand as AM/PM meridian indicators
    for (size_t i = 0; i < str.size(); i++)
    {
        if ((str[i] == 'A' || str[i] == 'P') && i + 1 < str.size() && str[i + 1] == 'm')
        {
            bool prev_alpha = (i > 0 && std::isalpha((unsigned char)str[i - 1]));
            bool next_alpha = (i + 2 < str.size() && std::isalpha((unsigned char)str[i + 2]));
            if (!prev_alpha && !next_alpha)
            {
                str[i + 1] = 'M';
            }
        }
    }

    return str;
}

void PixelScreenPlugin::OnSensorTimerTimeout()
{
    if (!sensor_manager) return;

    std::string lhm_url = "http://127.0.0.1:8085/data.json";
    // Only run curl/fetchSensors if at least one enabled device is set to "Sensor Data" mode (display_mode == 4)
    {
        std::lock_guard<std::mutex> settings_lock(settings_mutex);
        bool sensor_mode_enabled = false;
        for (const auto& pair : settings.device_settings)
        {
            if (pair.second.enabled && pair.second.display_mode == 4)
            {
                sensor_mode_enabled = true;
                if (!pair.second.lhm_url.empty())
                {
                    lhm_url = pair.second.lhm_url;
                }
                break;
            }
        }

        if (!sensor_mode_enabled) return;
    }

    sensor_manager->fetchSensors(lhm_url);
}

void PixelScreenPlugin::AdvanceAnimation(DeviceMatrixSettings& dev_s)
{
    const auto now = std::chrono::steady_clock::now();
    if (dev_s.last_animation_update.time_since_epoch().count() == 0)
    {
        dev_s.last_animation_update = now;
        return;
    }

    double elapsed = std::chrono::duration<double>(now - dev_s.last_animation_update).count();
    dev_s.last_animation_update = now;
    if (elapsed <= 0.0)
    {
        return;
    }

    /* Keep the configured animation FPS as a visual sampling rate, without
     * delaying or dropping any hardware update.  The old 20 ms timer moved
     * scroll_speed / 100 pixels at 50 Hz, hence scroll_speed / 2 px/s. */
    const int animation_fps = std::max(1, std::min(60, dev_s.fps));
    const double frame_time = 1.0 / static_cast<double>(animation_fps);
    dev_s.animation_accumulator += elapsed;
    const double animation_frames = std::floor(dev_s.animation_accumulator / frame_time);
    if (animation_frames < 1.0)
    {
        return;
    }

    const double consumed_time = animation_frames * frame_time;
    dev_s.animation_accumulator -= consumed_time;
    const float distance = static_cast<float>((dev_s.scroll_speed / 2.0) * consumed_time);

    if (dev_s.scroll_direction == "Left" || dev_s.scroll_direction == "Up")
    {
        dev_s.scroll_offset -= distance;
    }
    else if (dev_s.scroll_direction == "Right" || dev_s.scroll_direction == "Down")
    {
        dev_s.scroll_offset += distance;
    }
    else if (dev_s.scroll_direction == "Ping-Pong" || dev_s.scroll_direction == "Ping-Pong Up down" || dev_s.scroll_direction == "Ping-Pong Up Down")
    {
        dev_s.scroll_offset += distance * dev_s.ping_pong_direction;
    }
}

void PixelScreenPlugin::OnDeviceUpdateHook(
    void* callback_arg,
    RGBControllerInterface* controller,
    const PixelScreenDeviceUpdateHook::OriginalCall& original_call)
{
    PixelScreenPlugin* plugin = static_cast<PixelScreenPlugin*>(callback_arg);
    if (!plugin)
    {
        original_call.Invoke(controller);
        return;
    }

    plugin->SendOverlayFrame(controller, original_call);
}

void PixelScreenPlugin::SendOverlayFrame(
    RGBControllerInterface* controller,
    const PixelScreenDeviceUpdateHook::OriginalCall& original_call)
{
    if (!controller)
    {
        return;
    }

    std::unique_lock<std::mutex> settings_lock(settings_mutex);
    std::shared_lock<std::shared_mutex> zones_lock(matrix_zones_mutex);

    bool has_enabled_zone = false;
    for (const MatrixZoneTarget& target : matrix_zones)
    {
        const auto setting = settings.device_settings.find(target.display_name);
        if (target.controller == controller
            && setting != settings.device_settings.end()
            && setting->second.enabled)
        {
            has_enabled_zone = true;
            break;
        }
    }

    const std::size_t color_count = controller->GetLEDCount();
    RGBColor* colors = color_count == 0 ? nullptr : controller->GetColorsPointer();
    if (!has_enabled_zone || !colors)
    {
        zones_lock.unlock();
        settings_lock.unlock();
        original_call.Invoke(controller);
        return;
    }

    std::vector<RGBColor> backup_colors;
    std::vector<RGBColor> rendered_colors;
    bool original_started = false;

    auto restore_colors = [&]
    {
        if (backup_colors.size() != color_count)
        {
            return;
        }

        if (rendered_colors.size() == color_count
            && std::equal(rendered_colors.begin(), rendered_colors.end(), colors))
        {
            std::copy(backup_colors.begin(), backup_colors.end(), colors);
        }
        else if (rendered_colors.empty())
        {
            std::copy(backup_colors.begin(), backup_colors.end(), colors);
        }
    };

    try
    {
        backup_colors.assign(colors, colors + color_count);

        for (const MatrixZoneTarget& target : matrix_zones)
        {
            if (target.controller != controller)
            {
                continue;
            }

            auto setting = settings.device_settings.find(target.display_name);
            if (setting == settings.device_settings.end() || !setting->second.enabled)
            {
                continue;
            }

            AdvanceAnimation(setting->second);
            OverlayTextOnBuffer(target, setting->second, colors, color_count);
        }

        rendered_colors.assign(colors, colors + color_count);
        zones_lock.unlock();
        settings_lock.unlock();
        original_started = true;
        original_call.Invoke(controller);
        restore_colors();
    }
    catch (...)
    {
        if (zones_lock.owns_lock()) zones_lock.unlock();
        if (settings_lock.owns_lock()) settings_lock.unlock();
        restore_colors();
        if (!original_started)
        {
            original_call.Invoke(controller);
            return;
        }
        throw;
    }
}

void PixelScreenPlugin::OverlayTextOnBuffer(const MatrixZoneTarget& target,
                                            DeviceMatrixSettings& dev_s,
                                            RGBColor* colors,
                                            std::size_t color_count)
{
    if (!target.controller || !colors) return;
    const unsigned int zone_idx = target.zone_idx;
    if (zone_idx >= target.controller->GetZoneCount()) return;

    const unsigned int matrix_w  = target.controller->GetZoneMatrixMapWidth(zone_idx);
    const unsigned int matrix_h  = target.controller->GetZoneMatrixMapHeight(zone_idx);
    const unsigned int* map      = target.controller->GetZoneMatrixMapData(zone_idx);
    const unsigned int start_idx = target.controller->GetZoneStartIndex(zone_idx);

    if (!map || matrix_w == 0 || matrix_h == 0) return;
    const std::size_t map_size = static_cast<std::size_t>(matrix_w) * matrix_h;

    struct RenderedGlyph {
        Glyph glyph;
        int offset_x;
        int offset_y;    // line row offset in pixels
        bool blink_off;  // true = glyph occupies space but renders no pixels
    };

    int total_width = 0;
    int total_height = 0;
    int glyph_h = 8;
    std::vector<RenderedGlyph> rendered_glyphs;

    if (dev_s.display_mode == 2 || dev_s.display_mode == 3)
    {
        try
        {
            nlohmann::json art_j = nlohmann::json::parse(dev_s.pixel_art_json);
            if (art_j.is_array() && !art_j.empty())
            {
                Glyph custom_glyph;
                custom_glyph.height = static_cast<unsigned int>(art_j.size());
                custom_glyph.width = 0;
                custom_glyph.grid.resize(custom_glyph.height);

                for (size_t r = 0; r < art_j.size(); r++)
                {
                    if (art_j[r].is_array())
                    {
                        if (art_j[r].size() > custom_glyph.width)
                        {
                            custom_glyph.width = static_cast<unsigned int>(art_j[r].size());
                        }
                        custom_glyph.grid[r].resize(art_j[r].size());
                        for (size_t c = 0; c < art_j[r].size(); c++)
                        {
                            custom_glyph.grid[r][c] = art_j[r][c].get<int>();
                        }
                    }
                }

                if (custom_glyph.width > 0 && custom_glyph.height > 0)
                {
                    rendered_glyphs.push_back({custom_glyph, 0, 0, false});
                    total_width = custom_glyph.width;
                    total_height = custom_glyph.height;
                    glyph_h = custom_glyph.height;
                }
            }
        }
        catch (...)
        {
        }
        if (rendered_glyphs.empty()) return;
    }
    else
    {
        std::string text = "";
        bool is_time_mode = (dev_s.display_mode == 0);
        bool colon_blink_off = false;
        if (is_time_mode)
        {
            text = FormatDateTime(dev_s.time_format);
            QTime now = QTime::currentTime();
            colon_blink_off = (now.second() % 2 != 0);
        }
        else if (dev_s.display_mode == 1)
        {
            text = dev_s.custom_text;
        }
        else if (dev_s.display_mode == 4)
        {
            if (sensor_manager)
                text = sensor_manager->resolveFormat(dev_s.sensor_format);
            else
                text = dev_s.sensor_format;
        }

        // Split text into lines on \n
        std::vector<std::string> lines;
        {
            std::string remaining = text;
            size_t pos;
            while ((pos = remaining.find('\n')) != std::string::npos)
            {
                lines.push_back(remaining.substr(0, pos));
                remaining = remaining.substr(pos + 1);
            }
            lines.push_back(remaining);
        }

        // Determine glyph height for line spacing
        {
            for (const auto& line : lines)
            {
                std::vector<std::string> first_chars = SplitUTF8(line);
                if (!first_chars.empty())
                {
                    Glyph sample = GetGlyph(first_chars[0], dev_s.font_size, is_time_mode);
                    if (sample.height > 0) { glyph_h = (int)sample.height; break; }
                }
            }
        }
        int line_spacing = glyph_h + 1; // 1px gap between lines

        // Pass 1: compute width of each line
        std::vector<int> line_widths;
        for (const auto& line : lines)
        {
            std::vector<std::string> chars = SplitUTF8(line);
            int w = 0;
            for (const auto& ch : chars)
                w += GetSpacing(ch, dev_s.font_size, is_time_mode);
            line_widths.push_back(w);
        }

        for (int w : line_widths)
            if (w > total_width) total_width = w;
        if (total_width <= 0) total_width = 1;

        // Pass 2: build rendered glyphs
        for (int line_idx = 0; line_idx < (int)lines.size(); line_idx++)
        {
            std::vector<std::string> chars = SplitUTF8(lines[line_idx]);
            int line_w = line_widths[line_idx];
            int line_y = line_idx * line_spacing;

            int line_align = 0;
            if (dev_s.text_align == 1)      // Center
                line_align = (total_width - line_w) / 2;
            else if (dev_s.text_align == 2) // End
                line_align = total_width - line_w;

            int line_x = 0;
            for (const auto& ch : chars)
            {
                Glyph g = GetGlyph(ch, dev_s.font_size, is_time_mode);
                int spacing = GetSpacing(ch, dev_s.font_size, is_time_mode);
                bool blink_off = colon_blink_off && (ch == ":");
                rendered_glyphs.push_back({g, line_x + line_align, line_y, blink_off});
                line_x += spacing;
            }
        }

        total_height = lines.empty() ? glyph_h : static_cast<int>((lines.size() - 1) * line_spacing + glyph_h);
        if (total_height <= 0) total_height = 1;
    }

    bool is_vert_scroll = (dev_s.scroll_direction == "Up" || dev_s.scroll_direction == "Down" ||
                           dev_s.scroll_direction == "Ping-Pong Up down" || dev_s.scroll_direction == "Ping-Pong Up Down");
    bool is_horiz_scroll = (dev_s.scroll_direction == "Left" || dev_s.scroll_direction == "Right" || dev_s.scroll_direction == "Ping-Pong");

    int buffer_width = total_width;
    if (dev_s.scroll_direction == "Left" || dev_s.scroll_direction == "Right")
    {
        buffer_width += matrix_w / 2;
    }

    int buffer_height = total_height;
    if (dev_s.scroll_direction == "Up" || dev_s.scroll_direction == "Down")
    {
        buffer_height += matrix_h / 2;
    }
    
    if (dev_s.scroll_direction == "Ping-Pong")
    {
        float min_offset = (float)matrix_w - (float)buffer_width;
        if (min_offset > 0.0f) min_offset = 0.0f;
        
        if (dev_s.scroll_offset <= min_offset)
        {
            dev_s.scroll_offset = min_offset;
            dev_s.ping_pong_direction = 1;
        }
        else if (dev_s.scroll_offset >= 0.0f)
        {
            dev_s.scroll_offset = 0.0f;
            dev_s.ping_pong_direction = -1;
        }
    }
    else if (dev_s.scroll_direction == "Ping-Pong Up down" || dev_s.scroll_direction == "Ping-Pong Up Down")
    {
        float min_offset = (float)matrix_h - (float)buffer_height;
        if (min_offset > 0.0f) min_offset = 0.0f;

        if (dev_s.scroll_offset <= min_offset)
        {
            dev_s.scroll_offset = min_offset;
            dev_s.ping_pong_direction = 1;
        }
        else if (dev_s.scroll_offset >= 0.0f)
        {
            dev_s.scroll_offset = 0.0f;
            dev_s.ping_pong_direction = -1;
        }
    }
    else if (is_horiz_scroll)
    {
        if (dev_s.scroll_offset <= -buffer_width) dev_s.scroll_offset += buffer_width;
        if (dev_s.scroll_offset >= buffer_width) dev_s.scroll_offset -= buffer_width;
    }
    else if (is_vert_scroll)
    {
        if (dev_s.scroll_offset <= -buffer_height) dev_s.scroll_offset += buffer_height;
        if (dev_s.scroll_offset >= buffer_height) dev_s.scroll_offset -= buffer_height;
    }
    else
    {
        dev_s.scroll_offset = 0.0f;
    }

    int screen_align = 0;
    if (!is_horiz_scroll)
    {
        if (dev_s.text_align == 1)      // Center
        {
            screen_align = ((int)matrix_w - total_width) / 2;
        }
        else if (dev_s.text_align == 2) // End
        {
            screen_align = (int)matrix_w - total_width;
        }
    }

    RGBColor text_color = ToRGBColor(dev_s.text_r, dev_s.text_g, dev_s.text_b);
    
    for (unsigned int y = 0; y < matrix_h; y++)
    {
        for (unsigned int x = 0; x < matrix_w; x++)
        {
            const std::size_t map_idx = static_cast<std::size_t>(y) * matrix_w + x;
            if (map_idx >= map_size) continue;

            unsigned int led_idx = map[map_idx];
            if (led_idx == 0xFFFFFFFF) continue;
            
            int src_x;
            if (!is_horiz_scroll)
            {
                src_x = (int)x - screen_align;
            }
            else
            {
                src_x = (int)std::floor((float)x - dev_s.scroll_offset) % buffer_width;
                if (src_x < 0) src_x += buffer_width;
            }
            
            int src_y;
            if (!is_vert_scroll)
            {
                src_y = (int)y;
            }
            else
            {
                src_y = (int)std::floor((float)y - dev_s.scroll_offset) % buffer_height;
                if (src_y < 0) src_y += buffer_height;
            }

            src_x = src_x - dev_s.padding_x;
            src_y = src_y - dev_s.padding_y;
            
            bool pixel_on = false;
            
            if (src_x >= 0 && src_x < total_width && src_y >= 0 && src_y < total_height)
            {
                for (const auto& rg : rendered_glyphs)
                {
                    if (rg.blink_off) continue;
                    int local_x = src_x - rg.offset_x;
                    int local_y = src_y - rg.offset_y;
                    if (local_y >= 0 && static_cast<std::size_t>(local_y) < rg.glyph.grid.size())
                    {
                        const auto& row = rg.glyph.grid[local_y];
                        if (local_x >= 0 && static_cast<std::size_t>(local_x) < row.size())
                        {
                            if (row[local_x] > 0)
                            {
                                pixel_on = true;
                                break;
                            }
                        }
                    }
                }
            }
            
            const std::size_t color_index = static_cast<std::size_t>(start_idx) + led_idx;
            if (color_index >= color_count) continue;
            
            RGBColor underlying_color = colors[color_index];
            RGBColor pixel_color;
            if (dev_s.invert_color)
            {
                pixel_color = pixel_on ? underlying_color : text_color;
            }
            else
            {
                pixel_color = pixel_on ? text_color : underlying_color;
            }
            
            colors[color_index] = pixel_color;
        }
    }
}

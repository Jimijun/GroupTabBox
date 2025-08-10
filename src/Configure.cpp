#include "Configure.h"

#include <array>
#include <fstream>
#include <regex>
#include <unordered_map>

using SettingPair = std::pair<std::string, std::string>;

const char *kConfigFile = "GroupTabBox.ini";

const std::unordered_map<std::string, UINT> kModifierMap = {
    { "SHIFT", MOD_SHIFT }, { "CTRL", MOD_CONTROL }, { "ALT", MOD_ALT }
};
const std::unordered_map<std::string, UINT> kKeyMap = {
    { "F1", VK_F1 }, { "F2", VK_F2 }, { "F3", VK_F3 }, { "F4", VK_F4 },
    { "F5", VK_F5 }, { "F6", VK_F6 }, { "F7", VK_F7 }, { "F8", VK_F8 },
    { "F9", VK_F9 }, { "F10", VK_F10 }, { "F11", VK_F11 }, { "F12", VK_F12 },
    { "`", VK_OEM_3 }, { "TAB", VK_TAB }
};

static void writeIntoMember(const std::string &value_str, bool *mem_ptr)
{
    if (value_str.length() != 1 || !mem_ptr)
        return;
    if (value_str[0] == '0') {
        *mem_ptr = false;
    } else {
        *mem_ptr = true;
    }
}

static void writeIntoMember(const std::string &value_str, float *mem_ptr)
{
    if (value_str.empty() || !mem_ptr )
        return;
    *mem_ptr = std::stof(value_str);
}

static void writeIntoMember(const std::string &value_str, UINT *mem_ptr)
{
    if (value_str.empty() || !mem_ptr )
        return;

    auto it = kKeyMap.find(value_str);
    if (it != kKeyMap.end()) {
        *mem_ptr = it->second;
    } else if (value_str.length() == 1
            && (value_str[0] >= 'A' && value_str[0] <= 'Z'
                || value_str[0] >= '0' && value_str[0] <= '9')) {
        *mem_ptr = value_str[0];
    }
}

static void writeIntoMember(const std::string &value_str, Configure::HotkeyPair *mem_ptr)
{
    if (value_str.empty() || !mem_ptr || value_str == "0")
        return;

    std::vector<std::string> tokens;
    // add a '+' at the end to simplify splitting
    std::string str = value_str + '+';
    // slipt str by '+'
    size_t pos = str.find('+');
    while (pos != std::string::npos) {
        tokens.push_back(str.substr(0, pos));
        str = str.substr(pos + 1, str.length());
        pos = str.find('+');
    }
    if (tokens.size() < 2 || tokens.size() > 4)
        return;

    Configure::HotkeyPair backup = *mem_ptr;
    // parse key
    std::string key = std::move(tokens.back());
    tokens.pop_back();
    writeIntoMember(key, &mem_ptr->second);

    // parse modifiers
    while (!tokens.empty()) {
        auto it = kModifierMap.find(tokens.back());
        if (it == kModifierMap.end()) {
            *mem_ptr = backup;
            return;
        }
        mem_ptr->first |= it->second;
        tokens.pop_back();
    }
}

static void parseString(const SettingPair &setting_pair, void *mem_ptr)
{
    if (setting_pair.first.empty() || setting_pair.second.empty()
            || !mem_ptr)
        return;

    switch (setting_pair.first[0]) {
    case 'b':
        writeIntoMember(setting_pair.second, static_cast<bool *>(mem_ptr));
        break;

    case 'f':
        writeIntoMember(setting_pair.second, static_cast<float *>(mem_ptr));
        break;

    case 'k':
        writeIntoMember(setting_pair.second, static_cast<UINT *>(mem_ptr));
        break;

    case 'h':
        writeIntoMember(setting_pair.second, static_cast<Configure::HotkeyPair *>(mem_ptr));
        break;

    case 's':
        *static_cast<std::string *>(mem_ptr) = setting_pair.second;
        break;

    default:
        break;
    }
}

bool Configure::load()
{
    std::ifstream file(kConfigFile, std::ios::in);
    if (!file.is_open())
        return false;

    enum SettingGroup
    {
        None,
        WindowFilter,
        UI,
        Hotkeys,
        GroupNumber
    } current_setting_group = SettingGroup::None;

    static const std::unordered_map<std::string, SettingGroup> group_tag_map = {
        { "[Window Filter]", SettingGroup::WindowFilter },
        { "[UI]", SettingGroup::UI },
        { "[Hotkeys]", SettingGroup::Hotkeys },
    };

    using ConfigMap = std::unordered_map<std::string, void *>;
    const std::array<ConfigMap, SettingGroup::GroupNumber> setting_group_map = {
        ConfigMap{},  // None
        ConfigMap{  // Window Filter
            { "bIgnoreMinimized", &m_ignore_minimized },
        },
        ConfigMap{  // UI
            { "sFontFamily", &m_font_family },
            { "fFontSize", &m_font_size },
            { "fBackgroundAlpha", &m_background_alpha },
        },
        ConfigMap{  // Hotkeys
            { "kSwitchGroupkey", &m_switch_group_key },
            { "bEnablePrevGroupHotkey", &m_enable_prev_group_hotkey },
            { "kSwitchWindowkey", &m_switch_window_key },
            { "bEnablePrevWindowHotkey", &m_enable_prev_window_hotkey },
            { "kSwitchMonitorkey", &m_switch_monitor_key },
            { "bEnablePrevMonitorHotkey", &m_enable_prev_monitor_hotkey },
            { "hKeepShowingHotkey", &m_keep_showing_hotkey },
        },
    };

    SettingPair setting_pair;
    std::string buffer;
    while (std::getline(file, buffer)) {
        // ignore empty lines and comments
        if (buffer.empty() || buffer[0] == '#')
            continue;

        // current line is group tag
        if (buffer[0] == '[')
            current_setting_group = SettingGroup::None;

        if (current_setting_group == SettingGroup::None) {
            auto it = group_tag_map.find(buffer);
            if (it != group_tag_map.end()) {
                current_setting_group = it->second;
            } else {
                continue;
            }
        } else {
            std::smatch match;
            if (!std::regex_search(buffer, match, std::regex("\s*^(.*)\s*=\s*(.*)\s*$"))
                    || match.size() < 2) {
                continue;
            }
            setting_pair = { std::move(match[1].str()), std::move(match[2].str()) };

            const ConfigMap &config_map = setting_group_map[current_setting_group];
            auto it = config_map.find(setting_pair.first);
            // write into member
            if (it != config_map.end())
                parseString(setting_pair, it->second);
        }
    }

    // validate values
    if (m_background_alpha > 1.0f) {
        m_background_alpha = 1.0f;
    } else if (m_background_alpha < 0.01f) {
        m_background_alpha = 0.01f;
    }
    if (m_switch_group_key == 0)
        m_enable_prev_group_hotkey = false;
    if (m_switch_window_key == 0)
        m_enable_prev_window_hotkey = false;
    if (m_switch_monitor_key == 0)
        m_enable_prev_monitor_hotkey = false;

    return true;
}

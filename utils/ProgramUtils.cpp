#include "ProgramUtils.h"

#include <Windows.h>

#include <array>

const wchar_t *kAutoStartRegPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring programPath()
{
    static std::wstring s_path;
    if (s_path.empty()) {
        std::array<wchar_t, MAX_PATH> path = { 0 };
        GetModuleFileName(nullptr, path.data(), path.size());
        s_path = path.data();
    }
    return s_path;
}

std::wstring programDir()
{
    const std::wstring &path = programPath();
    size_t pos = path.find_last_of(L"\\");
    if (pos == std::wstring::npos)
        return L"";
    return path.substr(0, pos + 1);
}

void setAutoStart(const wchar_t *reg_key, bool enable)
{
    HKEY key;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, kAutoStartRegPath, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
        std::array<wchar_t, MAX_PATH> value = { 0 };
        DWORD value_len = MAX_PATH;
        bool got = RegGetValue(key, nullptr, reg_key, RRF_RT_REG_SZ, nullptr, value.data(), &value_len)
                == ERROR_SUCCESS;
        if (enable) {
            const std::wstring &path = programPath();
            // update the existing value or create a new one
            if ((got && wcsncmp(path.data(), value.data(), value_len) != 0) || !got) {
                RegSetValueEx(key, reg_key, 0, REG_SZ, reinterpret_cast<const BYTE *>(path.data()),
                        (wcslen(path.data()) + 1) * sizeof(wchar_t));
            }
        } else if (got) {
            // delete the existing value
            RegDeleteValue(key, reg_key);
        }
        RegCloseKey(key);
    }
}

// ref https://cplusplus.com/forum/windows/101207
bool isAlreadyAdmin()
{
    static BOOL is_admin = 2;
    if (is_admin == 2) {
        PSID admin_group = nullptr;
        SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
        if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &admin_group))
            return false;
        CheckTokenMembership(nullptr, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin == TRUE;
}

void runAsAdmin()
{
    if (isAlreadyAdmin())
        return;

    const std::wstring &path = programPath();
    if (!path.empty()) {
        SHELLEXECUTEINFO sei = { 0 };
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = path.data();
        sei.nShow = SW_SHOWNORMAL;
        ShellExecuteEx(&sei);
    }
}

#include <includes.h>

namespace utils {

bool is_elevated() {
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elev;
        DWORD size = sizeof(elev);
        if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &size)) {
            elevated = elev.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return elevated == TRUE;
}

bool relaunch_as_admin() {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) return false;

    SHELLEXECUTEINFOA sei{};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = "runas";
    sei.lpFile = path;
    sei.nShow  = SW_SHOWNORMAL;

    if (ShellExecuteExA(&sei)) return true;

    DWORD err = GetLastError();
    if (err == ERROR_CANCELLED) {
        std::cout << ANSI_YELLOW << "[!]" << ANSI_RESET << " uac cancelled by user.\n";
    } else {
        std::cerr << ANSI_RED << "[!!!]" << ANSI_RESET
                  << " failed to relaunch as admin (error " << err << ").\n";
    }
    return false;
}

std::string env_or_empty(const char* name) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}

}

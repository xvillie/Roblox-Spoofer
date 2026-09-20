#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <filesystem>
#include <string>

typedef struct sqlite3 sqlite3;
typedef int  (__cdecl* pfn_sqlite3_open)(const char*, sqlite3**);
typedef int  (__cdecl* pfn_sqlite3_close)(sqlite3*);
typedef int  (__cdecl* pfn_sqlite3_exec)(sqlite3*, const char*, int(*)(void*, int, char**, char**), void*, char**);
typedef void (__cdecl* pfn_sqlite3_free)(void*);
typedef int  (__cdecl* pfn_sqlite3_changes)(sqlite3*);
typedef const char* (__cdecl* pfn_sqlite3_errmsg)(sqlite3*);

namespace cleaner {

struct sqlite_api {
    HMODULE hmod = nullptr;
    pfn_sqlite3_open    open    = nullptr;
    pfn_sqlite3_close   close   = nullptr;
    pfn_sqlite3_exec    exec    = nullptr;
    pfn_sqlite3_free    free_fn = nullptr;
    pfn_sqlite3_changes changes = nullptr;
    pfn_sqlite3_errmsg  errmsg  = nullptr;
    bool loaded() const { return hmod != nullptr; }
};

bool clean_roblox_cookies(const std::string& userprofile);
void clean_browser_cookies(const std::string& local_appdata, const std::string& appdata);

}

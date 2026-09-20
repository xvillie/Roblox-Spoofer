#include <includes.h>

namespace cleaner {

namespace {

constexpr int sqlite_ok = 0;
thread_local std::string last_error;

sqlite_api load_sqlite() {
    sqlite_api api;
    auto bind = [&](HMODULE mod) -> bool {
        api.hmod    = mod;
        api.open    = reinterpret_cast<pfn_sqlite3_open>(GetProcAddress(mod, "sqlite3_open"));
        api.close   = reinterpret_cast<pfn_sqlite3_close>(GetProcAddress(mod, "sqlite3_close"));
        api.exec    = reinterpret_cast<pfn_sqlite3_exec>(GetProcAddress(mod, "sqlite3_exec"));
        api.free_fn = reinterpret_cast<pfn_sqlite3_free>(GetProcAddress(mod, "sqlite3_free"));
        api.changes = reinterpret_cast<pfn_sqlite3_changes>(GetProcAddress(mod, "sqlite3_changes"));
        api.errmsg  = reinterpret_cast<pfn_sqlite3_errmsg>(GetProcAddress(mod, "sqlite3_errmsg"));
        return api.open && api.close && api.exec && api.free_fn && api.changes && api.errmsg;
    };

    const char* names[] = { "winsqlite3.dll", "sqlite3.dll", "mozsqlite3.dll" };
    for (const char* dll : names) {
        HMODULE m = LoadLibraryA(dll);
        if (m && bind(m)) return api;
        if (m) FreeLibrary(m);
    }

    std::vector<std::filesystem::path> roots;
    for (const char* env : { "LOCALAPPDATA", "ProgramFiles", "ProgramFiles(x86)" }) {
        const char* p = std::getenv(env);
        if (!p) continue;
        roots.emplace_back(std::string(p) + "\\Google\\Chrome\\Application");
        roots.emplace_back(std::string(p) + "\\Microsoft\\Edge\\Application");
        roots.emplace_back(std::string(p) + "\\Mozilla Firefox");
    }

    for (const auto& root : roots) {
        if (!std::filesystem::exists(root)) continue;
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            const std::string fname = it->path().filename().string();
            if (fname != "sqlite3.dll" && fname != "mozsqlite3.dll") continue;
            HMODULE m = LoadLibraryA(it->path().string().c_str());
            if (!m) continue;
            if (bind(m)) return api;
            FreeLibrary(m);
        }
    }
    return {};
}

const sqlite_api& sqlite() {
    static sqlite_api api = load_sqlite();
    return api;
}

std::vector<std::filesystem::path> find_files(const std::filesystem::path& root, const std::string& name) {
    std::vector<std::filesystem::path> out;
    if (!std::filesystem::exists(root)) return out;
    std::error_code ec;
    for (auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (entry.is_regular_file(ec) && entry.path().filename().string() == name) {
            out.push_back(entry.path());
        }
    }
    return out;
}

int delete_roblox_rows(const std::filesystem::path& db_path, bool chromium) {
    const auto& sq = sqlite();
    last_error.clear();
    if (!sq.loaded()) {
        last_error = "sqlite runtime not found (winsqlite3/sqlite3/mozsqlite3).";
        return -1;
    }

    std::filesystem::path tmp = db_path.string() + ".tmp";
    std::error_code ec;
    std::filesystem::copy_file(db_path, tmp,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        last_error = "copy_file failed: " + ec.message();
        return -1;
    }

    sqlite3* db = nullptr;
    if (sq.open(tmp.string().c_str(), &db) != sqlite_ok) {
        last_error = db && sq.errmsg
            ? std::string("sqlite3_open failed: ") + sq.errmsg(db)
            : "sqlite3_open failed";
        if (db) sq.close(db);
        std::filesystem::remove(tmp, ec);
        return -1;
    }

    sq.exec(db, "PRAGMA journal_mode=DELETE;", nullptr, nullptr, nullptr);

    const char* sql = chromium
        ? "DELETE FROM cookies "
          "WHERE host_key = 'roblox.com' "
          "   OR host_key = '.roblox.com' "
          "   OR host_key LIKE '%.roblox.com';"
        : "DELETE FROM moz_cookies "
          "WHERE host = 'roblox.com' "
          "   OR host = '.roblox.com' "
          "   OR host LIKE '%.roblox.com' "
          "   OR baseDomain = 'roblox.com';";

    char* err = nullptr;
    int rc = sq.exec(db, sql, nullptr, nullptr, &err);

    if (!chromium && rc != sqlite_ok && err &&
        std::string(err).find("no such column: baseDomain") != std::string::npos) {
        sq.free_fn(err);
        err = nullptr;
        const char* fallback =
            "DELETE FROM moz_cookies "
            "WHERE host = 'roblox.com' "
            "   OR host = '.roblox.com' "
            "   OR host LIKE '%.roblox.com';";
        rc = sq.exec(db, fallback, nullptr, nullptr, &err);
    }
    if (err) sq.free_fn(err);

    int rows = -1;
    if (rc == sqlite_ok) {
        rows = sq.changes(db);
    } else if (sq.errmsg) {
        last_error = std::string("sqlite3_exec failed: ") + sq.errmsg(db);
    } else {
        last_error = "sqlite3_exec failed";
    }

    sq.close(db);

    if (rows >= 0) {
        std::filesystem::rename(tmp, db_path, ec);
        if (ec) {
            std::filesystem::copy_file(tmp, db_path,
                std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove(tmp, ec);
            if (ec && last_error.empty()) last_error = "copy back failed: " + ec.message();
        }
        std::error_code ignored;
        std::filesystem::remove(db_path.string() + "-wal", ignored);
        std::filesystem::remove(db_path.string() + "-shm", ignored);
    } else {
        std::filesystem::remove(tmp, ec);
    }
    return rows;
}

void kill_browsers() {
    std::system("taskkill /F /T "
                "/IM chrome.exe /IM msedge.exe /IM firefox.exe "
                "/IM opera.exe /IM opera_gx.exe /IM crashpad_handler.exe "
                ">nul 2>&1");
}

struct chromium_browser {
    std::string name;
    std::string profile_root;
};

void clear_chromium(const std::vector<chromium_browser>& browsers) {
    for (const auto& b : browsers) {
        if (!std::filesystem::exists(b.profile_root)) continue;

        auto files = find_files(b.profile_root, "Cookies");
        if (files.empty()) {
            std::cout << ANSI_YELLOW << "[!]" << ANSI_RESET << " " << b.name
                      << ": no cookies db found.\n";
            continue;
        }

        int failed = 0;
        for (const auto& f : files) {
            if (delete_roblox_rows(f, true) < 0) {
                ++failed;
                std::cout << ANSI_DIM << "    - " << f.string() << " :: "
                          << (last_error.empty() ? "unknown error" : last_error)
                          << ANSI_RESET << "\n";
            }
        }
        if (failed == 0) {
            std::cout << ANSI_GREEN << "[v]" << ANSI_RESET << " " << b.name
                      << ": roblox.com cookies cleared.\n";
        } else {
            std::cout << ANSI_RED << "[!!!]" << ANSI_RESET << " " << b.name
                      << ": " << failed << " profile(s) failed - close the browser and try again.\n";
        }
    }
}

void clear_firefox(const std::string& appdata) {
    const std::filesystem::path profiles(appdata + "\\Mozilla\\Firefox\\Profiles");
    if (!std::filesystem::exists(profiles)) return;

    auto files = find_files(profiles, "cookies.sqlite");
    if (files.empty()) {
        std::cout << ANSI_YELLOW << "[!]" << ANSI_RESET
                  << " firefox: no cookies.sqlite found.\n";
        return;
    }

    int failed = 0;
    for (const auto& f : files) {
        if (delete_roblox_rows(f, false) < 0) {
            ++failed;
            std::cout << ANSI_DIM << "    - " << f.string() << " :: "
                      << (last_error.empty() ? "unknown error" : last_error)
                      << ANSI_RESET << "\n";
        }
    }
    if (failed == 0) {
        std::cout << ANSI_GREEN << "[v]" << ANSI_RESET
                  << " firefox: roblox.com cookies cleared.\n";
    } else {
        std::cout << ANSI_RED << "[!!!]" << ANSI_RESET
                  << " firefox: " << failed << " profile(s) failed - close firefox and try again.\n";
    }
}

}

bool clean_roblox_cookies(const std::string& userprofile) {
    const std::string path = userprofile +
        "\\AppData\\Local\\Roblox\\LocalStorage\\RobloxCookies.dat";

    if (!std::filesystem::exists(path)) {
        logs::write(logs::level::warn, "roblox cookie file not found.");
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        logs::write(logs::level::error, "failed to delete roblox cookie file: " + ec.message());
        return false;
    }
    logs::write(logs::level::success, "roblox cookie file deleted.");
    return true;
}

void clean_browser_cookies(const std::string& local_appdata, const std::string& appdata) {
    kill_browsers();

    const std::vector<chromium_browser> browsers = {
        { "google chrome",  local_appdata + "\\Google\\Chrome\\User Data" },
        { "microsoft edge", local_appdata + "\\Microsoft\\Edge\\User Data" },
        { "opera gx",       appdata       + "\\Opera Software\\Opera GX Stable" },
        { "opera",          appdata       + "\\Opera Software\\Opera Stable"    },
    };

    clear_chromium(browsers);
    clear_firefox(appdata);
}

}

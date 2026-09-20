#include <includes.h>

namespace {

enum class action {
    spoof_only        = 1,
    clean_browser     = 2,
    clean_roblox      = 3,
    full              = 4,
    quit              = 5,
};

void print_menu() {
    std::cout << ANSI_BOLD << "  select an option:" << ANSI_RESET << "\n\n"
              << "  " << ANSI_CYAN << "[1]" << ANSI_RESET << " spoof mac address only\n"
              << "  " << ANSI_CYAN << "[2]" << ANSI_RESET << " clean browser cookies only (chrome/edge/firefox/opera)\n"
              << "  " << ANSI_CYAN << "[3]" << ANSI_RESET << " clean roblox client cookies only\n"
              << "  " << ANSI_CYAN << "[4]" << ANSI_RESET << " full  (spoof + clean everything)\n"
              << "  " << ANSI_CYAN << "[5]" << ANSI_RESET << " quit\n\n";
}

std::optional<action> read_choice() {
    std::cout << "  > ";
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    try {
        int n = std::stoi(line);
        if (n >= 1 && n <= 5) return static_cast<action>(n);
    } catch (...) {}
    return std::nullopt;
}

void do_spoof() {
    logs::write(logs::level::warn, "--- mac spoof ---");
    network::spoof_interactive();
}

void do_clean_browser(const std::string& local_appdata, const std::string& appdata) {
    logs::write(logs::level::warn, "--- browser cookie clean ---");
    cleaner::clean_browser_cookies(local_appdata, appdata);
}

void do_clean_roblox(const std::string& userprofile) {
    logs::write(logs::level::warn, "--- roblox cookie clean ---");
    cleaner::clean_roblox_cookies(userprofile);
}

void do_full(const std::string& userprofile,
             const std::string& local_appdata,
             const std::string& appdata)
{
    do_clean_roblox(userprofile);
    do_clean_browser(local_appdata, appdata);
    do_spoof();
}

}

int main() {
    console::enable_ansi();

    if (!utils::is_elevated()) {
        logs::write(logs::level::warn, "administrator privileges required. requesting elevation...");
        if (utils::relaunch_as_admin()) {
            logs::write(logs::level::info, "elevated window opened. this window will now close.");
        }
        return 0;
    }

    console::print_title("Spoofer");

    const std::string userprofile   = utils::env_or_empty("USERPROFILE");
    const std::string local_appdata = utils::env_or_empty("LOCALAPPDATA");
    const std::string appdata       = utils::env_or_empty("APPDATA");

    if (userprofile.empty() || local_appdata.empty() || appdata.empty()) {
        logs::write(logs::level::error, "could not read required environment variables.");
        console::pause_and_exit();
        return 1;
    }

    print_menu();

    auto choice = read_choice();
    if (!choice) {
        logs::write(logs::level::error, "invalid selection.");
        console::pause_and_exit();
        return 1;
    }

    std::cout << "\n";
    switch (*choice) {
        case action::spoof_only:    do_spoof();                                            break;
        case action::clean_browser: do_clean_browser(local_appdata, appdata);              break;
        case action::clean_roblox:  do_clean_roblox(userprofile);                          break;
        case action::full:          do_full(userprofile, local_appdata, appdata);          break;
        case action::quit:          break;
    }

    std::cout << "\n";
    console::pause_and_exit();
    return 0;
}

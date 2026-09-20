#include <includes.h>

namespace network {

namespace {

constexpr const char* net_class_key =
    "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}";

std::string reg_read_string(HKEY key, const char* value_name) {
    char buf[512];
    DWORD size = sizeof(buf);
    DWORD type = REG_SZ;
    if (RegQueryValueExA(key, value_name, nullptr, &type, reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS) {
        return std::string(buf);
    }
    return {};
}

std::string to_lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool is_virtual_adapter(const std::string& description) {
    const std::string lower = to_lower(description);
    static const char* skip_words[] = {
        "virtual", "loopback", "bluetooth", "wan miniport", "tap-windows", "pseudo"
    };
    for (const char* kw : skip_words) {
        if (lower.find(kw) != std::string::npos) return true;
    }
    return false;
}

std::string friendly_name(const std::string& netcfg_id, const std::string& fallback) {
    const std::string conn_path =
        "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" +
        netcfg_id + "\\Connection";

    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, conn_path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return fallback;
    }
    std::string name = reg_read_string(key, "Name");
    RegCloseKey(key);
    return name.empty() ? fallback : name;
}

std::string format_mac(const BYTE* bytes) {
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ss << ":";
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

std::string strip_braces(std::string s) {
    if (!s.empty() && s.front() == '{') s = s.substr(1);
    if (!s.empty() && s.back()  == '}') s = s.substr(0, s.size() - 1);
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

}

std::vector<adapter> list_adapters() {
    std::vector<adapter> out;

    HKEY class_key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, net_class_key, 0, KEY_READ, &class_key) != ERROR_SUCCESS) {
        logs::write(logs::level::error, "could not open network class registry key.");
        return out;
    }

    for (int i = 0; ; ++i) {
        std::ostringstream sub;
        sub << std::setfill('0') << std::setw(4) << i;

        HKEY adapter_key;
        if (RegOpenKeyExA(class_key, sub.str().c_str(), 0, KEY_READ, &adapter_key) != ERROR_SUCCESS) {
            break;
        }

        std::string desc      = reg_read_string(adapter_key, "DriverDesc");
        std::string netcfg_id = reg_read_string(adapter_key, "NetCfgInstanceID");
        RegCloseKey(adapter_key);

        if (desc.empty() || netcfg_id.empty()) continue;
        if (is_virtual_adapter(desc))          continue;

        out.push_back({ sub.str(), desc, friendly_name(netcfg_id, desc) });
    }

    RegCloseKey(class_key);
    return out;
}

std::string current_mac(const std::string& adapter_id) {
    const std::string reg_path = std::string(net_class_key) + "\\" + adapter_id;

    HKEY key;
    std::string netcfg_id;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS) {
        netcfg_id = reg_read_string(key, "NetCfgInstanceID");
        RegCloseKey(key);
    }
    if (netcfg_id.empty()) return "(unknown)";

    ULONG size = 15000;
    std::vector<BYTE> buf(size);
    if (GetAdaptersInfo(reinterpret_cast<IP_ADAPTER_INFO*>(buf.data()), &size) == ERROR_BUFFER_OVERFLOW) {
        buf.resize(size);
    }
    if (GetAdaptersInfo(reinterpret_cast<IP_ADAPTER_INFO*>(buf.data()), &size) != NO_ERROR) {
        return "(could not read)";
    }

    auto* it = reinterpret_cast<IP_ADAPTER_INFO*>(buf.data());
    const std::string want = strip_braces(netcfg_id);
    while (it) {
        if (strip_braces(it->AdapterName) == want) {
            return format_mac(it->Address);
        }
        it = it->Next;
    }
    return "(could not read)";
}

std::string random_mac() {
    std::mt19937 rng(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> byte_dist(0x00, 0xFF);

    std::ostringstream mac;
    mac << std::hex << std::uppercase << std::setfill('0');
    mac << std::setw(2) << 0x02;
    for (int i = 1; i < 6; ++i) {
        mac << std::setw(2) << byte_dist(rng);
    }
    return mac.str();
}

bool set_mac(const std::string& adapter_id, const std::string& mac) {
    const std::string path = std::string(net_class_key) + "\\" + adapter_id;
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_WRITE, &key) != ERROR_SUCCESS) {
        logs::write(logs::level::error, "failed to open adapter registry key for writing.");
        return false;
    }
    LONG rc = RegSetValueExA(key, "NetworkAddress", 0, REG_SZ,
                             reinterpret_cast<const BYTE*>(mac.c_str()),
                             static_cast<DWORD>(mac.size() + 1));
    RegCloseKey(key);
    if (rc != ERROR_SUCCESS) {
        logs::write(logs::level::error, "failed to write NetworkAddress.");
        return false;
    }
    return true;
}

bool restart_adapter(const std::string& connection_name) {
    const std::string disable_cmd = "netsh interface set interface \"" + connection_name + "\" admin=disable";
    if (std::system(disable_cmd.c_str()) != 0) {
        logs::write(logs::level::error, "failed to disable adapter.");
        return false;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    const std::string enable_cmd  = "netsh interface set interface \"" + connection_name + "\" admin=enable";
    if (std::system(enable_cmd.c_str()) != 0) {
        logs::write(logs::level::error, "failed to enable adapter.");
        return false;
    }
    return true;
}

void spoof_interactive() {
    auto adapters = list_adapters();
    if (adapters.empty()) {
        logs::write(logs::level::error, "no eligible network adapters found.");
        return;
    }

    logs::write(logs::level::info, "available network adapters:");
    for (size_t i = 0; i < adapters.size(); ++i) {
        const std::string mac = current_mac(adapters[i].id);
        std::cout << "  [" << (i + 1) << "] " << adapters[i].description
                  << " (" << adapters[i].connection_name << ")"
                  << " - mac: " << ANSI_YELLOW << mac << ANSI_RESET << "\n";
    }

    adapter* selected = nullptr;
    while (!selected) {
        logs::write(logs::level::info, "enter the number of the adapter to change:");
        std::string input;
        if (!std::getline(std::cin, input)) return;
        try {
            int n = std::stoi(input);
            if (n >= 1 && n <= static_cast<int>(adapters.size())) {
                selected = &adapters[n - 1];
                break;
            }
        } catch (...) {}
        logs::write(logs::level::error, "invalid selection.");
    }

    const std::string old_mac = current_mac(selected->id);
    const std::string new_mac = random_mac();

    if (!set_mac(selected->id, new_mac))                     return;
    if (!restart_adapter(selected->connection_name))         return;

    logs::write(logs::level::success, "mac spoofed.");
    logs::write(logs::level::info,    "old: " + old_mac);
    logs::write(logs::level::info,    "new: " + current_mac(selected->id));
}

}

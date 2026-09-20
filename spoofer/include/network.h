#pragma once

#include <string>
#include <vector>

namespace network {

struct adapter {
    std::string id;
    std::string description;
    std::string connection_name;
};

std::vector<adapter> list_adapters();
std::string          current_mac(const std::string& adapter_id);
std::string          random_mac();
bool                 set_mac(const std::string& adapter_id, const std::string& mac);
bool                 restart_adapter(const std::string& connection_name);
void                 spoof_interactive();

}

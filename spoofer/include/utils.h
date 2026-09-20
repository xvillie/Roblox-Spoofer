#pragma once

#include <string>

namespace utils {

bool is_elevated();
bool relaunch_as_admin();
std::string env_or_empty(const char* name);

}

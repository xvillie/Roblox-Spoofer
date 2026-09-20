#pragma once

#include <string>

namespace logs {

enum class level {
    info,
    error,
    success,
    warn,
};

void write(level lvl, const std::string& message);
void clear_screen();

}

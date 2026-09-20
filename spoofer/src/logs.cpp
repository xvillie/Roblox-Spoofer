#include <includes.h>

namespace logs {

namespace {

void set_color(WORD c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void reset_color() {
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

struct label_style {
    const char* text;
    WORD        color;
};

label_style style_for(level lvl) {
    switch (lvl) {
        case level::info:    return { "info",    FOREGROUND_BLUE  | FOREGROUND_INTENSITY };
        case level::error:   return { "error",   FOREGROUND_RED   | FOREGROUND_INTENSITY };
        case level::success: return { "success", FOREGROUND_GREEN | FOREGROUND_INTENSITY };
        case level::warn:    return { "warn",    FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY };
    }
    return { "log", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
}

}

void write(level lvl, const std::string& message) {
    auto s = style_for(lvl);
    std::cout << "[";
    set_color(s.color);
    std::cout << s.text;
    reset_color();
    std::cout << "] " << message << '\n';
}

void clear_screen() {
    std::system("cls");
}

}

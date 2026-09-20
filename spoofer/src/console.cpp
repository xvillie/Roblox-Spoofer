#include <includes.h>

namespace console {

void enable_ansi() {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

int terminal_width() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
    return 80;
}

void print_title(const std::string& title) {
    const int width = terminal_width();
    const int marker_len = 4;
    const int total = static_cast<int>(title.size()) + marker_len;

    if (total < width) {
        int pad = (width - total) / 2;
        std::cout << std::string(pad, ' ');
    }
    std::cout << ANSI_BLUE << "[?]" << ANSI_RESET << " " << title << "\n\n";
}

void pause_and_exit() {
    logs::write(logs::level::info, "press enter to exit...");
    std::cin.get();
    std::exit(0);
}

}

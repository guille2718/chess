#include "term_utils.h"

#include <iostream>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace terminal {


void ClearScreen() {
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        for (int i = 0; i < ws.ws_row; ++i) {
            std::cout << '\n';
        }
        // Reset cursor to the top
        std::cout << "\033[1;1H";
    }
}

}  // namespace terminal

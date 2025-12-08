#include "Arena.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    Arena arena;

    bool watch_live = false;
    bool fast_mode   = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-m" || arg == "--manual") {
            watch_live = true;
        }
        else if (arg == "-f" || arg == "--fast") {
            fast_mode = true;
        }
        else {
            std::cout << "Unknown option: " << arg << "\n";
            std::cout << "Usage: ./RobotWarz [-m] [-f]\n";
            return 1;
        }
    }

    // Load config (defaults if missing)
    arena.load_config("config.txt");

    if (watch_live) {
        std::cout << "Live mode enabled.\n";
        arena.set_watch_live(true);    
    }

    if (fast_mode) {
        std::cout << "Fast mode enabled.\n";
        arena.set_watch_live(false);    
        arena.set_fast_mode(true);
    }

    arena.load_obstacles();
    arena.load_robots();

    std::cout << "\nStarting RobotWarz simulation...\n\n";

    arena.run();

    std::cout << "\nSimulation finished.\n";
    return 0;
}
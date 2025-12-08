#include "Arena.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <dirent.h>
#include <dlfcn.h>

Arena::Arena()
    : rows(20),
      cols(20),
      max_rounds(99),
      num_mounds(10),
      num_pits(5),
      num_flames(5) {
    init_board();
}

// Format (one line, 7 ints):
// rows cols num_mounds num_pits num_flames max_rounds 
bool Arena::load_config(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cout << "Config file '" << filename
                  << "' not found. Using default settings.\n";
        return false;
    }

    fin >> rows >> cols
        >> num_mounds >> num_pits >> num_flames
        >> max_rounds >> watch_live;

    if (rows < 10) rows = 10;
    if (cols < 10) cols = 10;

    init_board();
    std::cout << "Loaded config: " << rows << "x" << cols
              << ", Mounds=" << num_mounds
              << ", Pits=" << num_pits
              << ", Flames=" << num_flames
              << ", Rounds=" << max_rounds << "\n";
    return true;
}

void Arena::init_board() {
    board.assign(rows, std::vector<char>(cols, '.'));
}

void Arena::load_obstacles() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    auto place_some = [&](char ch, int count) {
        int placed = 0;
        while (placed < count) {
            int r = std::rand() % rows;
            int c = std::rand() % cols;
            if (board[r][c] == '.') {
                board[r][c] = ch;
                placed++;
            }
        }
    };

    place_some('M', num_mounds);
    place_some('P', num_pits);
    place_some('F', num_flames);
}

void Arena::load_robots() {
    DIR* dir = opendir(".");
    if (!dir) {
        std::cerr << "Could not open current directory.\n";
        return;
    }

    const char* prefix = "Robot_";
    const char* suffix = ".cpp";

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.rfind(prefix, 0) != 0) {
            continue; 
        }
        if (filename.size() < 9) {
            continue;
        }
        if (filename.substr(filename.size() - 4) != suffix) {
            continue;
        }

        std::string core = filename.substr(6, filename.size() - 6 - 4); 
        std::string shared_lib = "lib" + core + ".so";

        std::string compile_cmd =
            "g++ -shared -fPIC -o " + shared_lib + " " + filename +
            " RobotBase.o -I. -std=c++20";
        std::cout << "Compiling " << filename << " to " << shared_lib << "...\n";

        int result = std::system(compile_cmd.c_str());
        if (result != 0) {
            std::cerr << "  Failed to compile " << filename << "\n";
            continue;
        }

        void* handle = dlopen(shared_lib.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "  Failed to load " << shared_lib << ": "
                      << dlerror() << "\n";
            continue;
        }

        RobotFactory create_robot =
            (RobotFactory)dlsym(handle, "create_robot");
        if (!create_robot) {
            std::cerr << "  Failed to find create_robot in "
                      << shared_lib << ": " << dlerror() << "\n";
            dlclose(handle);
            continue;
        }

        RobotBase* robot = create_robot();
        if (!robot) {
            std::cerr << "  create_robot failed.\n";
            dlclose(handle);
            continue;
        }

		if (robot->m_name == "Blank_Robot" || robot->m_name.empty()) {
			robot->m_name = core;  
		}

		static const char symbols[] = { '!', '@', '#', '$', '%', '&', '*', '+', '?', '~' };
		static int next_symbol_index = 0;

		char symbol;
		if (next_symbol_index < (int)(sizeof(symbols) / sizeof(symbols[0]))) {
			symbol = symbols[next_symbol_index++];
		} else {
			symbol = !robot->m_name.empty() ? robot->m_name[0] : '?';
		}

		if (robot->m_character == '\0') {
			robot->m_character = symbol;
		}

        int r, c;
        while (true) {
            r = std::rand() % rows;
            c = std::rand() % cols;

            if (board[r][c] != '.') continue;
            if (find_robot_at(r, c) != -1) continue;
            break;
        }

        robot->set_boundaries(rows, cols);
        robot->move_to(r, c);

        RobotInfo info;
        info.robot  = robot;
        info.symbol = robot->m_character; 
        info.row    = r;
        info.col    = c;
        info.alive  = true;
        info.handle = handle;

        robots.push_back(info);

        std::cout << "Loaded robot: " << robot->m_name
                  << " at (" << r << "," << c << ")\n";
    }

    closedir(dir);
}

bool Arena::in_bounds(int r, int c) const {
    return r >= 0 && r < rows && c >= 0 && c < cols;
}

char Arena::get_cell_type(int r, int c) const {
    for (const auto& info : robots) {
        if (info.row == r && info.col == c) {
            return info.alive ? 'R' : 'X';
        }
    }
    if (!in_bounds(r, c)) return '.';
    return board[r][c];
}

int Arena::find_robot_at(int r, int c) const {
    for (std::size_t i = 0; i < robots.size(); ++i) {
        if (robots[i].row == r && robots[i].col == c) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Arena::print_board(int round) const 
{
    if (watch_live) {
        std::cout << "\033[H\033[J";
    }

    std::cout << "=========== starting round " << round << " ===========\n\n";

    std::cout << "    ";
    for (int c = 0; c < cols; ++c) {
        if (c < 10) std::cout << " " << c << " ";
        else        std::cout << c << " ";
    }
    std::cout << "\n";

    for (int r = 0; r < rows; ++r) {
        if (r < 10) std::cout << " ";
        std::cout << r << " ";

        for (int c = 0; c < cols; ++c) {
            int idx = find_robot_at(r, c);
            if (idx != -1) {
                const RobotInfo& info = robots[idx];

                if (info.alive)
                    std::cout << "R" << info.symbol << " ";
                else
                    std::cout << "X" << info.symbol << " ";
            } else {
                std::cout << " " << board[r][c] << " ";
            }
        }
        std::cout << "\n\n";
    }

    for (const auto& info : robots) {
        std::cout << info.robot->print_stats();
        if (!info.alive) std::cout << "  (DEAD)";
        std::cout << "\n";
    }
    std::cout << "\n";
}

void Arena::update_board() {
}

bool Arena::check_for_winner() const {
    int alive_count = 0;
    const RobotInfo* last = nullptr;

    for (const auto& info : robots) {
        if (info.alive && info.robot->get_health() > 0) {
            alive_count++;
            last = &info;
        }
    }

    if (alive_count <= 1) {
        if (alive_count == 1 && last != nullptr) {
            std::cout << "Winner: " << last->robot->m_name << "!\n";
        } else {
            std::cout << "Nobody survived. It's a draw.\n";
        }
        return true;
    }
    return false;
}

void Arena::run() {
    if (robots.empty()) {
        std::cout << "No robots loaded. Nothing to do.\n";
        return;
    }

    for (int round = 0; round < max_rounds; ++round) {
        play_round(round);
        if (check_for_winner()) {
            return;
        }
    }
    std::cout << "Reached max rounds with multiple robots alive.\n";
}

void Arena::play_round(int round) {
    print_board(round);

    for (std::size_t i = 0; i < robots.size(); ++i) {
        RobotInfo& info = robots[i];

        if (!info.alive || info.robot->get_health() <= 0) {
            info.alive = false;
            continue;
        }

        handle_robot_turn(info);

        if (watch_live) {
            std::cout << "Press ENTER for next robot...\n";
            std::cin.get();
        }
    }
}

void Arena::handle_robot_turn(RobotInfo& info) {
    std::cout << info.robot->m_name << " " << info.symbol
              << " begins turn.\n";

    int radar_dir = 0;
    info.robot->get_radar_direction(radar_dir);

    if (radar_dir < 0 || radar_dir > 8) radar_dir = 0;

    std::vector<RadarObj> radar_results;
    do_radar_scan(info, radar_dir, radar_results);

    info.robot->process_radar_results(radar_results);

    int shot_row = 0;
    int shot_col = 0;
    bool wants_to_shoot = info.robot->get_shot_location(shot_row, shot_col);

    if (wants_to_shoot) {
        handle_shot(info, shot_row, shot_col);
    } else {
        int move_dir = 0;
        int move_dist = 0;
        info.robot->get_move_direction(move_dir, move_dist);
        handle_movement(info, move_dir, move_dist);
    }

    update_board();
}

void Arena::do_radar_scan(RobotInfo& info,
                          int radar_dir,
                          std::vector<RadarObj>& radar_results) {
    radar_results.clear();

    int r = info.row;
    int c = info.col;

    if (radar_dir == 0) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                int rr = r + dr;
                int cc = c + dc;
                if (dr == 0 && dc == 0) continue;
                if (!in_bounds(rr, cc)) continue;

                char t = get_cell_type(rr, cc);
                if (t != '.' ) {
                    radar_results.push_back(RadarObj(t, rr, cc));
                }
            }
        }
    } else {
        int dr = directions[radar_dir].first;
        int dc = directions[radar_dir].second;

        int step = 1;
        while (true) {
            int base_r = r + dr * step;
            int base_c = c + dc * step;
            if (!in_bounds(base_r, base_c)) break;

            if (dr != 0 && dc == 0) {
                for (int offset = -1; offset <= 1; ++offset) {
                    int rr = base_r;
                    int cc = base_c + offset;
                    if (!in_bounds(rr, cc)) continue;
                    char t = get_cell_type(rr, cc);
                    if (t != '.' && !(rr == r && cc == c)) {
                        radar_results.push_back(RadarObj(t, rr, cc));
                    }
                }
            } else if (dr == 0 && dc != 0) {
                for (int offset = -1; offset <= 1; ++offset) {
                    int rr = base_r + offset;
                    int cc = base_c;
                    if (!in_bounds(rr, cc)) continue;
                    char t = get_cell_type(rr, cc);
                    if (t != '.' && !(rr == r && cc == c)) {
                        radar_results.push_back(RadarObj(t, rr, cc));
                    }
                }
            } else {
                int rr = base_r;
                int cc = base_c;
                if (in_bounds(rr, cc)) {
                    char t = get_cell_type(rr, cc);
                    if (t != '.' && !(rr == r && cc == c)) {
                        radar_results.push_back(RadarObj(t, rr, cc));
                    }
                }
            }

            step++;
        }
    }

    if (radar_results.empty()) {
        std::cout << "  radar found nothing.\n";
    } else {
        std::cout << "  radar found " << radar_results.size() << " objects.\n";
    }
}

void Arena::handle_movement(RobotInfo& mover, int move_dir, int move_dist) {
    int max_speed = mover.robot->get_move_speed();
    if (max_speed <= 0) {
        std::cout << "  " << mover.robot->m_name
                  << " is stuck and cannot move.\n";
        return;
    }

    if (move_dir < 1 || move_dir > 8) {
        std::cout << "  invalid move direction.\n";
        return;
    }

    if (move_dist <= 0) {
        std::cout << "  chose not to move.\n";
        return;
    }

    if (move_dist > max_speed) {
        move_dist = max_speed; 
    }

    int dr = directions[move_dir].first;
    int dc = directions[move_dir].second;

    int row_1 = mover.row;
    int col_1 = mover.col;
    int row_2 = row_1 + dr * move_dist;
    int col_2 = col_1 + dc * move_dist;

    if (row_2 < 0) row_2 = 0;
    if (row_2 >= rows) row_2 = rows - 1;
    if (col_2 < 0) col_2 = 0;
    if (col_2 >= cols) col_2 = cols - 1;

    double delta_r = row_2 - row_1;
    double delta_c = col_2 - col_1;

    int steps = std::max(std::abs((int)delta_r), std::abs((int)delta_c));
    if (steps == 0) {
        std::cout << "  " << mover.robot->m_name
                  << " ends move at (" << mover.row << "," << mover.col << ").\n";
        return;
    }

    double row_increment = delta_r / steps;
    double col_increment = delta_c / steps;

    double row_counter = row_1;
    double col_counter = col_1;

    int last_r = row_1;
    int last_c = col_1;

    for (int i = 0; i < steps; ++i) {
        row_counter += row_increment;
        col_counter += col_increment;

        int r = std::round(row_counter);
        int c = std::round(col_counter);

        if (r == last_r && c == last_c) {
            continue;
        }
        last_r = r;
        last_c = c;

        if (!in_bounds(r, c)) {
            break;
        }

        char cell = get_cell_type(r, c);

        if (cell == 'R' || cell == 'X' || cell == 'M') {
            break;
        }

        if (board[r][c] == 'P') {
            mover.row = r;
            mover.col = c;
            mover.robot->move_to(r, c);
            mover.robot->disable_movement();
            std::cout << "  " << mover.robot->m_name
                      << " fell into a pit at (" << r << "," << c << ").\n";
            return;
        }

        if (board[r][c] == 'F') {
            mover.row = r;
            mover.col = c;
            mover.robot->move_to(r, c);
            std::cout << "  " << mover.robot->m_name
                      << " moves through flames at (" << r << "," << c << ").\n";
            apply_damage(mover, 30, 50);
            if (!mover.alive) {
                return;
            }
            continue;
        }

        mover.row = r;
        mover.col = c;
        mover.robot->move_to(r, c);
    }

    std::cout << "  " << mover.robot->m_name << " ends move at ("
              << mover.row << "," << mover.col << ").\n";
}

void Arena::handle_shot(RobotInfo& shooter, int shot_row, int shot_col) {
    if (!in_bounds(shot_row, shot_col)) {
        std::cout << "  Shot location is out of bounds; ignoring.\n";
        return;
    }

    WeaponType w = shooter.robot->get_weapon();

    std::cout << "  " << shooter.robot->m_name << " fires ";

    if (w == railgun) {
        std::cout << "railgun.\n";
        railgun_line(shooter, shot_row, shot_col);
    } else if (w == flamethrower) {
        std::cout << "flamethrower.\n";
        flamethrower_cone(shooter, shot_row, shot_col);
    } else if (w == grenade) {
        std::cout << "grenade.\n";
        if (shooter.robot->get_grenades() <= 0) {
            std::cout << "  But has no grenades left!\n";
            return;
        }
        shooter.robot->decrement_grenades();

        for (int r = shot_row - 1; r <= shot_row + 1; ++r) {
            for (int c = shot_col - 1; c <= shot_col + 1; ++c) {
                if (!in_bounds(r, c)) continue;
                int idx = find_robot_at(r, c);
                if (idx != -1 && robots[idx].alive &&
                    robots[idx].robot != shooter.robot) {
                    apply_damage(robots[idx], 10, 40);
                }
            }
        }
    } else if (w == hammer) {
        std::cout << "hammer.\n";
        if (std::abs(shot_row - shooter.row) <= 1 &&
            std::abs(shot_col - shooter.col) <= 1) {
            int idx = find_robot_at(shot_row, shot_col);
            if (idx != -1 && robots[idx].alive &&
                robots[idx].robot != shooter.robot) {
                apply_damage(robots[idx], 50, 60);
            } else {
                std::cout << "  Nothing there to hammer.\n";
            }
        } else {
            std::cout << "  Hammer target not adjacent.\n";
        }
    }
}

void Arena::railgun_line(RobotInfo& shooter,
                         int target_row,
                         int target_col) {
    int start_r = shooter.row;
    int start_c = shooter.col;

    double delta_r = static_cast<double>(target_row - start_r);
    double delta_c = static_cast<double>(target_col - start_c);

    int steps = std::max(std::abs(static_cast<int>(delta_r)),
                         std::abs(static_cast<int>(delta_c)));
    if (steps == 0) return;

    double row_inc = delta_r / steps;
    double col_inc = delta_c / steps;

    double row = start_r + row_inc;
    double col = start_c + col_inc;

    int last_r = -1;
    int last_c = -1;

    while (true) {
        int rr = static_cast<int>(std::round(row));
        int cc = static_cast<int>(std::round(col));

        if (!in_bounds(rr, cc)) break;

        if (rr != last_r || cc != last_c) {
            int idx = find_robot_at(rr, cc);
            if (idx != -1 &&
                robots[idx].alive &&
                robots[idx].robot != shooter.robot) {
                apply_damage(robots[idx], 10, 20); 
            }
            last_r = rr;
            last_c = cc;
        }

        row += row_inc;
        col += col_inc;
    }
}

void Arena::flamethrower_cone(RobotInfo& shooter,
                              int target_row,
                              int target_col) {
    int start_r = shooter.row;
    int start_c = shooter.col;

    double delta_r = static_cast<double>(target_row - start_r);
    double delta_c = static_cast<double>(target_col - start_c);

    int steps = std::max(std::abs(static_cast<int>(delta_r)),
                         std::abs(static_cast<int>(delta_c)));
    if (steps == 0) return;

    double row_inc = delta_r / steps;
    double col_inc = delta_c / steps;

    double row = start_r;
    double col = start_c;

    for (int i = 0; i < 4; ++i) { 
        row += row_inc;
        col += col_inc;

        int rr = static_cast<int>(std::round(row));
        int cc = static_cast<int>(std::round(col));
        if (!in_bounds(rr, cc)) break;

        if (std::abs(delta_r) >= std::abs(delta_c)) {
            for (int offset = -1; offset <= 1; ++offset) {
                int r2 = rr;
                int c2 = cc + offset;
                if (!in_bounds(r2, c2)) continue;
                int idx = find_robot_at(r2, c2);
                if (idx != -1 &&
                    robots[idx].alive &&
                    robots[idx].robot != shooter.robot) {
                    apply_damage(robots[idx], 30, 50);
                }
            }
        } else {
            for (int offset = -1; offset <= 1; ++offset) {
                int r2 = rr + offset;
                int c2 = cc;
                if (!in_bounds(r2, c2)) continue;
                int idx = find_robot_at(r2, c2);
                if (idx != -1 &&
                    robots[idx].alive &&
                    robots[idx].robot != shooter.robot) {
                    apply_damage(robots[idx], 30, 50);
                }
            }
        }
    }
}

void Arena::apply_damage(RobotInfo& target,
                         int min_dmg,
                         int max_dmg) {
    if (!target.alive) return;

    int base = min_dmg;
    if (max_dmg > min_dmg) {
        base += std::rand() % (max_dmg - min_dmg + 1);
    }

    int armor = target.robot->get_armor();
    double reduction = 0.1 * armor;
    if (reduction > 0.9) reduction = 0.9;

    int final_dmg =
        static_cast<int>(std::round(base * (1.0 - reduction)));
    if (final_dmg < 0) final_dmg = 0;

    int before = target.robot->get_health();
    target.robot->reduce_armor(1);
    int after = target.robot->take_damage(final_dmg);

    std::cout << "  " << target.robot->m_name
              << " takes " << final_dmg
              << " damage (health " << before
              << " -> " << after << ").\n";

    if (after <= 0) {
        target.alive = false;
        std::cout << "  " << target.robot->m_name << " is destroyed!\n";
    }
}
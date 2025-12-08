#pragma once

#include <vector>
#include <string>
#include <utility>

#include "RobotBase.h"
#include "RadarObj.h"

struct RobotInfo {
    RobotBase* robot;   
    char symbol;        
    int row;
    int col;
    bool alive;
    void* handle;       

    RobotInfo()
        : robot(nullptr), symbol('!'), row(0), col(0), alive(true), handle(nullptr) {}
};

class Arena {
public:
    Arena();
    bool load_config(const std::string& filename);
    void load_obstacles();
    void load_robots();
    void run();
	void set_watch_live(bool v) { watch_live = v; }
	void set_fast_mode(bool v) { fast_mode = v; }

private:
	bool watch_live = false;
	bool fast_mode = false;

    int rows;
    int cols;
    int max_rounds;

    int num_mounds;
    int num_pits;
    int num_flames;

    std::vector<std::vector<char>> board;  

    std::vector<RobotInfo> robots;

    void init_board();
    void print_board(int round) const;
    void update_board();   

    bool check_for_winner() const;

    void play_round(int round);
    void handle_robot_turn(RobotInfo& info);

    void do_radar_scan(RobotInfo& info, int radar_dir, std::vector<RadarObj>& radar_results);
    char get_cell_type(int r, int c) const; 
    int find_robot_at(int r, int c) const;  
    bool in_bounds(int r, int c) const;
    void handle_movement(RobotInfo& mover, int move_dir, int move_dist);
    void handle_shot(RobotInfo& shooter, int shot_row, int shot_col);
    void apply_damage(RobotInfo& target, int min_dmg, int max_dmg);
    void railgun_line(RobotInfo& shooter, int target_row, int target_col);
    void flamethrower_cone(RobotInfo& shooter, int target_row, int target_col);
};
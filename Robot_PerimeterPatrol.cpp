#include "RobotBase.h"
#include <vector>
#include <cmath>  

class Robot_PerimeterPatrol : public RobotBase {
private:
    bool has_target;
    int target_row;
    int target_col;

    //simple Manhattan distance
    int manhattan_distance(int r1, int c1, int r2, int c2) const {
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

public:
    Robot_PerimeterPatrol()
        : RobotBase(4, 3, flamethrower),
          has_target(false),
          target_row(-1),
          target_col(-1)
    {
        m_name = "PerimeterPatrol";
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0; 
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        has_target = false;
        target_row = -1;
        target_col = -1;

        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') { 
                target_row = obj.m_row;
                target_col = obj.m_col;
                has_target = true;
                break; 
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (!has_target) {
            return false;
        }

        int current_row, current_col;
        get_current_location(current_row, current_col);

        int dist = manhattan_distance(current_row, current_col, target_row, target_col);

        if (dist <= 4) {
            shot_row = target_row;
            shot_col = target_col;
            return true;
        }

        return false;
    }

    void get_move_direction(int& move_direction, int& move_distance) override {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        int max_move = get_move_speed();

        int last_row = m_board_row_max - 1;
        int last_col = m_board_col_max - 1;

        move_direction = 0;
        move_distance = 0;

        // Top row (not at right edge): move right
        if (current_row == 0 && current_col < last_col) {
            move_direction = 3; // Right
            int steps_to_edge = last_col - current_col;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        // Right column (not at bottom edge): move down
        else if (current_col == last_col && current_row < last_row) {
            move_direction = 5; // Down
            int steps_to_edge = last_row - current_row;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        // Bottom row (not at left edge): move left
        else if (current_row == last_row && current_col > 0) {
            move_direction = 7; // Left
            int steps_to_edge = current_col;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        // Left column (not at top edge): move up
        else if (current_col == 0 && current_row > 0) {
            move_direction = 1; // Up
            int steps_to_edge = current_row;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        else {
            
            int dist_to_top    = current_row;
            int dist_to_bottom = last_row - current_row;
            int dist_to_left   = current_col;
            int dist_to_right  = last_col - current_col;

            int min_dist = dist_to_top;
            char best = 'U'; 

            if (dist_to_bottom < min_dist) {
                min_dist = dist_to_bottom;
                best = 'D';
            }
            if (dist_to_left < min_dist) {
                min_dist = dist_to_left;
                best = 'L';
            }
            if (dist_to_right < min_dist) {
                min_dist = dist_to_right;
                best = 'R';
            }

            if (best == 'U' && min_dist > 0) {
                move_direction = 1;
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'D' && min_dist > 0) {
                move_direction = 5; 
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'L' && min_dist > 0) {
                move_direction = 7; 
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'R' && min_dist > 0) {
                move_direction = 3; 
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else {
                move_direction = 0;
                move_distance = 0;
            }
        }
    }
};

// Factory function so the arena can create this robot from the .so
extern "C" RobotBase* create_robot() {
    return new Robot_PerimeterPatrol();
}
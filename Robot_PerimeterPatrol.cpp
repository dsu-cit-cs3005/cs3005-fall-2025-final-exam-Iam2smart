#include "RobotBase.h"
#include <cstdlib>
#include <ctime>
#include <set>
#include <cmath>
#include <limits>
#include <utility>

class Robot_PerimeterFlame : public RobotBase 
{
private:
    bool target_found = false;
    int target_row = -1;
    int target_col = -1;

    int radar_direction = 1;
    bool fixed_radar = false;
    const int max_range = 4;
    std::set<std::pair<int, int>> obstacles_memory;

    int calculate_distance(int row1, int col1, int row2, int col2) const 
    {
        return std::abs(row1 - row2) + std::abs(col1 - col2);
    }

    void find_closest_enemy(const std::vector<RadarObj>& radar_results, int current_row, int current_col) 
    {
        target_found = false;
        int closest_distance = std::numeric_limits<int>::max();

        for (const auto& obj : radar_results) 
        {
            if (obj.m_type == 'R')
            {
                int distance = calculate_distance(current_row, current_col, obj.m_row, obj.m_col);
                if (distance <= max_range && distance < closest_distance) 
                {
                    closest_distance = distance;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                    target_found = true;
                    fixed_radar = true;
                }
            }
        }
    }

    void update_obstacle_memory(const std::vector<RadarObj>& radar_results) 
    {
        for (const auto& obj : radar_results) 
        {
            if (obj.m_type == 'M' || obj.m_type == 'P' || obj.m_type == 'F') 
            {
                obstacles_memory.insert({obj.m_row, obj.m_col});
            }
        }
    }

    bool is_passable(int row, int col) const 
    {
        return obstacles_memory.find({row, col}) == obstacles_memory.end();
    }

public:
    Robot_PerimeterFlame() : RobotBase(3, 4, flamethrower) 
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        m_name = "PerimeterFlame";
    }

    virtual void get_radar_direction(int& radar_direction_out) override 
    {
        if (fixed_radar && target_found) 
        {
            radar_direction_out = radar_direction;
        } 
        else 
        {
            radar_direction_out = radar_direction;
            radar_direction = (radar_direction % 8) + 1;
        }
    }

    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override 
    {
        target_found = false;
        int current_row, current_col;
        get_current_location(current_row, current_col);

        update_obstacle_memory(radar_results);
        find_closest_enemy(radar_results, current_row, current_col);

        if (!target_found) 
        {
            fixed_radar = false;
        }
    }

    virtual bool get_shot_location(int& shot_row, int& shot_col) override 
    {
        if (target_found) 
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            if (calculate_distance(current_row, current_col, target_row, target_col) <= max_range) 
            {
                shot_row = target_row;
                shot_col = target_col;
                return true;
            } 
            else 
            {
                target_found = false;
                fixed_radar = false;
            }
        }

        return false;
    }

    virtual void get_move_direction(int& move_direction, int& move_distance) override 
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        if (target_found) 
        {
            int row_step = (target_row > current_row) ? 1 : (target_row < current_row) ? -1 : 0;
            int col_step = (target_col > current_col) ? 1 : (target_col < current_col) ? -1 : 0;

            if (row_step != 0 && is_passable(current_row + row_step, current_col)) 
            {
                move_direction = (row_step > 0) ? 5 : 1;
                move_distance = 1;
            } 
            else if (col_step != 0 && is_passable(current_row, current_col + col_step)) 
            {
                move_direction = (col_step > 0) ? 3 : 7;
                move_distance = 1;
            } 
            else 
            {
                move_direction = 0;
                move_distance = 0;
            }

            return;
        }

        int max_move = get_move_speed();
        int last_row = m_board_row_max - 1;
        int last_col = m_board_col_max - 1;

        if (current_row == 0 && current_col < last_col) 
        {
            move_direction = 3;
            int steps_to_edge = last_col - current_col;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        else if (current_col == last_col && current_row < last_row) 
        {
            move_direction = 5;
            int steps_to_edge = last_row - current_row;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        else if (current_row == last_row && current_col > 0) 
        {
            move_direction = 7;
            int steps_to_edge = current_col;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        else if (current_col == 0 && current_row > 0) 
        {
            move_direction = 1;
            int steps_to_edge = current_row;
            move_distance = (steps_to_edge < max_move) ? steps_to_edge : max_move;
        }
        else 
        {
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

            if (best == 'U' && min_dist > 0) 
            {
                move_direction = 1;
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'D' && min_dist > 0) 
            {
                move_direction = 5;
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'L' && min_dist > 0) 
            {
                move_direction = 7;
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else if (best == 'R' && min_dist > 0) 
            {
                move_direction = 3;
                move_distance = (min_dist < max_move) ? min_dist : max_move;
            }
            else 
            {
                move_direction = 0;
                move_distance = 0;
            }
        }
    }
};

extern "C" RobotBase* create_robot() 
{
    return new Robot_PerimeterFlame();
}
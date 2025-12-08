#include "RobotBase.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <limits>

class Robot_TankBuddy : public RobotBase
{
private:
    bool m_has_target;
    int m_target_row;
    int m_target_col;

    int manhattan(int r1, int c1, int r2, int c2) const
    {
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

public:
    // Move 2, Armor 5, Hammer
    Robot_TankBuddy() : RobotBase(2, 5, hammer),
                        m_has_target(false),
                        m_target_row(-1),
                        m_target_col(-1)
    {
        m_name = "TankBuddy";
        m_character = 'T';
    }

    // Look around in all 8 neighboring squares (radar 0)
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = 0;
    }

    // Find the closest robot in radar_results
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        int cur_row, cur_col;
        get_current_location(cur_row, cur_col);

        m_has_target = false;
        int best_dist = std::numeric_limits<int>::max();

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R') // live robot
            {
                int d = manhattan(cur_row, cur_col, obj.m_row, obj.m_col);
                if (d < best_dist)
                {
                    best_dist   = d;
                    m_target_row = obj.m_row;
                    m_target_col = obj.m_col;
                    m_has_target = true;
                }
            }
        }
    }

    // Only shoot (hammer) if target is adjacent
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (!m_has_target) return false;

        int cur_row, cur_col;
        get_current_location(cur_row, cur_col);

        if (std::abs(cur_row - m_target_row) <= 1 &&
            std::abs(cur_col - m_target_col) <= 1)
        {
            shot_row = m_target_row;
            shot_col = m_target_col;
            return true;
        }

        return false;
    }

    // Move closer to target, simple “step towards” logic
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int cur_row, cur_col;
        get_current_location(cur_row, cur_col);

        int max_move = get_move_speed();
        if (max_move <= 0)
        {
            move_direction = 0;
            move_distance = 0;
            return;
        }

        if (!m_has_target)
        {
            // No known target – just wander slowly up/down
            move_direction = (cur_row < m_board_row_max / 2 ? 5 : 1); // down / up
            move_distance = 1;
            return;
        }

        int dr = 0;
        int dc = 0;

        if (m_target_row > cur_row) dr = 1;
        else if (m_target_row < cur_row) dr = -1;

        if (m_target_col > cur_col) dc = 1;
        else if (m_target_col < cur_col) dc = -1;

        // Choose a primary direction: prefer vertical if farther vertically
        if (std::abs(m_target_row - cur_row) >= std::abs(m_target_col - cur_col))
        {
            // Move up or down
            if (dr > 0) move_direction = 5; // down
            else if (dr < 0) move_direction = 1; // up
            else if (dc > 0) move_direction = 3; // right
            else if (dc < 0) move_direction = 7; // left
            else move_direction = 0;
        }
        else
        {
            // Move left or right
            if (dc > 0) move_direction = 3; // right
            else if (dc < 0) move_direction = 7; // left
            else if (dr > 0) move_direction = 5; // down
            else if (dr < 0) move_direction = 1; // up
            else move_direction = 0;
        }

        if (move_direction == 0)
        {
            move_distance = 0;
        }
        else
        {
            move_distance = 1; // single step, let Arena handle obstacles
        }
    }
};

// Factory
extern "C" RobotBase* create_robot()
{
    return new Robot_TankBuddy();
}
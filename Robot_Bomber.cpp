#include "RobotBase.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

class Robot_Bomber : public RobotBase
{
private:
    bool m_has_target;
    int m_target_row;
    int m_target_col;

public:
    // Move 3, Armor 3, Grenade launcher
    Robot_Bomber() : RobotBase(3, 3, grenade),
                     m_has_target(false),
                     m_target_row(-1),
                     m_target_col(-1)
    {
        m_name = "Bomber";
        m_character = 'B';
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    // Look around locally
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = 0; // surrounding cells
    }

    // Find "center" of nearby robots
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        int sum_r = 0;
        int sum_c = 0;
        int count = 0;

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                sum_r += obj.m_row;
                sum_c += obj.m_col;
                count++;
            }
        }

        if (count > 0)
        {
            m_has_target  = true;
            m_target_row = sum_r / count;
            m_target_col = sum_c / count;
        }
        else
        {
            m_has_target = false;
        }
    }

    // Shoot grenades at the cluster center if we have any left
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (!m_has_target) return false;

        if (get_grenades() <= 0)
        {
            // No ammo – can't shoot
            return false;
        }

        shot_row = m_target_row;
        shot_col = m_target_col;
        // Arena will call decrement_grenades() when handling the shot.
        return true;
    }

    // Move randomly when no target; otherwise move a bit toward target
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
            // wander randomly
            move_direction = (std::rand() % 8) + 1; // 1..8
            move_distance = 1;
            return;
        }

        // Move gently toward target
        int dr = 0;
        int dc = 0;

        if (m_target_row > cur_row) dr = 1;
        else if (m_target_row < cur_row) dr = -1;

        if (m_target_col > cur_col) dc = 1;
        else if (m_target_col < cur_col) dc = -1;

        // pick one of the primary directions (up/down/left/right)
        if (std::abs(m_target_row - cur_row) >= std::abs(m_target_col - cur_col))
        {
            if (dr > 0) move_direction = 5; // down
            else if (dr < 0) move_direction = 1; // up
            else if (dc > 0) move_direction = 3; // right
            else if (dc < 0) move_direction = 7; // left
            else move_direction = 0;
        }
        else
        {
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
            move_distance = 1;
        }
    }
};

// Factory
extern "C" RobotBase* create_robot()
{
    return new Robot_Bomber();
}
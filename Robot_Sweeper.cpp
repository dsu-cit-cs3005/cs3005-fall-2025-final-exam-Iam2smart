#include "RobotBase.h"
#include <vector>
#include <cstdlib>
#include <cmath>

class Robot_Sweeper : public RobotBase
{
private:
    int m_radar_dir;          // 3 = right, 7 = left
    int m_target_row;
    int m_target_col;
    bool m_has_target;

public:
    Robot_Sweeper() : RobotBase(4, 3, railgun),
                      m_radar_dir(3),
                      m_target_row(-1),
                      m_target_col(-1),
                      m_has_target(false)
    {
        // Optional – if arena doesn't overwrite these, they’ll show up nicely.
        m_name = "Sweeper";
        m_character = 'S';
    }

    // Decide where to point radar
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = m_radar_dir;
        // Next time, look the other way
        m_radar_dir = (m_radar_dir == 3 ? 7 : 3);
    }

    // Look at radar results and pick a target (first robot seen)
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        m_has_target = false;
        m_target_row = -1;
        m_target_col = -1;

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')  // live robot
            {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                break;              // take the first one we see
            }
        }
    }

    // If we have a target, shoot it
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (m_has_target)
        {
            shot_row = m_target_row;
            shot_col = m_target_col;
            // use target once, then forget – radar will reacquire
            m_has_target = false;
            return true;
        }
        return false;
    }

    // Patrol left/right when not shooting
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

        // If we're very close to left edge, go right. Close to right edge, go left.
        if (cur_col <= 1)
        {
            move_direction = 3; // right
        }
        else if (cur_col >= m_board_col_max - 2)
        {
            move_direction = 7; // left
        }
        else
        {
            // middle of the board – keep moving in last radar direction
            move_direction = (m_radar_dir == 3 ? 3 : 7);
        }

        move_distance = 1; // conservative – let arena handle collisions
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Sweeper();
}
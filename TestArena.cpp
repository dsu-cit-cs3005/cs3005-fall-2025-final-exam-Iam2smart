#include "TestArena.h"
#include "RadarObj.h"
#include <iomanip>

// Helper to record and print a test result
bool TestArena::print_test_result(const std::string& test_name, bool condition) {
    std::string prefix = condition ? "[PASS] " : "[FAIL] ";
    std::cout << prefix << test_name << '\n';
    test_log.push_back((condition ? "PASS: " : "FAIL: ") + test_name);
    return condition;
}

// Print a simple summary at the end
void TestArena::print_summary() {
    std::cout << "\n================ Test Summary ================\n";
    int passed = 0;
    for (const auto& line : test_log) {
        std::cout << "  " << line << '\n';
        if (line.rfind("PASS:", 0) == 0) {
            ++passed;
        }
    }
    std::cout << "----------------------------------------------\n";
    std::cout << "  " << passed << " / " << test_log.size() << " tests passed\n";
    std::cout << "==============================================\n";
}

// ----------------------------------------------------------
// 1) RobotBase creation / configuration sanity
// ----------------------------------------------------------
void TestArena::test_robot_creation() {
    bool ok = true;

    RobotOutOfBounds out_bot;
    BadMovesRobot bad_bot;
    JumperRobot jumper_bot;
    ShooterRobot shooter_rail(railgun,      "RailShooter");
    ShooterRobot shooter_flame(flamethrower,"FlameShooter");
    ShooterRobot shooter_grenade(grenade,   "GrenadeShooter");
    ShooterRobot shooter_hammer(hammer,     "HammerShooter");

    auto check_stats = [&](RobotBase& rb, WeaponType expected_weapon) {
        bool local_ok = true;
        int move  = rb.get_move_speed();
        int armor = rb.get_armor();

        local_ok &= (rb.get_weapon() == expected_weapon);
        local_ok &= (move  >= 2 && move  <= 5);
        local_ok &= (armor >= 2 && armor <= 5);
        local_ok &= (move + armor == 7);   // spec: total 7 points

        return local_ok;
    };

    ok &= check_stats(out_bot,        hammer);
    ok &= check_stats(bad_bot,        grenade);
    ok &= check_stats(jumper_bot,     flamethrower);
    ok &= check_stats(shooter_rail,   railgun);
    ok &= check_stats(shooter_flame,  flamethrower);
    ok &= check_stats(shooter_grenade,grenade);
    ok &= check_stats(shooter_hammer, hammer);

    print_test_result("RobotBase creation & configuration", ok);
}

// ----------------------------------------------------------
// 2) "Initialize board" – basic location / boundaries sanity
//    (tests RobotBase location helpers, not Arena::init_board)
// ----------------------------------------------------------
void TestArena::test_initialize_board() {
    bool ok = true;

    JumperRobot bot;
    bot.set_boundaries(10, 10);   // pretend board is 10x10

    // move_to should update current location
    bot.move_to(5, 5);
    int r = -1, c = -1;
    bot.get_current_location(r, c);
    ok &= (r == 5 && c == 5);

    // disable_movement should zero out movement (like being in a pit)
    bot.disable_movement();
    ok &= (bot.get_move_speed() == 0);

    print_test_result("Initialize board / location helpers", ok);
}

// ----------------------------------------------------------
// 3) Handle movement – invalid move requests from BadMovesRobot
// ----------------------------------------------------------
void TestArena::test_handle_move() {
    bool ok = true;

    BadMovesRobot bad;

    int dir = 0, dist = 0;

    // 1st call: direction = 3, distance = 10 (too far)
    bad.get_move_direction(dir, dist);
    ok &= (dir == 3 && dist == 10);

    // 2nd: direction = 1, distance = -2 (negative)
    bad.get_move_direction(dir, dist);
    ok &= (dir == 1 && dist == -2);

    // 3rd: direction = 9, distance = 3 (invalid dir)
    bad.get_move_direction(dir, dist);
    ok &= (dir == 9 && dist == 3);

    // 4th: direction = 0, distance = 3 (direction 0 meaning "no movement")
    bad.get_move_direction(dir, dist);
    ok &= (dir == 0 && dist == 3);

    // After that, it should fall back to no-move
    bad.get_move_direction(dir, dist);
    ok &= (dir == 0 && dist == 0);

    print_test_result("Handle movement (BadMovesRobot cheats)", ok);
}

// ----------------------------------------------------------
// 4) Handle collision-like scenarios – RobotOutOfBounds pattern
// ----------------------------------------------------------
void TestArena::test_handle_collision() {
    bool ok = true;

    RobotOutOfBounds rob;
    int dir = 0, dist = 0;

    // First move: valid (down-right)
    rob.get_move_direction(dir, dist);
    ok &= (dir == 4 && dist == 2);

    // Next four moves are out-of-bounds in each direction
    rob.get_move_direction(dir, dist);  // right
    ok &= (dir == 3 && dist == 2);

    rob.get_move_direction(dir, dist);  // down
    ok &= (dir == 5 && dist == 2);

    rob.get_move_direction(dir, dist);  // up
    ok &= (dir == 1 && dist == 2);

    rob.get_move_direction(dir, dist);  // left
    ok &= (dir == 7 && dist == 2);

    // Final fall-back
    rob.get_move_direction(dir, dist);
    ok &= (dir == 0 && dist == 0);

    print_test_result("Handle collision / boundary requests", ok);
}

// ----------------------------------------------------------
// 5) TestRobot: bad shot locations (negative or off-board)
// ----------------------------------------------------------
void TestArena::test_handle_shot_with_fake_radar() {
    bool ok = true;

    TestRobot test_bot(3, 4, railgun, "Testy");
    test_bot.set_boundaries(10, 10);
    test_bot.move_to(5, 5);

    int shot_r = 0, shot_c = 0;

    // move_attempt starts 0 -> even -> negative shot
    test_bot.move_attempt = 0;
    bool fired = test_bot.get_shot_location(shot_r, shot_c);
    ok &= fired;
    ok &= (shot_r < 0 && shot_c < 0);   // invalid negative

    // move_attempt odd -> off-board shot
    test_bot.move_attempt = 1;
    fired = test_bot.get_shot_location(shot_r, shot_c);
    ok &= fired;
    ok &= (shot_r >= 10 || shot_c >= 10);   // outside arena bounds

    print_test_result("Handle shot with bad coordinates (TestRobot)", ok);
}

// ----------------------------------------------------------
// 6) ShooterRobot with all weapons + radar / shot pipeline
// ----------------------------------------------------------
void TestArena::test_robot_with_all_weapons() {
    bool ok = true;

    ShooterRobot shooter_rail(railgun,      "RailShooter");
    ShooterRobot shooter_flame(flamethrower,"FlameShooter");
    ShooterRobot shooter_grenade(grenade,   "GrenadeShooter");
    ShooterRobot shooter_hammer(hammer,     "HammerShooter");

    // All of these should use local radar direction (0)
    int dir;
    shooter_rail.get_radar_direction(dir);
    ok &= (dir == 0);
    shooter_flame.get_radar_direction(dir);
    ok &= (dir == 0);
    shooter_grenade.get_radar_direction(dir);
    ok &= (dir == 0);
    shooter_hammer.get_radar_direction(dir);
    ok &= (dir == 0);

    // Build a fake radar result and ensure shot aims at first target
    std::vector<RadarObj> radar_results;
    radar_results.emplace_back('R', 3, 4);
    radar_results.emplace_back('M', 5, 6);

    int shot_r = 0, shot_c = 0;
    bool fired = false;

    shooter_rail.process_radar_results(radar_results);
    fired = shooter_rail.get_shot_location(shot_r, shot_c);
    ok &= fired && shot_r == 3 && shot_c == 4;

    shooter_flame.process_radar_results(radar_results);
    fired = shooter_flame.get_shot_location(shot_r, shot_c);
    ok &= fired && shot_r == 3 && shot_c == 4;

    shooter_grenade.process_radar_results(radar_results);
    fired = shooter_grenade.get_shot_location(shot_r, shot_c);
    ok &= fired && shot_r == 3 && shot_c == 4;

    shooter_hammer.process_radar_results(radar_results);
    fired = shooter_hammer.get_shot_location(shot_r, shot_c);
    ok &= fired && shot_r == 3 && shot_c == 4;

    print_test_result("ShooterRobot with all weapons (radar + shooting)", ok);
}

// ----------------------------------------------------------
// 7) Grenade / damage – use RobotBase::take_damage as Arena would
// ----------------------------------------------------------
void TestArena::test_grenade_damage() {
    bool ok = true;

    ShooterRobot target(grenade, "GrenadeTarget");

    int h0 = target.get_health();
    ok &= (h0 == 100);   // spec: robots start at 100 health

    // First hit: 30 damage
    int h1 = target.take_damage(30);
    ok &= (h1 == target.get_health());
    ok &= (h1 == h0 - 30);

    // Second hit: 80 damage (may overkill)
    int h2 = target.take_damage(80);
    ok &= (h2 == target.get_health());
    int expected_h2 = h1 - 80;
    if (expected_h2 < 0) expected_h2 = 0;
    ok &= (h2 == expected_h2);

    // Big overkill: should clamp at 0
    int h3 = target.take_damage(999);
    ok &= (h3 == target.get_health());
    ok &= (h3 == 0);

    print_test_result("Grenade / damage handling via RobotBase::take_damage", ok);
}

// ----------------------------------------------------------
// 8) Radar tests – TestRobot and ShooterRobot behavior
// ----------------------------------------------------------
void TestArena::test_radar() {
    bool ok = true;

    // TestRobot cycles radar directions 0..8
    TestRobot tbot(3, 4, railgun, "RadarTester");
    for (int i = 0; i < 12; ++i) {
        int dir = -1;
        tbot.get_radar_direction(dir);
        int expected = tbot.move_attempt % 9; // move_attempt not incremented here
        ok &= (dir == expected);
        tbot.move_attempt++;
    }

    // ShooterRobot always uses local radar (0)
    ShooterRobot shooter(railgun, "RadarShooter");
    int dir = -1;
    shooter.get_radar_direction(dir);
    ok &= (dir == 0);

    print_test_result("Radar direction behavior (TestRobot & ShooterRobot)", ok);
}

void TestArena::test_radar_local() {
    bool ok = true;

    ShooterRobot shooter(railgun, "LocalRadar");
    int dir = -1;
    shooter.get_radar_direction(dir);
    ok &= (dir == 0);   // "local radar" code uses 0

    // Also verify that when radar_results is empty, shooter does not fire
    std::vector<RadarObj> empty_results;
    shooter.process_radar_results(empty_results);

    int shot_r = 0, shot_c = 0;
    bool fired = shooter.get_shot_location(shot_r, shot_c);
    ok &= (!fired);

    print_test_result("Radar local scan & no-target behavior", ok);
}
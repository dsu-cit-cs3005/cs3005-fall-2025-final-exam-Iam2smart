# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic
ALL_THE_OS = Arena.o RobotBase.o

# Default: build both programs
all: RobotWarz test_arena

RobotWarz: RobotWarz.o Arena.o RobotBase.o
	$(CXX) $(CXXFLAGS) RobotWarz.o Arena.o RobotBase.o -ldl -o RobotWarz

RobotWarz.o: RobotWarz.cpp Arena.h RobotBase.h RadarObj.h
	$(CXX) $(CXXFLAGS) -c RobotWarz.cpp

test_arena: test_arena.o TestArena.o $(ALL_THE_OS)
	$(CXX) -g -o test_arena test_arena.o TestArena.o $(ALL_THE_OS)

TestArena.o: TestArena.cpp TestArena.h Arena.h RobotBase.h RadarObj.h
	$(CXX) $(CXXFLAGS) -c TestArena.cpp

Arena.o: Arena.cpp Arena.h RadarObj.h
	$(CXX) $(CXXFLAGS) -c Arena.cpp

RobotBase.o: RobotBase.cpp RobotBase.h RadarObj.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp

clean:
	rm -f *.o RobotWarz test_arena *.so
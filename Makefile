all: grep55

grep55: grep55.cpp
	$(CPP) -std=c++17 -o grep55 grep55.cpp -lstdc++fs -lstdc++


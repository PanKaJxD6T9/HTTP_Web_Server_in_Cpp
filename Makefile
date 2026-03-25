CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
LDFLAGS = -lws2_32
TARGET = server
SRC = src/main.cpp src/parser.cpp src/router.cpp src/response.cpp src/server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /Q $(TARGET).exe 2>NUL || rm -f $(TARGET)

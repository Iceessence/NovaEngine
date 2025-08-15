CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
LDFLAGS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXinerama -lXcursor -lXi

TARGET = NovaSimple
SOURCE = src/app/SimpleMain.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
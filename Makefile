CXX = g++
LD = g++
CXXFLAGS = -Wall  -g -ggdb
EXEC = smooth
LDFLAGS = -o $(EXEC) -lSOIL -L/usr/lib -lGL -lGLU -lglut -lGLEW
SOURCES = smooth.cpp
OBJS = $(SOURCES:.cpp=.o)


$(EXEC) : $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS)  

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^

clean:
	rm -rf *.o smooth


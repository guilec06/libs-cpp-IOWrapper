CXX	=	g++
CXXFLAGS = -Wall -Wextra -O2 -g
INCLUDES = -Iinclude -Iinclude/io_uring

CXX_SRCS = src/Async/AsyncIORing.cpp
CXX_OBJS = $(CXX_SRCS:.cpp=.o)

C_OBJS = src/io_uring/ring_access.o src/io_uring/ring_ressource_manager.o

OBJS = $(CXX_OBJS) $(C_OBJS)

all: $(CXX_OBJS) io_uring

io_uring:
	$(MAKE) -C src/io_uring

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(CXX_OBJS) example
	$(MAKE) -C src/io_uring clean

example: all
	$(CXX) -o example $(CXXFLAGS) $(INCLUDES) main.cpp $(OBJS)

.PHONY: all clean example io_uring

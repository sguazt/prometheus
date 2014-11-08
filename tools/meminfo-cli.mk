CXXFLAGS += -Wall -Wextra -ansi -pedantic
CXXFLAGS += -Og -g
CXXFLAGS += -I$(HOME)/sys/src/git/boost
LDFLAGS += -L$(HOME)/sys/src/git/boost/stage/lib -lboost_system
LDFLAGS += -ljsoncpp

all: meminfo-cli

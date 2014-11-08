CXXFLAGS+=-Wall -Wextra -ansi -pedantic -g
CXXFLAGS+=-I$(HOME)/sys/src/git/boost
CXXFLAGS+=$(shell pkg-config libvirt --cflags)
LDFLAGS+=-lm
LDFLAGS+=$(shell pkg-config libvirt --libs)

all: dom_stats

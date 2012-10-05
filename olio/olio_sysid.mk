CXXFLAGS+=-Wall -Wextra -ansi -pedantic -g
CXXFLAGS+=-I./inc
CXXFLAGS+=-I$(HOME)/projects/boost-trunk
CXXFLAGS+=$(shell pkg-config libvirt --cflags)
LDFLAGS+=-lm
LDFLAGS+=$(shell pkg-config libvirt --libs)


all: src/olio_sysid

clean:
	rm -rf src/olio_sysid src/olio_sysid.o

CXXFLAGS+=-Wall -Wextra -ansi -pedantic -g
CXXFLAGS+=-I../inc
CXXFLAGS+=-I$(HOME)/Projects/src/dcsxx-commons/inc
CXXFLAGS+=-I$(HOME)/sys/src/git/boost
CXXFLAGS+=$(shell pkg-config libvirt --cflags)
CXXFLAGS+=-DDCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
#CXXFLAGS+=-DDCS_TESTBED_SENSOR_MEMINFO_SERVER_ADDRESS="\"10.10.15.148\"" -DDCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT="\"9090\""
CXXFLAGS+=-DDCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT="\"9090\""
LDFLAGS+=-lm
LDFLAGS+=$(shell pkg-config libvirt --libs)
LDFLAGS+=-L$(HOME)/sys/src/git/boost/stage/lib
LDLIBS+=-lboost_system -lboost_chrono -lboost_thread -lpthread -lrt
LDLIBS+=-ljsoncpp


all: dom_stats dom_mem_stress

clean:
	$(RM) dom_stats
	$(RM) dom_mem_stress

###### User-configurable parameteres

## Paths
#boost_header_path=$(HOME)/sys/src/git/boost
boost_header_path=$(CURDIR)/thirdparty/boost_1_65_0
#boost_libs_path=$(HOME)/sys/src/git/boost/stage/lib
boost_libs_path=$(CURDIR)/thirdparty/boost_1_65_0/stage/lib
boost_numeric_bindings_home=$(HOME)/sys/src/svn/boost-numeric_bindings
boost_ublasx_home=../boost-ublasx
cplex_home=$(HOME)/sys/opt/optim/ibm/ILOG/CPLEX_Studio1271
dcsxx_commons_home=../dcsxx-commons
dcsxx_control_home=../dcsxx-control
dcsxx_network_home=../dcsxx-network
dcsxx_sysid_home=../dcsxx-sysid
fuzzylite_header_path=$(HOME)/sys/src/git/fuzzylite/fuzzylite
#fuzzylite_libs_path=$(HOME)/sys/src/git/fuzzylite/fuzzylite/debug/bin
fuzzylite_libs_path=$(HOME)/sys/src/git/fuzzylite/fuzzylite/release/bin
fuzzylitex_home=../fuzzylitex
gurobi_home=$(HOME)/sys/opt/optim/gurobi605/linux64
yaml_header_path=$(HOME)/sys/include
yaml_libs_path=$(HOME)/sys/lib

## Inform that MATLAB is available
dcs_testbed_have_matlab=1

## Choose what optimizer to use
dcs_control_qp_use_cplex=1
dcs_control_qp_use_gurobi=0

## Port used to contact memory info server inside a VM
meminfo_server_port=9090

## Have LAPACK
flx_have_lapack=1
####################################


ifeq (1,$(dcs_control_qp_use_cplex))
#cplex_version=126
cplex_incs=-I$(cplex_home)/cplex/include -I$(cplex_home)/concert/include -DIL_STD
cplex_libs=-L$(cplex_home)/cplex/lib/x86-64_linux/static_pic -L$(cplex_home)/concert/lib/x86-64_linux/static_pic -lilocplex -lcplex -lconcert -lm -lpthread
else ifeq (1,$(dcs_control_qp_use_gurobi))
gurobi_incs=-I$(gurobi_home)/include
gurobi_libs=-L$(gurobi_home)/lib -lgurobi60 -lgurobi_c++ -Wl,-rpath,$(gurobi_home)/lib
endif


# verify that JAVA_HOME is set to something (should be the bare minimum)
ifndef JAVA_HOME
	$(warning WARNING: Variable JAVA_HOME is not set! Use default value.)
	JAVA_HOME=/usr/lib/jvm/java
endif

project_home=$(PWD)
export project_home

CXXFLAGS+=-Wall -Wextra -ansi -pedantic
#CXXFLAGS+=-Wall -Wextra -std=c++11 -pedantic
CXXFLAGS+=-g

# boost
CXXFLAGS+=-I$(boost_header_path)
CXXFLAGS+=-DBOOST_THREAD_VERSION=4
LDFLAGS+=-L$(boost_libs_path)
LDLIBS+=-lboost_system -lboost_chrono -lboost_thread -lpthread -lrt
LDLIBS+=-lboost_timer

# boost-numeric_bindings
CXXFLAGS+=-I$(boost_numeric_bindings_home)

# boost-ublasx
CXXFLAGS+=-I$(boost_ublasx_home)

# dcsxx-commons
CXXFLAGS+=-I$(dcsxx_commons_home)/inc

# dcsxx-control
CXXFLAGS+=-I$(dcsxx_control_home)/inc
ifeq (1,$(dcs_control_qp_use_cplex))
CXXFLAGS+=-DDCS_CONTROL_QP_USE_CPLEX
CXXFLAGS+=$(cplex_incs)
LDFLAGS+=$(cplex_libs)
else ifeq (1,$(dcs_control_qp_use_gurobi))
CXXFLAGS+=-DDCS_CONTROL_QP_USE_GUROBI
CXXFLAGS+=$(gurobi_incs)
LDFLAGS+=$(gurobi_libs)
endif

# dcsxx-network
CXXFLAGS+=-I$(dcsxx_network_home)/inc

# dcsxx-sysid
CXXFLAGS+=-I$(dcsxx_sysid_home)/inc

# dcsxx-testbed
CXXFLAGS+=-I./inc
#CXXFLAGS+=-DDCS_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION
#CXXFLAGS+=-DDCS_TESTBED_EXP_DUMP_SMOOTHED_DATA
#CXXFLAGS+=-DDCS_TESTBED_EXP_FILTER_OPERATIONS
#CXXFLAGS+=-DDCS_TESTBED_EXP_RESET_ESTIMATION_EVERY_INTERVAL
#CXXFLAGS+=-DDCS_TESTBED_APP_MGR_APPLY_EWMA_TO_EACH_OBSERVATION
#CXXFLAGS+=-DDCS_TESTBED_USE_LQRY_APP_MGR
#CXXFLAGS+=-DDCS_TESTBED_EXP_LQ_APP_MGR_USE_ALT_SS=$(shell printf "%d" "'Y'")
#CXXFLAGS+=-DDCS_TESTBED_EXP_LQ_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
#CXXFLAGS+=-DDCS_TESTBED_EXP_LQ_APP_MGR_USE_COMPENSATION
#CXXFLAGS+=-DDCS_TESTBED_USE_PADALA2009_AUTOCONTROL_APP_MGR
#CXXFLAGS+=-DDCS_TESTBED_EXP_PADALA2009_AUTOCONTROL_APP_MGR_USE_ARX_B0_SIGN_HEURISTIC
#CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_SQLITE_DATA_STORE
#CXXFLAGS+=$(shell pkg-config sqlite3 --cflags)
#CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_MYSQL_DATA_STORE
#CXXFLAGS+=$(shell mysql_config --cflags) -I$(HOME)/sys/include
#CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_RAM_DATA_STORE
#CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_BOOST_THREAD_SYNC_PACKET_QUEUE
##CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_MWMR_PACKET_QUEUE
##CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_BOOST_LOCKFREE_SPSC_PACKET_QUEUE
##CXXFLAGS+=-DDCS_TESTBED_NETSNIF_USE_DCS_CONCURRENT_BLOCKING_PACKET_QUEUE
CXXFLAGS+=-DDCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
CXXFLAGS+=-DDCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT="\"$(meminfo_server_port)\""
ifeq (1,$(dcs_testbed_have_matlab))
CXXFLAGS+=-DDCS_TESTBED_HAVE_MATLAB
endif
LDLIBS+=-lm
LDLIBS+=-llapack -lblas
LDLIBS+=-ljsoncpp

# fuzzylite
CXXFLAGS+=-I$(fuzzylite_header_path)
LDFLAGS+=-L$(fuzzylite_libs_path)
#LDLIBS+=-lfuzzylited
LDLIBS+=-lfuzzylite

# fuzzylitex
CXXFLAGS+=-I$(fuzzylitex_home)/include
CXXFLAGS+=-DFLX_CONFIG_HAVE_LAPACK
LDLIBS+=-llapack -lblas -lm
#CXXFLAGS+=-DFLX_CONFIG_HAVE_LAPACKE -I/usr/include/lapacke
#CXXFLAGS+=-DBOOST_NUMERIC_BINDINGS_LAPACK_LAPACKE -Dlapack_complex_float="void" -Dlapack_complex_double="void"
#LDLIBS+=-llapacke
LDFLAGS+=-L$(fuzzylitex_home)/bin
LDLIBS+=-lfuzzylitex

# libvirt
CXXFLAGS+=$(shell pkg-config libvirt --cflags)
LDFLAGS+=$(shell pkg-config libvirt --libs)

# t-digest
CXXFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
DCS_TESTBED_JNI_CLASSPATH+="\"$(CLASSPATH):thirdparty/t-digest\""
CXXFLAGS+=-DDCS_TESTBED_JNI_CLASSPATH=$(DCS_TESTBED_JNI_CLASSPATH)
LDFLAGS += -L$(JAVA_HOME)/jre/lib/i386 -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/i386/client -L$(JAVA_HOME)/jre/lib/amd64/server
LDLIBS += -ljvm

# yaml
CXXFLAGS+=-I$(yaml_header_path)
LDFLAGS+=-L$(yaml_libs_path)
LDLIBS+=-lyaml-cpp


.PHONY: all clean sysid sysmgt thirdparty


#all: netsnif sysid sysmgt

#netsnif: src/netsnif

all: sysid sysmgt

clean: thirdparty-clean
	$(RM)	src/sysid.o src/sysid \
			src/sysmgt.o src/sysmgt
#			src/netsnif.o src/netsnif

sysid: thirdparty src/sysid

sysmgt: thirdparty src/sysmgt

# These two rules are needed because the default behavior would be:
#  $(CXX) $(CXXFLAGS) $(LDFLAGS) foobar.cpp -o foobar
# which causes CPLEX to not link

src/sysmgt: src/sysmgt.o thirdparty/t-digest/tdigestx.o thirdparty/t-digest/GiwsException.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

src/sysid: src/sysid.o thirdparty/t-digest/tdigestx.o thirdparty/t-digest/GiwsException.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

thirdparty: tdigest

thirdparty-clean: tdigest-clean

tdigest:
	cd thirdparty/t-digest && $(MAKE)

tdigest-clean:
	cd thirdparty/t-digest && $(MAKE) clean

## This file contains default settings
## Variables ending with an underscore are not part of configuration.
## Instead they are auxiliary variables
thirdparty_path_ := $(CURDIR)/thirdparty

## [build]
## Generic settings for compiling and linking
# Compiler flags with debug enabled)
build_cflags ?= -O0 -g -UNDEBUG
# Compiler flags with debug disabled)
#build_cflags ?= -O3 -DNDEBUG
build_ldflags ?=
build_ldlibs ?=

## [Boost]
## Settings for the Boost C++ libraries
boost_home_ := $(thirdparty_path_)/boost_1_65_0
boost_cflags ?= -I$(boost_home_)
boost_ldflags ?= -L$(boost_home_)/stage/lib
boost_ldlibs ?=

## [Boost.NumericBindings]
## Settings for the Boost.NumericBindings libraries (not officially part of Boost)
boost_numeric_bindings_home_ := $(thirdparty_path_)/boost-numeric_bindings
boost_numeric_bindings_cflags ?= -I$(boost_numeric_bindings_home_)
boost_numeric_bindings_ldflags ?=
boost_numeric_bindings_ldlibs ?=

## [Boost.uBLASx]
## Settings for the Boost.uBLASx libraries (not officially part of Boost)
boost_ublasx_home_ = $(thirdparty_path_)/boost-ublasx
boost_ublasx_cflags ?= -I$(boost_ublasx_home_)
boost_ublasx_ldflags ?=
boost_ublasx_ldlibs ?=

## [CPLEX]
## Settings for the IBM CPLEX optimizer
cplex_home_ = $(thirdparty_path_)/ibm/ILOG/CPLEX_Studio1271
cplex_cflags ?= -I$(cplex_home_)/cplex/include -I$(cplex_home_)/concert/include -DIL_STD
cplex_ldflags ?= -L$(cplex_home_)/cplex/lib/x86-64_linux/static_pic -L$(cplex_home_)/concert/lib/x86-64_linux/static_pic
cplex_ldlibs ?= -lilocplex -lcplex -lconcert -lm -lpthread

## [dcsxx-commons]
## Settings for the dcsxx-commons project
dcs_commons_home_ = $(thirdparty_path_)/dcsxx-commons
dcs_commons_cflags ?= -I$(dcs_commons_home_)/inc
dcs_commons_ldflags ?=
dcs_commons_ldlibs ?=

## [dcsxx-control]
## Settings for the dcsxx-control project
dcs_control_home_ = $(thirdparty_path_)/dcsxx-control
dcs_control_cflags ?= -I$(dcs_control_home_)/inc
dcs_control_ldflags ?=
dcs_control_ldlibs ?=
# Choose what quadratic programming optimizer to use (0 means "use", 1 means "don't use")
dcs_control_qp_use_cplex ?= 1
dcs_control_qp_use_gurobi ?= 0

## [dcsxx-sysid]
## Settings for the dcsxx-sysid project
dcs_sysid_home_ = $(thirdparty_path_)/dcsxx-sysid
dcs_sysid_cflags ?= -I$(dcs_sysid_home_)/inc
dcs_sysid_ldflags ?=
dcs_sysid_ldlibs ?=

## [fuzzylite]
## Settings for the Fuzzylite library (for C++)
fuzzylite_home_ = $(thirdparty_path)/fuzzylite
fuzzylite_debug_ = 0
fuzzylite_cflags ?= -I$(fuzzylite_home_)/fuzzylite
ifeq (1,$(fuzzylite_debug_))
fuzzylite_ldflags ?= -L$(fuzzylite_home_)/fuzzylite/debug/bin
fuzzylite_ldlibs ?= -lfuzzylited
else
fuzzylite_ldflags ?= -L$(fuzzylite_home_)/fuzzylite/release/bin
fuzzylite_ldlibs ?= -lfuzzylite
endif

## [fuzzylitex]
## Settings for the Fuzzylitex library (not officialy part of Fuzzylite)
fuzzylitex_home_ = $(thirdparty_path)/fuzzylitex
fuzzylitex_cflags ?= -I$(fuzzylitex_home_)/include
fuzzylitex_ldflags ?= -L$(fuzzylitex_home_)/bin
fuzzylitex_ldlibs ?= -lfuzzylitex
fuzzylitex_use_lapack ?= 1

## [GUROBI]
## Settings for the GUROBI optimizer
gurobi_home_ = $(thirdparty_path_)/gurobi605/linux64
gurobi_cflags ?= -I$(gurobi_home_)/include
gurobi_ldflags ?= -L$(gurobi_home_)/lib -lgurobi60 -lgurobi_c++ -Wl,-rpath,$(gurobi_home_)/lib
gurobi_ldlibs ?=

## [Java]
## Settings for the Java virtual machine
# # Variable pointing to the Java installation directory
java_home = $(JAVA_HOME)
# Variables for JNI
# - Compiler flags
java_jni_cflags ?= -I$(java_home)/include -I$(java_home)/include/linux
# - Linker flags (x86 arch)
#java_jni_ldflags ?= -L$(java_home)/jre/lib/i386 -L$(java_home)/jre/lib/i386/client
# - Linker flags (x86_64 arch)
java_jni_ldflags ?= -L$(java_home)/jre/lib/amd64 -L$(java_home)/jre/lib/amd64/server
# - Libraries to link
java_jni_ldlibs ?= -ljvm

## [JsonCpp]
## Settings for the JSONcpp library
json_cflags ?= $(shell pkg-config jsoncpp --cflags)
json_ldflags ?= $(shell pkg-config jsoncpp --libs)
json_ldlibs ?= 

## [LAPACK]
## Settings for the LAPACK library
lapack_cflags ?=
lapack_ldflags ?=
lapack_ldlibs ?= -llapack -lblas -lm

## [libvirt]
## Settings for the libvirt project
libvirt_cflags ?= $(shell pkg-config libvirt --cflags)
libvirt_ldflags ?= $(shell pkg-config libvirt --libs)
libvirt_ldlibs ?=

## [Prometheus]
## Settings for the Prometheus toolkit
# Meminfo server (a "memory info" server running inside a VM used by memory
# sensor to collect memory demands)
# - Tells whether meminfo server should be used
prometheus_have_meminfo_server ?= 1
# - Port used to contact meminfo server inside a VM
prometheus_meminfo_server_port ?= 9090
# - Inform that MATLAB is available
prometheus_have_matlab ?= 1

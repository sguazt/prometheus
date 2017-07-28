## This file contains default settings
## Variables ending with an underscore are not part of configuration.
## Instead they are auxiliary variables
thirdparty_path_ := $(CURDIR)/thirdparty

## [build]
## Generic settings for compiling and linking
build_cflags = -O0 -g -UNDEBUG
build_ldflags =
build_ldlibs =

## [Boost]
## Settings for the Boost C++ libraries
boost_cflags = -I/usr/include
boost_ldflags = -L/usr/lib64
boost_ldlibs =

## [Boost.NumericBindings]
## Settings for the Boost.NumericBindings libraries (not officially part of Boost)
boost_numeric_bindings_cflags = -I/opt/boost-numeric_bindings
boost_numeric_bindings_ldflags =
boost_numeric_bindings_ldlibs =

## [Boost.uBLASx]
## Settings for the Boost.uBLASx libraries (not officially part of Boost)
boost_ublasx_cflags = -I/opt/boost-ublasx
boost_ublasx_ldflags =
boost_ublasx_ldlibs =

## [CPLEX]
## Settings for the IBM CPLEX optimizer
cplex_cflags = -I/opt/ibm/ILOG/CPLEX_Studio1271/cplex/include -I/opt/ibm/ILOG/CPLEX_Studio1271/concert/include -DIL_STD
cplex_ldflags = -L/opt/ibm/ILOG/CPLEX_Studio1271/cplex/lib/x86-64_linux/static_pic -L/opt/ibm/ILOG/CPLEX_Studio1271/concert/lib/x86-64_linux/static_pic
cplex_ldlibs = -lilocplex -lcplex -lconcert -lm -lpthread

## [dcsxx-commons]
## Settings for the dcsxx-commons project
dcs_commons_cflags = -I/opt/dcsxx-commons/inc
dcs_commons_ldflags =
dcs_commons_ldlibs =

## [dcsxx-control]
## Settings for the dcsxx-control project
dcs_control_cflags = -I/opt/dcsxx-control/inc
dcs_control_ldflags =
dcs_control_ldlibs =
# Choose what quadratic programming optimizer to use (0 means "use", 1 means "don't use")
dcs_control_qp_use_cplex = 1
dcs_control_qp_use_gurobi = 0

## [dcsxx-sysid]
## Settings for the dcsxx-sysid project
dcs_sysid_cflags = -I/opt/dcsxx-sysid/inc
dcs_sysid_ldflags =
dcs_sysid_ldlibs =

## [fuzzylite]
## Settings for the Fuzzylite library (for C++)
fuzzylite_cflags = -I/opt/fuzzylite/fuzzylite
fuzzylite_ldflags = -L/opt/fuzzylite/fuzzylite/release/bin
fuzzylite_ldlibs = -lfuzzylite

## [fuzzylitex]
## Settings for the Fuzzylitex library (not officialy part of Fuzzylite)
fuzzylitex_cflags = -I/opt/fuzzylitex/include
fuzzylitex_ldflags = -L/opt/fuzzylitex/bin
fuzzylitex_ldlibs = -lfuzzylitex
fuzzylitex_use_lapack = 1

## [GUROBI]
## Settings for the GUROBI optimizer
gurobi_cflags = -I/opt/gurobi605/linux64/include
gurobi_ldflags = -L/opt/gurobi605/linux64/lib -lgurobi60 -lgurobi_c++ -Wl,-rpath,/opt/gurobi605/linux64/lib
gurobi_ldlibs =

## [Java]
## Settings for the Java virtual machine
# Variable pointing to the Java installation directory
java_home = /usr/lib/jvm/java
# Variables for JNI
java_jni_cflags = -I/usr/lib/jvm/java/include -I/usr/lib/jvm/java/include/linux
java_jni_ldflags = -L/usr/lib/jvm/java/jre/lib/amd64 -L/usr/lib/jvm/java/jre/lib/amd64/server
java_jni_ldlibs = -ljvm

## [JsonCpp]
## Settings for the JSONcpp library
json_cflags = $(shell pkg-config jsoncpp --cflags)
json_ldflags = $(shell pkg-config jsoncpp --libs)
json_ldlibs = 

## [LAPACK]
## Settings for the LAPACK library
lapack_cflags =
lapack_ldflags =
lapack_ldlibs = -llapack -lblas -lm

## [libvirt]
## Settings for the libvirt project
libvirt_cflags = $(shell pkg-config libvirt --cflags)
libvirt_ldflags = $(shell pkg-config libvirt --libs)
libvirt_ldlibs =

## [Prometheus]
## Settings for the Prometheus toolkit
# Meminfo server (a "memory info" server running inside a VM used by memory
# sensor to collect memory demands)
# - Tells whether meminfo server should be used
prometheus_have_meminfo_server = 1
# - Port used to contact meminfo server inside a VM
prometheus_meminfo_server_port = 9090
# - Inform that MATLAB is available
prometheus_have_matlab = 1

project_home=$(CURDIR)
#lib_path ?= $(prj_path)/lib
thirdparty_path = $(project_home)/thirdparty

ifneq (,$(wildcard ./config.mk))
include config.mk
endif
#include config_default.mk

$(info )
$(info === Read configuration)
$(info build_cflags = $(build_cflags))
$(info build_ldflags = $(build_ldflags))
$(info build_ldlibs = $(build_ldlibs))
$(info boost_cflags = $(boost_cflags))
$(info boost_ldflags = $(boost_ldflags))
$(info boost_ldlibs = $(boost_ldlibs))
$(info boost_numeric_bindings_cflags = $(boost_numeric_bindings_cflags))
$(info boost_numeric_bindings_ldflags = $(boost_numeric_bindings_ldflags))
$(info boost_numeric_bindings_ldlibs = $(boost_numeric_bindings_ldlibs))
$(info boost_ublasx_cflags = $(boost_ublasx_cflags))
$(info boost_ublasx_ldflags = $(boost_ublasx_ldflags))
$(info boost_ublasx_ldlibs = $(boost_ublasx_ldlibs))
$(info cplex_cflags = $(cplex_cflags))
$(info cplex_ldflags = $(cplex_ldflags))
$(info cplex_ldlibs = $(cplex_ldlibs))
$(info dcs_commons_cflags = $(dcs_commons_cflags))
$(info dcs_commons_ldflags = $(dcs_commons_ldflags))
$(info dcs_commons_ldlibs = $(dcs_commons_ldlibs))
$(info dcs_control_cflags = $(dcs_control_cflags))
$(info dcs_control_ldflags = $(dcs_control_ldflags))
$(info dcs_control_ldlibs = $(dcs_control_ldlibs))
$(info dcs_control_qp_use_cplex = $(dcs_control_qp_use_cplex))
$(info dcs_control_qp_use_gurobi = $(dcs_control_qp_use_gurobi))
$(info dcs_sysid_cflags = $(dcs_sysid_cflags))
$(info dcs_sysid_ldflags = $(dcs_sysid_ldflags))
$(info dcs_sysid_ldlibs = $(dcs_sysid_ldlibs))
$(info fuzzylite_cflags = $(fuzzylite_cflags))
$(info fuzzylite_ldflags = $(fuzzylite_ldflags))
$(info fuzzylite_ldlibs = $(fuzzylite_ldlibs))
$(info fuzzylitex_cflags = $(fuzzylitex_cflags))
$(info fuzzylitex_ldflags = $(fuzzylitex_ldflags))
$(info fuzzylitex_ldlibs = $(fuzzylitex_ldlibs))
$(info fuzzylitex_use_lapack = $(fuzzylitex_use_lapack))
$(info gurobi_cflags = $(gurobi_cflags))
$(info gurobi_ldflags = $(gurobi_ldflags))
$(info gurobi_ldlibs = $(gurobi_ldlibs))
$(info java_home = $(java_home))
$(info java_jni_cflags = $(java_jni_cflags))
$(info java_jni_ldflags = $(java_jni_ldflags))
$(info java_jni_ldlibs = $(java_jni_ldlibs))
$(info json_cflags = $(json_cflags))
$(info json_ldflags = $(json_ldflags))
$(info json_ldlibs = $(json_ldlibs))
$(info lapack_cflags = $(lapack_cflags))
$(info lapack_ldflags = $(lapack_ldflags))
$(info lapack_ldlibs = $(lapack_ldlibs))
$(info libvirt_cflags = $(libvirt_cflags))
$(info libvirt_ldflags = $(libvirt_ldflags))
$(info libvirt_ldlibs = $(libvirt_ldlibs))
$(info prometheus_have_meminfo_server = $(prometheus_have_meminfo_server))
$(info prometheus_meminfo_server_port = $(prometheus_meminfo_server_port))
$(info prometheus_have_matlab = $(prometheus_have_matlab))
$(info ===================================================================)
$(info  )


export build_cflags build_ldflags build_ldlibs
export boost_cflags boost_ldflags boost_ldlibs
export boost_numeric_bindings_cflags boost_numeric_bindings_ldflags boost_numeric_bindings_ldlibs
export boost_ublasx_cflags boost_ublasx_ldflags boost_ublasx_ldlibs
export cplex_cflags cplex_ldflags cplex_ldlibs
export dcs_commons_cflags dcs_commons_ldflags dcs_commons_ldlibs
export dcs_control_cflags dcs_control_ldflags dcs_control_ldlibs dcs_control_qp_use_cplex dcs_control_qp_use_gurobi
export dcs_sysid_cflags dcs_sysid_ldflags dcs_sysid_ldlibs
export fuzzylite_cflags fuzzylite_ldflags fuzzylite_ldlibs
export fuzzylitex_cflags fuzzylitex_ldflags fuzzylitex_ldlibs fuzzylitex_use_lapack
export gurobi_cflags gurobi_ldflags gurobi_ldlibs
export java_home java_jni_cflags java_jni_ldflags java_jni_ldlibs
export json_cflags json_ldflags json_ldlibs
export lapack_cflags lapack_ldflags lapack_ldlibs
export libvirt_cflags libvirt_ldflags libvirt_ldlibs
export prometheus_have_meminfo_server prometheus_meminfo_server_port prometheus_have_matlab


#CXXFLAGS += -Wall -Wextra -ansi -pedantic
CXXFLAGS += -Wall -Wextra -std=c++11 -pedantic
#ifeq (1,$(prometheus_debug))
#CXXFLAGS += -UNDEBUG
#CXXFLAGS += -O0 -g
#else
#CXXFLAGS += -DNDEBUG
#CXXFLAGS += -O3
#endif
LDLIBS += -lm

## [build]
CXXFLAGS += $(build_cflags)
LDFLAGS += $(build_ldflags)
LDLIBS += $(build_ldlibs)

## [Boost]
CXXFLAGS += $(boost_cflags)
CXXFLAGS += -DBOOST_THREAD_VERSION=4
LDFLAGS += $(boost_ldflags)
LDLIBS += -lboost_system -lboost_chrono -lboost_thread -lpthread -lrt
LDLIBS += -lboost_timer
LDLIBS += $(boost_ldlibs)

# [dcsxx-commons]
CXXFLAGS += $(dcs_commons_cflags)
LDFLAGS += $(dcs_commons_ldflags)
LDLIBS += $(dcs_commons_ldlibs)

# [Java]
# verify that JAVA_HOME is set to something (should be the bare minimum)
ifeq (,$(java_home))
java_home = /usr/lib/jvm/java
$(warning WARNING: Variable 'java_home' is not set! Set to the default value '$(java_home)'.)
endif
ifndef JAVA_HOME
#$(warning WARNING: Variable JAVA_HOME is not set! Use default value.)
#JAVA_HOME=/usr/lib/jvm/java
JAVA_HOME = $(java_home)
export JAVA_HOME
endif

#jni_srcpath ?= $(prj_path)/src/jni
#java_clspath ?= $(java_srcpath)
##java_clspath ?= $(java_clspath):$(thirdparty_path)/sqlite-jdbc-3.16.1.jar
##java_clspath ?= $(java_clspath):$(thirdparty_path)/commons-cli-1.3.1.jar
##java_clspath ?= $(java_clspath):$(thirdparty_path)/commons-lang3-3.5.jar
#java_clspath ?= $(java_clspath)$(patsubst %,:%,$(wildcard $(thirdpaty_path)/*.jar))

## [libvirt]
CXXFLAGS += $(libvirt_cflags)
LDFLAGS += $(libvirt_ldflags)
LDLIBS += $(libvirt_ldlibs)

## [t-digest]
tdigestx_home = $(thirdparty_path)/t-digest
CXXFLAGS += -I$(tdigestx_home)
CXXFLAGS += -I$(java_home)/include -I$(java_home)/include/linux
#DCS_TESTBED_JNI_CLASSPATH += "\"$(CLASSPATH):$(tdigestx_home)\""
#CXXFLAGS += -DDCS_TESTBED_JNI_CLASSPATH=$(DCS_TESTBED_JNI_CLASSPATH)
#CXXFLAGS += -DDCS_TESTBED_JNI_CLASSPATH="\"$(CLASSPATH):$(tdigestx_home)\""
##LDFLAGS += -L$(JAVA_HOME)/jre/lib/i386 -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/i386/client -L$(JAVA_HOME)/jre/lib/amd64/server
##LDFLAGS += -L$(JAVA_HOME)/jre/lib/i386 -L$(JAVA_HOME)/jre/lib/i386/client
#LDFLAGS += -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/amd64/server
#LDLIBS += -ljvm
CXXFLAGS += $(java_jni_cflags)
LDFLAGS += $(java_jni_ldflags)
LDLIBS += $(java_jni_ldlibs)

## [Prometheus]
CXXFLAGS += -I$(project_home)/lib/include
ifeq (1,$(prometheus_have_meminfo_server))
CXXFLAGS += -DDCS_TESTBED_SENSOR_HAVE_MEMINFO_SERVER
CXXFLAGS += -DDCS_TESTBED_SENSOR_MEMINFO_SERVER_PORT="\"$(prometheus_meminfo_server_port)\""
CXXFLAGS += $(json_cflags)
LDFLAGS += $(json_ldflags)
LDLIBS += $(json_ldlibs)
endif

# The two lines below are a trick to cheat Make in calling c++ instead of cc when linking object files
CC = $(CXX)
CFLAGS = $(CXXFLAGS)

export project_home tdigestx_home thirdparty_path
export CC CFLAGS CXXFLAGS LDFLAGS LDLIBS


.PHONY: all apps apps-clean apps-close-banner clean lib lib-clean scripts scripts-clean tdigestx tdigestx-clean test thirdparty thirdparty-clean
.DEFAULT_GOAL: lib


all: apps

apps: lib scripts thirdparty apps-close-banner
	cd apps && $(MAKE)

apps-close-banner:
	@echo ""
	@echo "==========================================================================="
	@echo "Don't forget to set library path before running these apps:"
	@echo "# export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):/path/to/boost/libs:/path/to/jvm/lib:/path/to/jvm/lib/server:/path/to/fuzzylite"
	@echo "You must use the same paths you specified in your 'config.mk' file."
	@echo "For instance:"
	#@echo "# export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):thirdparty/boost/stage/lib:$(JAVA_HOME)/jre/lib/amd64:$(JAVA_HOME)/jre/lib/amd64/server:thirdparty/fuzzylite/fuzzylite/release/bin"
	#@echo "or"
	@echo "# export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):thirdparty/boost/stage/lib:$(JAVA_HOME)/jre/lib:$(JAVA_HOME)/jre/lib/server:thirdparty/fuzzylite/fuzzylite/release/bin"
	@echo "==========================================================================="

apps-clean:
	cd apps && $(MAKE) clean

clean: apps-clean lib-clean thirdparty-clean scripts-clean

#lib: scripts thirdparty
lib:
	cd lib && $(MAKE)

lib-clean:
	cd lib && $(MAKE) clean

test: lib
	cd test && $(MAKE)

thirdparty: tdigestx

thirdparty-clean: tdigestx-clean

tdigestx:
	cd $(tdigestx_home) && $(MAKE)

tdigestx-clean:
	cd $(tdigestx_home) && $(MAKE) clean

scripts:
	#TODO: use scripts/fuzzylite_version to set the variable PROMETHEUS_FUZZYLITE_VERSION (note, as of 2021-01-03, fuzzylite does not have a macro for retrieve its version at compile-time)
	cd scripts && $(MAKE)

scripts-clean:
	cd scripts && $(MAKE) clean


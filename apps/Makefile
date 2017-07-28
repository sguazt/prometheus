CXXFLAGS += -I$(CURDIR)/include

## [boost-numeric_bindings]
CXXFLAGS += $(boost_numeric_bindings_cflags)
LDFLAGS += $(boost_numeric_bindings_ldflags)
LDLIBS += $(boost_numeric_bindings_ldlibs)

## [boost-ublasx]
CXXFLAGS += $(boost_ublasx_cflags)
LDFLAGS += $(boost_ublasx_ldflags)
LDLIBS += $(boost_ublasx_ldlibs)

## [dcsxx-control]
CXXFLAGS += $(dcs_control_cflags)
LDFLAGS += $(dcs_control_ldflags)
LDLIBS += $(dcs_control_ldlibs)
ifeq (1,$(dcs_control_qp_use_cplex))
CXXFLAGS += -DDCS_CONTROL_QP_USE_CPLEX
CXXFLAGS += $(cplex_cflags)
LDFLAGS += $(cplex_ldflags)
LDLIBS += $(cplex_ldlibs)
else ifeq (1,$(dcs_control_qp_use_gurobi))
CXXFLAGS += -DDCS_CONTROL_QP_USE_GUROBI
CXXFLAGS += $(gurobi_cflags)
LDFLAGS += $(gurobi_ldflags)
LDLIBS += $(gurobi_ldlibs)
else
$(error dcsxx-control: no QP optimizer selected)
endif


## [dcsxx-sysid]
CXXFLAGS += $(dcs_sysid_cflags)
LDFLAGS += $(dcs_sysid_ldflags)
LDLIBS += $(dcs_sysid_ldlibs)

## [fuzzylite]
CXXFLAGS += $(fuzzylite_cflags)
LDFLAGS += $(fuzzylite_ldflags)
LDLIBS += $(fuzzylite_ldlibs)

## [fuzzylitex]
CXXFLAGS += $(fuzzylitex_cflags)
LDFLAGS += $(fuzzylitex_ldflags)
LDLIBS += $(fuzzylitex_ldlibs)
ifeq (1,$(fuzzylitex_use_lapack))
CXXFLAGS += -DFLX_CONFIG_HAVE_LAPACK
#LDLIBS += -llapack -lblas -lm
CXXFLAGS += $(lapack_cflags)
LDFLAGS += $(lapack_ldflags)
LDLIBS += $(lapack_ldlibs)
endif
#CXXFLAGS +=- DFLX_CONFIG_HAVE_LAPACKE -I/usr/include/lapacke
#CXXFLAGS += -DBOOST_NUMERIC_BINDINGS_LAPACK_LAPACKE -Dlapack_complex_float="void" -Dlapack_complex_double="void"
#LDLIBS += -llapacke

## [t-digest]
tdigestx_objs = $(tdigestx_home)/tdigestx.o $(tdigestx_home)/GiwsException.o

## [lapack]
CXXFLAGS += $(lapack_cflags)
LDFLAGS += $(lapack_ldflags)
LDLIBS += $(lapack_ldlibs)

## [Prometheus]
ifeq (1,$(prometheus_have_matlab))
CXXFLAGS += -DDCS_TESTBED_HAVE_MATLAB
endif


.PHONY: all clean sysid sysmgt


all: src/sysid src/sysmgt

clean:
	$(RM) $(tdigestx_objs)
	$(RM) src/sysid.o src/sysid
	$(RM) src/sysmgt.o src/sysmgt

src/sysid: src/sysid.o $(tdigestx_objs)

src/sysmgt: src/sysmgt.o $(tdigestx_objs)


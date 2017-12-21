Prometheus
==========

A flexible toolkit for the experimentation with virtualized infrastructures.

How to Cite
-----------

Please, cite this project as the following paper (BibTeX format):

	@ARTICLE{CPE:CPE4400,
        author = {Cosimo Anglano and Massimo Canonico and Marco Guazzone},
        title = {{Prometheus}: a flexible toolkit for the experimentation with virtualized infrastructures},
		journal = {Concurrency and Computation: Practice and Experience},
		year = {2017},
		issn = {1532-0634},
		url = {http://dx.doi.org/10.1002/cpe.4400},
		doi = {10.1002/cpe.4400},
		pages = {e4400--n/a},
		keywords = {experimental evaluation, physical testbed, resource management, toolkit, virtualization},
		note = {In Press},
	}



In addition, you can cite the following papers where Prometheus has been used to carry out the experimental evaluation (BibTeX format):

    @ARTICLE{CPE:CPE3968,
        author = {Cosimo Anglano and Massimo Canonico and Marco Guazzone},
        title = {{FCMS}: a Fuzzy Controller for {CPU} and Memory Consolidation under {SLA} Constraints},
        journal = {Concurrency and Computation: Practice and Experience},
        year = {2017},
        volume = {29},
        number = {5},
        pages = {e3968--n/a}
        doi = {10.1002/cpe.3968},
        url = {http://dx.doi.org/10.1002/cpe.3968},
        issn = {1532-0634},
        keywords = {cloud computing, resource management, feedback control, fuzzy control, server consolidation, virtualized cloud applications},
    }

    @ARTICLE{CPE:CPE3410,
        author = {Cosimo Anglano and Massimo Canonico and Marco Guazzone},
        title = {{FC2Q}: Exploiting Fuzzy Control in Server Consolidation for Cloud Applications with {SLA} Constraints},
        journal = {Concurrency and Computation: Practice and Experience},
        volume = {27},
        number = {17},
        pages = {4491--4514},
        year = {2015},
        issn = {1532-0634},
        url = {http://dx.doi.org/10.1002/cpe.3410},
        doi = {10.1002/cpe.3410},
        keywords = {cloud computing, resource management, feedback control, fuzzy control, server consolidation, virtualized cloud applications},
    }

    @INPROCEEDINGS{6686023,
        author = {Luca Albano and Cosimo Anglano and Massimo Canonico and Marco Guazzone}, 
        booktitle = {2013 International Conference on Cloud and Green Computing}, 
        title = {Fuzzy-{Q}{\&}{E}: Achieving QoS Guarantees and Energy Savings for Cloud Applications with Fuzzy Control}, 
        year = {2013}, 
        pages = {159-166}, 
        month = {Sept},
        doi = {10.1109/CGC.2013.31}, 
        url = {http://dx.doi.org/10.1109/CGC.2013.31}, 
        keywords = {cloud computing; fuzzy control; power aware computing; quality of experience; quality of service; virtual machines; QoS; VMs; Xen-based testbed; bursty workload; cloud application management; dynamic workload; e-commerce benchmark; energy consumption; energy savings; fuzzy controller; fuzzy-Q&E; physical capacity; physical infrastructure; quality-of-services; virtual machines; Benchmark testing; Fuzzy control; IP networks; Pragmatics; Quality of service; Time factors; Virtual machine monitors}, 
    }


Overview
--------

Prometheus is a flexible toolkit for the experimentation with virtualized infrastructures.
The System Under Test (SUT) is managed by exploiting virtualization technologies and, in particular, by using the [libvirt](http://www.libvirt.org) library.

Prometheus consists of a set of C++ components for setting up and managing a physical testbed.
This testbed allows the experimenter to perform both system identification and resource management experiments

This project has been published in the following article:

> Cosimo Anglano, Massimo Canonico and Marco Guazzone.
>
> *Prometheus: a flexible toolkit for the experimentation with virtualized infrastructures*
>
> Concurrency and Computation: Practice and Experience, 2017.
>
> In Press.
>
> doi:[10.1002/cpe.4400](https://doi.org/10.1002/cpe.4400).

Furthermore, this project has been successfully used for the experimental evaluations presented in the following articles:

> Cosimo Anglano, Massimo Canonico and Marco Guazzone.
>
> *FCMS: a Fuzzy Controller for CPU and Memory Consolidation under SLA Constraints*
>
> Concurrency and Computation: Practice and Experience, 29(5), 2017.
>
> doi:[10.1002/cpe.3968](https://doi.org/10.1002/cpe.3968).
>
>
> Cosimo Anglano, Massimo Canonico and Marco Guazzone
>
> *FC2Q: Exploiting Fuzzy Control in Server Consolidation for Cloud Applications with SLA Constraints*
>
> Concurrency Computat.: Pract. Exper., 27(17):4491-4514, 2015.
>
> doi: [10.1002/cpe.3410](https://doi.org/10.1002/cpe.3410)
>
>
> Luca Albano, Cosimo Anglano, Massimo Canonico and Marco Guazzone
>
> *Fuzzy Q&E: Achieving QoS Guarantees and Energy Savings for Cloud Applications with Fuzzy Control*
>
> 2013 International Conference on Cloud and Green Computing (2013), Karlsruhe, Germany, pp. 159-166, 2013.
>
> doi: [10.1109/CGC.2013.31](https://doi.org/10.1109/CGC.2013.31)


Building
--------

### Prerequisites

#### Hard Requirements

The following software components are always required:

* A modern C++98 compiler (e.g., GCC v4.8 or newer is fine)
* [Boost](http://boost.org) C++ libraries (v1.60 or newer, but v1.64 which is broken)
* [dcsxx-commons](https://github.com/sguazt/dcsxx-commons) library (v2 or newer)
* [JsonCpp](https://github.com/open-source-parsers/jsoncpp) C++ library for interacting with JSON (v0.6 or newer)
* [libvirt](http://libvirt.org) virtualization API library (v1 or newer)
* ... and related prerequisites

#### Soft Requirements

The following software modules are needed only by certain classes of Prometheus (e.g., the Fuzzylite and Fuzzylitex libraries are needed only by those application managers that use fuzzy logic, like FCMS, APPLEware and FMPC):

* [Boost.Numeric Bindings](http://svn.boost.org/svn/boost/sandbox/numeric_bindings) library (v2 or newer)
* [Boost.uBLASx](https://github.com/sguazt/boost-ublasx) library (v1 or newer)
* [dcsxx-control](https://github.com/sguazt/dcsxx-control) library (v2 or newer)
* [dcsxx-sysid](https://github.com/sguazt/dcsxx-sysid) library (v1 or newer)
* [Fuzzylite](http://www.fuzzylite.com) fuzzy logic control library (v5 or newer)
* [Fuzzylitex](http://github.com/sguazt/fuzzylitex) fuzzy logic control library (v1.1.1 or newer)
* [LAPACK](http://www.netlib.org/lapack/) Linear Algebra PACKage (v3.5 or newer)
* Either [IBM CPLEX](https://www.ibm.com/software/products/it/ibmilogcpleoptistud) optimizer (v12.6 or newer) or [GUROBI](http://www.gurobi.com/products/gurobi-optimizer) optimizer (v6.0 or newer)

### Compilation

Note, since this is a header-only library, compilation is needed only when you create a full application or you want to compile the provided applications.

1. Make sure all the above prerequisites are satisfied.

2. Make sure you have properly set configuration parameters.

   Configuration parameters are specified as *GNU Make* variables.
   Default values are stored in `config_default.mk` file.
   If you need to change some parameter, do this in a new file called `config.mk`.
   Don't change the `config_default.mk` file!
   If you need to create a new `config.mk` file, you can use as template the `config_sample.mk` file.

   Valid configuration parameters are:

    * *Boost*:
        - `boost_cflags`: values to append to the `CXXFLAGS`.
        - `boost_ldflags`: values to append to the `LDFLAGS` variable.
        - `boost_ldlibs`: values to append to the `LDLIBS` variable.
    * *Boost.NumericBindings*:
        - `boost_numeric_bindings_cflags`: values to append to the `CXXFLAGS` variable.
        - `boost_numeric_bindings_ldflags`: values to append to the `LDFLAGS` variable.
        - `boost_numeric_bindings_ldlibs`: values to append to the `LDLIBS` variable.
    * *Boost.uBLASx*:
        - `boost_ublasx_cflags`: values to append to the `CXXFLAGS` variable.
        - `boost_ublasx_ldflags`: values to append to the `LDFLAGS` variable.
        - `boost_ublasx_ldlibs`: values to append to the `LDLIBS` variable.
    * Build system:
        - `build_cflags`: values to append to the `CXXFLAGS` variable.
        - `build_ldflags`: values to append to the `LDFLAGS` variable.
        - `build_ldlibs`: values to append to the `LDLIBS` variable.
    * *IBM CPLEX* optimizer:
        - `cplex_cflags`: values to append to the `CXXFLAGS`.
        - `cplex_ldflags`: values to append to the `LDFLAGS`.
        - `cplex_ldlibs`: values to append to the `LDLIBS`.
    * *dcsxx-commons*:
        - `dcs_commons_cflags`: values to append to the `CXXFLAGS`.
        - `dcs_commons_ldflags`: values to append to the `LDFLAGS`.
        - `dcs_commons_ldlibs`: values to append to the `LDLIBS`.
    * *dcsxx-control*:
        - `dcs_control_cflags`: values to append to the `CXXFLAGS`.
        - `dcs_control_ldflags`: values to append to the `LDFLAGS`.
        - `dcs_control_ldlibs`: values to append to the `LDLIBS`.
        - `dcs_control_qp_use_cplex`: integer value telling whether to use (1) or not (0) the CPLEX optimizer to solve quadratic programming problems.
        - `dcs_control_qp_use_gurobi`: integer value telling whether to use (1) or not (0) the GUROBI optimizer to solve quadratic programming problems.
    * *dcsxx-sysid*:
        - `dcs_sysid_cflags`: values to append to the `CXXFLAGS`.
        - `dcs_sysid_ldflags`: values to append to the `LDFLAGS`.
        - `dcs_sysid_ldlibs`: values to append to the `LDLIBS`.
    * *Fuzzylite*:
        - `fuzzylite_cflags`: values to append to the `CXXFLAGS`.
        - `fuzzylite_ldflags`: values to append to the `LDFLAGS`.
        - `fuzzylite_ldlibs`: values to append to the `LDLIBS`.
    * *Fuzzylitex*:
        - `fuzzylitex_cflags`: values to append to the `CXXFLAGS`.
        - `fuzzylitex_ldflags`: values to append to the `LDFLAGS`.
        - `fuzzylitex_ldlibs`: values to append to the `LDLIBS`.
        - `fuzzylitex_use_lapack`: integer value telling whether to use (1) or not (0) the LAPACK library to solve linear algebra problems.
    * *Java*:
        - `java_home`: point to the Java installation directory.
        - `java_jni_cflags`: values to append to the `CXXFLAGS`.
        - `java_jni_ldflags`: values to append to the `LDFLAGS`.
        - `java_jni_ldlibs`: values to append to the `LDLIBS`.
    * *JsonCpp*:
        - `json_cflags`: values to append to the `CXXFLAGS`.
        - `json_ldflags`: values to append to the `LDFLAGS`.
        - `json_ldlibs`: values to append to the `LDLIBS`.
    * *LAPACK*:
        - `lapack_cflags`: values to append to the `CXXFLAGS`.
        - `lapack_ldflags`: values to append to the `LDFLAGS`.
        - `lapack_ldlibs`: values to append to the `LDLIBS`.
    * *libvirt*:
        - `libvirt_cflags`: values to append to the `CXXFLAGS`.
        - `libvirt_ldflags`: values to append to the `LDFLAGS`.
        - `libvirt_ldlibs`: values to append to the `LDLIBS`.
    * *Prometheus*:
        - `prometheus_have_meminfo_server`: integer value telling whether to use (1) or not (0) the meminfo server to collect VM memory utilizations.
        - `prometheus_meminfo_server_port`: port number at which the meminfo server is accepting connections.
        - `prometheus_have_matlab`: integer value telling whether to use (1) or not (0) MATLAB specific code.
3. Run the `make` program to build the provided applications

            $ make clean apps


Running
-------

### Prerequisite

In addition to the compile-time prerequisites, you may also need the following software components:

* [optional] [RAIN](https://github.com/yungsters/rain-workload-toolkit) Workload toolkit (development version).
* [optional] [YCSB](https://github.com/brianfrankcooper/YCSB) benchmark.
  The exact version of YCSB varies according to the workload one wants to generate.
  Visit the YCSB project page for more details.

### Execution

#### The `sysid` Application

* Run:

    $ ./app/src/sysid {options}
* To get a complete list of all supported command-line options, run:

    $ ./app/src/sysid --help

#### The `sysmgt` Application

* Run:

    $ ./app/src/sysmgt {options}
* To get a complete list of all supported command-line options, run:

    $ ./app/src/sysmgt --help

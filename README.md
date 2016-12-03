dcsxx-testbed
=============

A set of C++ components for setting up and managing a real computing testbed.

The system under test (SUT) is managed by exploiting virtualization technologies and, in particular, by using the [libvirt](http://www.libvirt.org) library.
This testbed allows the experimenter to perform both system identification and resource management experiments

This project has been used in the experimental evaluations presented in the following articles:

> Cosimo Anglano, Massimo Canonico and Marco Guazzone.
>
> *FCMS: a Fuzzy Controller for CPU and Memory Consolidation under SLA Constraints*
>
> Concurrency and Computation: Practice and Experience, In Press, 2016.
>
> doi:[10.1002/cpe.3968](http://dx.doi.org/10.1002/cpe.3968).
>
>
> Cosimo Anglano, Massimo Canonico and Marco Guazzone
>
> *FC2Q: Exploiting Fuzzy Control in Server Consolidation for Cloud Applications with SLA Constraints*
>
> Concurrency Computat.: Pract. Exper., 27(17):4491-4514, 2015.
>
> doi: [10.1002/cpe.3410](http://dx.doi.org/10.1002/cpe.3410)
>
>
> Luca Albano, Cosimo Anglano, Massimo Canonico and Marco Guazzone
>
> *Fuzzy Q&E: Achieving QoS Guarantees and Energy Savings for Cloud Applications with Fuzzy Control*
>
> 2013 International Conference on Cloud and Green Computing (2013), Karlsruhe, Germany, pp. 159-166, 2013.
>
> doi: [10.1109/CGC.2013.31](http://dx.doi.org/10.1109/CGC.2013.31)

Please, cite this project as at least one of the following papers (BibTeX format):

    @ARTICLE{CPE:CPE3968,
        author = {Cosimo Anglano and Massimo Canonico and Marco Guazzone},
        title = {{FCMS}: a Fuzzy Controller for {CPU} and Memory Consolidation under {SLA} Constraints}
        journal = {Concurrency and Computation: Practice and Experience},
        year = {2016},
        note = {In Press},
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


Building
--------

### Prerequisites

* A modern C++98 compiler (e.g., GCC v4.8 or newer is fine)
* [Boost](http://boost.org) C++ libraries (v1.60 or newer)
* [Boost.Numeric Bindings](http://svn.boost.org/svn/boost/sandbox/numeric_bindings) library (v2 or newer)
* [Boost.uBLASx](https://github.com/sguazt/boost-ublasx) library (v1 or newer)
* [dcsxx-commons](https://github.com/sguazt/dcsxx-commons) library (v2 or newer)
* [dcsxx-control](https://github.com/sguazt/dcsxx-control) library (v2 or newer)
* [dcsxx-sysid](https://github.com/sguazt/dcsxx-sysid) library (v1 or newer)
* [fuzzylite](http://www.fuzzylite.com) fuzzy logic control library (v5 or newer)
* [fuzzylitex](http://github.com/sguazt/fuzzylitex) fuzzy logic control library (v1.1.1 or newer)
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp) C++ library for interacting with JSON (v0.6 or newer)
* [LAPACK](http://www.netlib.org/lapack/) Linear Algebra PACKage (v3.5 or newer)
* [libvirt](http://libvirt.org) virtualization API library (v1 or newer)
* ... and related prerequisites

### Compilation

1. Edit the `Makefile` file to properly set library paths, specifically:
	* `boost_header_path` is the path to the *Boost* header files; for instance:

						boost_header_path=/usr/include
	* `boost_libs_path` is the path to the *Boost* library files; for instance:

						boost_libs_path=/usr/lib64
	* `boost_numeric_bindings_home` is the base path of the *Boost.NumericBindings* library; for instance:

						boost_numeric_bindings_home=/opt/boost-numeric_bindings
	* `boost_ublasx_home` is the base path of the *Boost.uBLASx* library; for instance:

						boost_ublasx_home=/opt/boost-ublasx
	* `dcsxx_commons_home` is the base path of the *dcsxx-commons* library; for instance:

						dcsxx_commons_home=/opt/dcsxx-commons
	* `dcsxx_control_home` is the base path of the *dcsxx-control* library; for instance:

						dcsxx_control_home=/opt/dcsxx-control
	* `dcsxx_sysid_home` is the base path of the *dcsxx-sysid* library; for instance:

						dcsxx_sysid_home=/opt/dcsxx-sysid
	* `fuzzylite_header_path` is the path to the *fuzzylite* header files; for instance:

						fuzzylite_header_path=/usr/include
	* `fuzzylite_libs_path` is the path to the *fuzzylite* library files; for instance:

						fuzzylite_libs_path=/usr/lib64
	* `fuzzylitex_home` is the base path of the *fuzzylitex* library; for instance:

						fuzzylitex_home=/opt/fuzzylitex
2. If not already done, compile the *fuzzylitex* library in the related directory.

3. Run the `make` program

				$ make


Running
-------

### Prerequisite

In addition to the compile-time prerequisites, you also need:
* [optional] [RAIN](https://github.com/yungsters/rain-workload-toolkit) Workload toolkit (development version)
* [optional] [YCSB](https://github.com/brianfrankcooper/YCSB) benchmark (development version)

### Execution

* Run:

				$ ./src/sysmgt {options}
* To get a complete list of all supported command-line options, run:

				$ ./src/sysmgt --help

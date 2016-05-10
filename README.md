dcsxx-testbed
=============

A set of C++ components for setting up and managing a real computing testbed.

The system under test (SUT) is managed by exploiting virtualization technologies and, in particular, by using the libvirt library (http://www.libvirt.org).
This testbed allows the experimenter to perform both system identification and resource management experiments


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

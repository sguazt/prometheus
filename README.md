dcsxx-testbed
=============

A set of C++ components for setting up and managing a real computing testbed.

The system under test (SUT) is managed by exploiting virtualization technologies and, in particular, by using the libvirt library (http://www.libvirt.org).
This testbed allows the experimenter to perform both system identification and resource management experiments


Building
--------

### Prerequisites

* A modern C++98 compiler (e.g., GCC v4.8 or newer is fine)
* [Boost](http://boost.org) C++ libraries (v1.54 or newer)
* [Boost.uBLASx](https://github.com/sguazt/boost-ublasx) library (v1 or newer)
* [dcsxx-commons](https://github.com/sguazt/dcsxx-commons) library (v2 or newer)
* [dcsxx-control](https://github.com/sguazt/dcsxx-control) library (v2 or newer)
* [dcsxx-sysid](https://github.com/sguazt/dcsxx-sysid) library (v1 or newer)
* [libvirt](http://libvirt.org) virtualization API library (v1 or newer)
* ... and related prerequisites

### Compilation

1. Edit the `Makefile` file to properly set library paths, specifically:
	* `BOOST_HEADER_PATH` is the path to the *Boost* header files; for instance:

						BOOST_HEADER_PATH=/usr/include
	* `BOOST_LIBS_PATH` is the path to the *Boost* library files; for instance:

						BOOST_LIBS_PATH=/usr/lib64
	* `BOOST_NUMERIC_BINDINGS_HOME` is the base path of the *Boost.NumericBindings* library; for instance:

						BOOST_NUMERIC_BINDINGS_HOME=/opt/boost-numeric_bindings
	* `BOOST_UBLASX_HOME` is the base path of the *Boost.uBLASx* library; for instance:

						BOOST_UBLASX_HOME=/opt/boost-ublasx
	* `DCSXX_COMMONS_HOME` is the base path of the *dcsxx-commons* library; for instance:

						DCSXX_COMMONS_HOME=/opt/dcsxx-commons
	* `DCSXX_CONTROL_HOME` is the base path of the *dcsxx-control* library; for instance:

						DCSXX_CONTROL_HOME=/opt/dcsxx-control
	* `DCSXX_SYSID_HOME` is the base path of the *dcsxx-sysid* library; for instance:

						DCSXX_SYSID_HOME=/opt/dcsxx-sysid
	* `FUZZYLITE_HEADER_PATH` is the path to the *fuzzylite* header files; for instance:

						FUZZYLITE_HEADER_PATH=/usr/include
	* `FUZZYLITE_LIBS_PATH` is the path to the *fuzzylite* library files; for instance:

						FUZZYLITE_LIBS_PATH=/usr/lib64
2. Run the `make` program

				$ make


Running
-------

### Prerequisite

In addition to the compile-time prerequisites, you also need:
* [optional] [RAIN](https://github.com/yungsters/rain-workload-toolkit) Workload toolkit (development version, from 97f64fd83999573900d477ca9fd0899ca04e8250 commit)
* [optional] [YCSB](https://github.com/brianfrankcooper/YCSB) benchmark (development version, from 5659fc582c8280e1431ebcfa0891979f806c70ed commit)

### Execution

* Run:

				$ ./src/sysmgt {options}
* To get a complete list of all supported command-line options, run:

				$ ./src/sysmgt --help

![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Structural Simulation Toolkit (SST) Macroscale Element Library

#### Copyright (c) 2009-2020, Sandia National Laboratories
Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

---

The Structural Simulation Toolkit (SST) macroscale element library provides functionality for simulating extreme-scale applications.  Extreme scale is achieved through:

* Skeletonized endpoint models that provide accurate, but fast models of applications.
* Coarse-grained networks models that capture important traffic contention without modeling flit-level details
* Parallelization through a PDES (parallel discrete event simulation) core

The most unique functionality is dedicated to skeleton applications which run as emulated processes on a barebones virtual OS.  Each process (and thread) is modeled as a user-space thread, allowing millions of virtual simulation processes to be modeled in a single simulation.

The SST transports library in this repository provides bindings for the common network APIs like Infiniband verbs, Cray uGNI, libfabric.
Once installed, middleware libraries like MPI can be built for simulation simply be linking against the SST versions of these libraries rather than the native versions that would run over actual hardware
For more information on SST/macro, see the SST/macro [repo](git@github.com:sstsimulator/sst-macro.git).
For details on the process of skeletonization and running simulated versions of network libraries, consult the the following papers:
* [Supercomputer in a Laptop: Distributed Application and Runtime Development via Architecture Simulation](https://link.springer.com/chapter/10.1007/978-3-030-02465-9_23)
* [Compiler-Assisted Source-to-Source Skeletonization of Application Models for System Simulation](https://link.springer.com/chapter/10.1007/978-3-319-92040-5_7)

## Building SST Transports
SST Transports requires an existing install of SST/macro.
For SST/macro build instructions, see the SST/macro [repo](git@github.com:sstsimulator/sst-macro.git).
Once SST/macro installed, SST Transports is built simply by running:
````
> cmake ${SST_TRANSPORTS_SOURCE_DIR} \
  -DSSTMacro_ROOT=${SSTMACRO_INSTALL_ROOT} \
  -DCMAKE_CXX_COMPILER=${CPP_COMPILER} \
  -DCMAKE_C_COMPILER=${C_COMPILER} \
  -DCMAKE_INSTALL_PREFIX=${SST_TRANSPORTS_INSTALL_PATH}
> make install
````

### Spack build
SST Transports has a Spack package which should automatically configure and install dependencies.
After downloading [Spack](https://github.com/spack/spack), you can run `spack info sst-transports` to see options or run `spack install sst-transports` to build the library.

## Using SST Transports
After installing, libraries like MPI can be built against simulator APIs.
For example, if building a library that uses verbs, you might configure your project with:
````
> configure \
    --with-ib-verbs=${SST_TRANSPORTS_INSTALL_PATH}
````


###
To learn more, see the PDF manuals in the top-level source directory.
Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST core.

##### [LICENSE](https://github.com/sstsimulator/sst-core/blob/devel/LICENSE)

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Under the terms of Contract DE-NA0003525 with NTESS,
the U.S. Government retains certain rights in this software.


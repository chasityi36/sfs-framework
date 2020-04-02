# Project Overview

This framework implements an experimental structure from motion pipeline based on _true_ spherical images.

# Copyright and License

**sfs-framework** - Nils Hamel, Charles Papon <br >
Copyright (c) 2019-2020 DHLAB, EPFL & HES-SO Valais-Wallis

This program is licensed under the terms of the GNU GPLv3. Documentation and illustrations are licensed under the terms of the CC BY 4.0.

See [COPYRIGHT](COPYRIGHT.md) file for third parties redistribution.

# Dependencies

The _sfs-framework_ comes with the following package dependencies (Ubuntu 16.04 LTS, [Instruction](DEPEND.md)) :

* build-essential
* cmake
* libeigen3-dev

and the following external dependencies (Ubuntu 16.04 LTS, [Instructions](DEPEND.md)) :

* opencv 4.0.1 
* yaml-cpp 0.6.3

The code documentation is built using doxygen.

# Usage

```bash
mkdir build
cd build
cmake ..
make
bin/sfs-framework CONFIG_YAML_FILE
```

# Eclipse debug

```bash
cd ..
mkdir sfs-framework.git-debug
cd sfs-framework.git-debug
cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j$(nproc) ../sfs-framework.git
```



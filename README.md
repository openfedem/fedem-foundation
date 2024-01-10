<!---
  SPDX-FileCopyrightText: 2023 SAP SE

  SPDX-License-Identifier: Apache-2.0

  This file is part of FEDEM - https://openfedem.org
--->

[![REUSE status](https://api.reuse.software/badge/github.com/SAP/fedem-foundation)](https://api.reuse.software/info/github.com/SAP/fedem-foundation)

# FEDEM Foundation libraries

![Fedem Logo](cfg/FedemLogo.png)

## About this project

This project contains the source code of some application-independent libraries
that are used as building blocks by the FEDEM solvers and also the FEDEM GUI.
It is therefore consumed as a submodule both in
[fedem-solvers](https://github.com/SAP/fedem-solvers) and in
[fedem-gui](https://github.com/SAP/fedem-gui).

### Name convention

The main bulk of the libraries of this repository are named `FF*Lib` where the double-F
prefix stands for "FEDEM foundation" and the subsequent letter(s) indicates the
purpose of the library:

* FFaLib - Auxiliary classes
* FFlLib - FE part handling
* FFrLib - Result file API
* FFpLib - Post-processing of curve data
* FFlrLib - Handling of FE part results
* FFaFunctionLib - Management and evaluation of explicit functions
* FFaMathExpr - Implementation of math expression functions

The classes of the above libraries are typically named by what library they belong to,
e.g., `FFaTensor3` belongs to `FFaLib`, `FFlNode` belongs to `FFlLib`, etc.

In addition, there are two libraries `FiDeviceFunctions` and `FiUserElmPlugin` which
implements interfaces to external data files and user-defined finite elements,
respectively.

## Requirements and Setup

The build of the FEDEM foundation libraries will normally be handled by the respective
supermodules ([fedem-solvers](https://github.com/SAP/fedem-solvers) and
[fedem-gui](https://github.com/SAP/fedem-gui)). However, it is also
possible to build them separately, for instance to perform some local testing, etc.
To facilitate this, the [cmake-modules](https://github.com/SAP/cmake-modules)
repository is included via submodule reference, to provide the compiler setup.

We use the packages [googletest](https://github.com/google/googletest) and
[pFUnit](https://github.com/Goddard-Fortran-Ecosystem/pFUnit) to implement
some unit tests for the C++ and Fortran code, respectively. Therefore,
you need to set the environment variables `GTEST_ROOT` and `PFUNIT` to point to
the root path of valid installations of the googletest and pFUnit frameworks,
respectively, before executing the cmake command shown below.

On Linux, the FEDEM foundation libraries including the unit tests can be built and
executed by:

    mkdir Release; cd Release
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make check

All of the FEDEM foundation libraries contains C++ code only, except for
[FFaFunctionLib](src/FFaFunctionLib) which also contains some Fortran code (`*.f90`)
implementing various explicit mathematical functions.
This Fortran code is not compiled by default. To build with the Fortran code included,
add the command-line option `-DUSE_FORTRAN=ON` to the cmake command above.

## Contributing

This project is open to feature requests, suggestions, bug reports, etc.,
via [GitHub issues](https://github.com/SAP/cmake-modules/issues).
Contributions and feedback are encouraged and always welcome.
For more information about how to contribute,
see our [Contribution Guidelines](.github/CONTRIBUTING.md).

## Security / Disclosure

If you find any bug that may be a security problem, please follow our instructions at [in our security policy](https://github.com/SAP/fedem-foundation/security/policy) on how to report it. Please do not create GitHub issues for security-related doubts or problems.

## Code of Conduct

We as members, contributors, and leaders pledge to make participation in our community a harassment-free experience for everyone. By participating in this project, you agree to abide by its [Code of Conduct](https://github.com/SAP/.github/blob/main/CODE_OF_CONDUCT.md) at all times.

## Licensing

Copyright 2023 SAP SE or an SAP affiliate company and fedem-foundation contributors. Please see our [LICENSE](LICENSE) for copyright and license information. Detailed information including third-party components and their licensing/copyright information is available [via the REUSE tool](https://api.reuse.software/info/github.com/SAP/fedem-foundation).

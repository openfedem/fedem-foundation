<!---
  SPDX-FileCopyrightText: 2023 SAP SE

  SPDX-License-Identifier: Apache-2.0

  This file is part of FEDEM - https://openfedem.org
--->

# Unit testing of FEDEM code

We use the packages [googletest](https://github.com/google/googletest) and
[pFUnit](https://github.com/Goddard-Fortran-Ecosystem/pFUnit) to implement
some unit tests for the C++ and Fortran code, respectively, in FEDEM.

This folder contains the two source files `pFUnitExtraArg.f90` and
`pFUnitExtra.f90.in` which supplement your `pFUnit` installation to enable
passing additional command-line options to the unit test executables.

In addition, the sub-folder [pFUnit-3.3.3](pFUnit-3.3.3)
contains the sources of the package pFUnit 3, which no longer is available on
[github](https://github.com/Goddard-Fortran-Ecosystem/pFUnit).
If you are using a cmake version older than 3.12, you need to use `pFUnit 3`.
Otherwise, it is recommended to use `pFUnit 4` although `pFUnit 3` should still work.
See [below](#pfunit-3) for further details on installing the pFUnit package.

## Installing googletest and pFUnit

### googletest

On Linux, the googletest framework can be installed directly from the package manager,
e.g., for Ubuntu:

    sudo apt install libgtest-dev

For Ubuntu 20 and later, this will only install the sources of the googletest framework.
To build it, you need to proceed as follows:

    cd /usr/src/googletest
    sudo cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_GMOCK=OFF
    sudo make install

On Windows, we build and install the googletest framework from the sources:

- Visit their [github repository](https://github.com/google/googletest)
  and download the [latest release](https://github.com/google/googletest/releases/latest).
  Currently, this is [v1.14.0](https://github.com/google/googletest/releases/tag/v1.14.0).
  Download the zip'ed source code found there.

- Unzip the downloaded `googletest-1.14.0.zip` file in arbitrary location,
  e.g., `%USERPROFILE%\Fedem-src\googletest-1.14.0`.

- With Visual Studio 2019, configure googletest by running the following
  commands from a DOS shell (put this in a bat-file for the convenience):

      @echo off
      title googletest configuration
      call "%VS2019INSTALLDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64
      "%VS2019INSTALLDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ^
      -G "Visual Studio 16 2019" ^
      -S %USERPROFILE%\Fedem-src\googletest-1.14.0 ^
      -B %USERPROFILE%\Fedem-build\googletest ^
      -DCMAKE_INSTALL_PREFIX=C:\googletest ^
      -DBUILD_GMOCK=OFF ^
      -Dgtest_force_shared_crt=ON
      pause

- Open the generated solution file `%USERPROFILE%\Fedem-build\googletest\googletest-distribution.sln`
  in Visual Studio and build the `INSTALL` target for `Release` configuration.

### pFUnit 3

If your system uses a cmake version older than 3.12, you need to use `pFUnit 3`
because the build system of `pFUnit 4` requires cmake version 3.12 or newer.
In that case, you can build and install the package from the sources provided here.
Do do this on Linux, run the following commands:

    mkdir pFUnit-3.3.3/Release
    cd pFUnit-3.3.3/Release
    export PFUNIT=$HOME/pFUnit3
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PFUNIT
    make install

This will install the pFUnit package in the folder `$HOME/pFUnit3`.
See also the [pFUnit-3.3.3/README.md](pFUnit-3.3.3/README.md) file
for further details.

The pFUnit 3 package can also be used on Windows
(we have not yet tested pFUnit 4 on the Windows platform).

- Assuming Visual Studio 2019 and Intel&reg; Fortran Compilers,
  run the following commands from a DOS shell to configure pFUnit 3
  (put this in a bat-file for the convenience):

      @echo off
      title pFUnit configuration
      set PFUNIT=C:\pFUnit3
      call "C:\Program Files (x86)\Intel\oneAPI\setvars.bat" intel64 vs2019
      "%VS2019INSTALLDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ^
      -G "Visual Studio 16 2019" ^
      -S %USERPROFILE%\Fedem-src\fedem-foundation\pFUnit\pFUnit-3.3.3 ^
      -B %USERPROFILE%\Fedem-build\pFUnit3 ^
      -DCMAKE_INSTALL_PREFIX=%PFUNIT%
      pause

- Open the generated solution file `%USERPROFILE%\Fedem-build\pFUnit3\pFUnit.sln`
  in Visual Studio and build the `INSTALL` target for `Release` configuration.
  This will build and install pFUnit 3 under `C:\pFUnit3`.

### pFUnit 4

To build and install pFUnit 4 on Linux (requires cmake 3.12 or newer),
proceed as follows:

- Download [version 4.1.14](https://github.com/Goddard-Fortran-Ecosystem/pFUnit/releases/tag/v4.1.14)
  from their [github repository](https://github.com/Goddard-Fortran-Ecosystem/pFUnit).
  Make sure to select the `pFUnit-*.tar` link, and not the auto-generated `Source code`
  `(zip)` or `(tar.gz)` links which do not contain the submodules.

  The newer releases of pFUnit 4 (i.e., version 4.2 to 4.8) do not yet work with FEDEM.
  Porting the FEDEM unit tests to the latest version of pFUnit 4 will be done later.
  There exist a [version 4.1.15](https://github.com/Goddard-Fortran-Ecosystem/pFUnit/releases/tag/v4.1.15),
  however, without a tar-ball including the submodules so we have not tried that version.
  You may consider using version 4.1.15 together with the submodule sources that come with version 4.1.14.

- Extract the sources in arbitrary location, e.g., in a bash shell, run the commands:

      cd ~/Fedem-src
      tar xfv ~/Downloads/pfunit-4.1.14.tar

- Build and install on Linux:

      mkdir ~/Fedem-src/pFUnit-4.1.14/Release
      cd ~/Fedem-src/pFUnit-4.1.14/Release
      cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME \
               -DSKIP_MPI=YES -DSKIP_OPENMP=YES -DSKIP_FHAMCREST=YES
      make install

This will install the pFUnit package in the folder `$HOME/PFUNIT-4.1`.
See also [Building and installing pFUnit](https://github.com/Goddard-Fortran-Ecosystem/pFUnit?tab=readme-ov-file#building-and-installing-pfunit)
on their github site for further details.

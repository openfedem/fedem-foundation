<!---
  SPDX-FileCopyrightText: 2023 SAP SE

  SPDX-License-Identifier: Apache-2.0

  This file is part of FEDEM - https://openfedem.org
--->

# FEDEM Admin module

The files in this directory are used to administer the versioning
of the FEDEM program modules. It consists of the following set of files:

- CMakeLists.txt - Cmake project file, generating platform dependent Makefile
- FedemAdmin.[CH] - The source code that is compiled into the Admin library
- version.h - Text file containing the current version of Fedem,
              in the format `R<major>.<minor>[.<patch>]`.
              The major, minor and patch values are updated manually.

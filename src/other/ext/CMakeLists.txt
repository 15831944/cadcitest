#-----------------------------------------------------------------------
# Local Copies of External Libraries
#
# BRL-CAD depends on a variety of external libraries and tools -
# rather than fail if those requirements are not satisfied, we build
# local copies at need.
#
# There are three overall approaches to the handling of these
# dependencies:
#
# 1.  Auto - detect system libraries and use them if suitable,
#     otherwise build and use the local copy.  This is the default
#     approach.
#
# 2.  Bundled - regardless of system conditions, build and use all
#     bundled libraries.
#
# 3.  System - fail to build if the system libraries do not satisfy
#     requirements.  This is primarily useful for distributions that
#     want to ensure packages are using external libraries.
#
# In addition to the broad toplevel control, individual libraries can
# also be overridden - for example, if the toplevel setting is for
# Bundled libs, it is still possible to request a system library in
# individual cases.
#
#-----------------------------------------------------------------------

# Yuck - OpenBSD has it's own naming convention for executables and
# libraries, which is messing up both our copying logic and the
# find_package results.  Will have to try to handle it - beginning
# to wonder if we need a function to encapsulate the platform
# naming conventions to avoid all these boilerplate patterns...

project(BDEPS)

cmake_minimum_required(VERSION 3.12)

# install(CODE) uses generator expressions - requires 3.14 or newer.
# We want it to be on, but until we can bump our minimum requirement
# set it to OLD to make sure we don't break anything.
if (POLICY CMP0087)
  cmake_policy(SET CMP0087 OLD)
endif (POLICY CMP0087)

# Find the executable extension, if there is one
get_filename_component(EXE_EXT "${CMAKE_COMMAND}" EXT)

# If targets are built, we want to group them in build tools that support
# doing so.
function(SetTargetFolder targetname folder)
  if(TARGET ${targetname})
    set_target_properties(${targetname} PROPERTIES FOLDER "${folder}")
  endif(TARGET ${targetname})
endfunction(SetTargetFolder)

# If defined, get feature variables from parent
if (EXISTS ${CMAKE_BINARY_DIR}/brlcad_vars.cmake)
  include(${CMAKE_BINARY_DIR}/brlcad_vars.cmake)
endif (EXISTS ${CMAKE_BINARY_DIR}/brlcad_vars.cmake)

# We need some extra functionality for this...
set(BDEPS_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH "${BDEPS_CMAKE_DIR};${CMAKE_MODULE_PATH}")

# We do not want to search in our install target; on a second
# configure pass the outputs of previous builds will be detected
# and mess up the results...
set(CMAKE_SYSTEM_IGNORE_PATH "${CMAKE_INSTALL_PREFIX}")

# We will build up a list of all include dirs coming from 3rd party
# projects, so we can include them with -isystem
unset(SYS_INCLUDE_PATTERNS CACHE)

# For the individual builds to be done here, we need to place their outputs to
# standard locations in the CMAKE_BINARY_DIR.  However, that target is not
# always unique - for multiconfig build systems, we need to define per-config
# output targets.  The $<CONFIG> generator expression lets us set up per-config
# paths, but as $<CONFIG> is ALSO defined for non-multiconfig builds when
# CMAKE_BUILD_TYPE is set we need to define $<CONFIG> based paths ONLY when we
# have CMAKE_CONFIGURATION_TYPES set (which indicates a multiconfig generator
# is in use.)
if (CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BINARY_ROOT "${CMAKE_BINARY_DIR}/$<CONFIG>")
else (CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BINARY_ROOT "${CMAKE_BINARY_DIR}")
endif (CMAKE_CONFIGURATION_TYPES)

# We execute the install process of ExternalProjects and then copy the outputs
# we need into the BRL-CAD structure.  For cleanup convenience, use a common
# install target that will hold all of the various subbuild outputs.
set(CMAKE_BINARY_INSTALL_ROOT "${CMAKE_BINARY_ROOT}/extinstall")
DISTCLEAN("${CMAKE_BINARY_ROOT}/extinstall")

#---------------------------------------------------------------------
# Define relative install locations and output directories.  Don't set
# these if they have already been set by some other means (like a
# higher level CMakeLists.txt file including this one).
# For output directories - where built library and executable
# files will be placed after building but prior to install.  The
# necessary variables change between single and multi configuration
# build systems, so it is necessary to handle both cases on a
# conditional basis.

include(Path_Setup)

#---------------------------------------------------------------------
# RPath handling is what allows us to run binaries from the build
# directory without having to set LD_LIBRARY_PATH, and what allows
# for relocatable binary packages on Linux and similar platforms.
# BRL-CAD's needs are rather complex when it comes to RPath, so we
# define a series of functions:
if (NOT COMMAND std_build_rpath)
  include(RPath_Setup)
endif (NOT COMMAND std_build_rpath)

# We want the full RPATH set in the build tree so we can run programs without
# needing to set LD_LIBRARY_PATH
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# We DON'T want the final install directory RPATH set in the build directory
# - it should only be set to the installation value when actually installed.
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Add the automatically determined parts of the RPATH which point to
# directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set RPATH value to use when installing.  This should be set to always
# prefer the version in the installed path when possible, but fall back on a
# location relative to the loading file's path if the installed version is
# not present.  How to do so is platform specific.
relative_rpath(RELPATH)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}${RELPATH}")

# 3rd party build outputs use a CMAKE_RPATH_BUILD setting that is different
# from the "vanilla" management used with other BRL-CAD targets.  The
# function below encapsulates the setup needed for CMAKE_RPATH_BUILD
ext_build_rpath()


#---------------------------------------------------------------------
# By default, changes in src/superbuild files won't trigger a rebuild.  Setting
# this to 1 changes that, but at the expense of running the build step every
# time.  This may be fast if there is nothing to update in the project, but it
# will be slower than skipping the step completely.
if (NOT DEFINED EXT_BUILD_ALWAYS)
  set(EXT_BUILD_ALWAYS 0)
endif (NOT DEFINED EXT_BUILD_ALWAYS)

# The verbosity of the ExtProject stages is controlled by log options - they
# can either output to the console or to log files.  We establish a top level
# control for this - by default these outputs are logged, but if the verbose
# flag is set they will be printed to the console during build (useful in
# scenarios such as CI build runners, where it is advantageous to capture as
# much output as possible in the top level log.
if (EXT_BUILD_VERBOSE)
  set(EXT_BUILD_QUIET OFF)
else (EXT_BUILD_VERBOSE)
  set(EXT_BUILD_QUIET ON)
endif (EXT_BUILD_VERBOSE)

# Use this variable to key both CMake messages and tools like rpath_replace
if(NOT DEFINED EXTPROJ_VERBOSE)
  set(EXTPROJ_VERBOSE 0)
endif(NOT DEFINED EXTPROJ_VERBOSE)

# Tool for replacing rpath values in build files.  CMAKE_BUILD_RPATH must be
# set before configuring this file.
configure_file(${BDEPS_CMAKE_DIR}/rpath_replace.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/rpath_replace.cxx)
add_executable(rpath_replace ${CMAKE_CURRENT_BINARY_DIR}/rpath_replace.cxx)
set_target_properties(rpath_replace PROPERTIES FOLDER "Compilation Utilities")
DISTCLEAN(${CMAKE_CURRENT_BINARY_DIR}/rpath_replace.cxx)

# Independent projects with their own build systems (potentially not CMake
# based) need ExternalProject_Add
include(ExternalProject)

# Custom patch utility to replace the build directory path with the install
# directory path in text files - make sure CMAKE_BINARY_DIR and
# CMAKE_INSTALL_PREFIX are finalized before generating this file!
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/buildpath_replace.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/buildpath_replace.cxx)
add_executable(buildpath_replace ${CMAKE_CURRENT_BINARY_DIR}/buildpath_replace.cxx)
set_target_properties(buildpath_replace PROPERTIES FOLDER "Compilation Utilities")
# TODO - remove once we require CMake >=3.14
# Until we can require CMake 3.14 for generator expressions in install(CODE), force
# the location of buildpath_replace to a known location in all cases
set_property(TARGET buildpath_replace PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeTmp)
if(CMAKE_CONFIGURATION_TYPES)
  foreach(ct ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${ct}" CTU)
    set_property(TARGET buildpath_replace PROPERTY RUNTIME_OUTPUT_DIRECTORY_${CTU} ${CMAKE_BINARY_DIR}/CMakeTmp)
  endforeach(ct ${CMAKE_CONFIGURATION_TYPES})
endif(CMAKE_CONFIGURATION_TYPES)
DISTCLEAN(${CMAKE_CURRENT_BINARY_DIR}/buildpath_replace.cxx)

# patchelf is used to set/update RPATH values on certain platforms.
# Build a copy if we don't already have it and we're not on Windows
# or Mac.  If we're going to use it, it needs to be defined before
# we start setting up the external targets.
find_program(PATCHELF_EXECUTABLE patchelf)
if (NOT PATCHELF_EXECUTABLE)
  add_subdirectory(patchelf)
  set(PATCHELF_EXECUTABLE "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${BIN_DIR}/patchelf")
  if (TARGET patchelf)
    set_target_properties(patchelf PROPERTIES FOLDER "Compilation Utilities")
  endif (TARGET patchelf)
endif (NOT PATCHELF_EXECUTABLE)
set(PATCHELF_FILES
  patchelf/CMakeLists.txt
  patchelf/COPYING
  patchelf/patchelf.cc
  patchelf/elf.h
  )
CMAKEFILES(${PATCHELF_FILES})

# BRL-CAD defines some additional custom targets used to manage the outputs of
# ExternalProject_Add targets.
# Note- make sure CMAKE_BINARY_DIR and CMAKE_INSTALL_PREFIX are finalized
# BEFORE including this file!
include(ExternalProject_Target)

# External build outputs can be verbose - capture them to files instead of
# outputting to console
#set(LOG_OPTS
#  LOG_DIR "${CMAKE_BINARY_DIR}/CMakeFiles/ExternalProject_logs"
#  LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON
#  LOG_MERGED_STDOUTERR ON LOG_OUTPUT_ON_FAILURE ON
#  )

set(LOG_OPTS)

# Load some CMake macros to handle the special case of third party libraries.
include(ThirdParty)
include(ThirdPartyExecutable)
include(ThirdPartyHeaders)

# In case we need to pass some sort of reasonable parallel
# flag to a build system, check our processor count
include(ProcessorCount)
ProcessorCount(N)
math(EXPR pcnt "${N} / 2")
if (NOT pcnt)
  set(pcnt 1)
endif (NOT pcnt)


# With MSVC, some of the non-CMake subbuilds are going to need the vcvars bat
# file to set up the environment.
if(MSVC)

  get_filename_component(COMPILER_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  get_filename_component(COMPILER_ROOT "${COMPILER_DIR}" NAME)
  set(VCVARS_BAT "${COMPILER_DIR}/vcvars${COMPILER_ROOT}.bat")
  if(NOT EXISTS "${VCVARS_BAT}")
    # Try the VS2017 location (https://stackoverflow.com/q/43372235)
    set(COMPILER_ROOT "")
    foreach(DIRS RANGE 0 5)
      get_filename_component(COMPILER_DIR "${COMPILER_DIR}" DIRECTORY)
    endforeach(DIRS RANGE 0 5)
    set(COMPILER_DIR "${COMPILER_DIR}/Auxiliary/Build")
    if(CMAKE_CL_64)
      set(VCVARS_BAT "${COMPILER_DIR}/vcvars64.bat")
    else(CMAKE_CL_64)
      set(VCVARS_BAT "${COMPILER_DIR}/vcvars32.bat")
    endif(CMAKE_CL_64)
    if(NOT EXISTS "${VCVARS_BAT}")
      message(FATAL_ERROR "Could not find vcvars bat file in ${COMPILER_DIR}")
    endif(NOT EXISTS "${VCVARS_BAT}")
  endif(NOT EXISTS "${VCVARS_BAT}")

endif(MSVC)

# OpenBSD has its own naming conventions.  Set a platform variable based on
# the OS name so we can test for it succinctly.
if ("${CMAKE_SYSTEM}" MATCHES ".*OpenBSD.*")
  set(OPENBSD ON)
endif ("${CMAKE_SYSTEM}" MATCHES ".*OpenBSD.*")

# TODO - need to set these based on the feature flags...
set(BRLCAD_LEVEL2 ON)
set(BRLCAD_LEVEL3 ON)

# TODO - in stand-alone mode, these need to be set to enable the builds...
#set(BRLCAD_ENABLE_STEP ON)
#set(BRLCAD_ENABLE_GDAL ON)
#set(BRLCAD_ENABLE_TCL ON)
#set(BRLCAD_ENABLE_TK ON)

###############################################################################
# BRL-CAD and some of its dependencies require the lemon, re2c and perplex
# tools for compilation.  They are not installed, but must be available.  We
# ensure they can be found by building them up front if not present on the
# system.
###############################################################################

include(${CMAKE_CURRENT_SOURCE_DIR}/perplex.cmake)

###############################################################################
# Build logic is broken out per-library, but the ordering is important.  Some
# libraries will depend on others listed here (for example, we want openNURBS
# to use our bundled zlib if it is enabled.) Developers adding, reordering or
# removing dependencies here need to make sure they are aware of impact they
# may be having on other external projects in other files.
###############################################################################

# zlib compression/decompression library
include(${CMAKE_CURRENT_SOURCE_DIR}/zlib.cmake)

# libregex regular expression matching
include(${CMAKE_CURRENT_SOURCE_DIR}/regex.cmake)

# netpbm library - support for pnm,ppm,pbm, etc. image files
include(${CMAKE_CURRENT_SOURCE_DIR}/netpbm.cmake)

# libpng - Portable Network Graphics image file support
include(${CMAKE_CURRENT_SOURCE_DIR}/png.cmake)

# STEPcode - support for reading and writing STEP files
include(${CMAKE_CURRENT_SOURCE_DIR}/stepcode.cmake)

## libbson - for binary attributes, always use this local version
#include(${CMAKE_CURRENT_SOURCE_DIR}/bson.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/bson.dist)

# PROJ4 - generic coordinate transformation
include(${CMAKE_CURRENT_SOURCE_DIR}/proj4.cmake)

# GDAL -  translator library for raster and vector geospatial data formats
include(${CMAKE_CURRENT_SOURCE_DIR}/gdal.cmake)

# TCL - scripting language.  For Tcl/Tk builds we want
# static lib building on so we get the stub libraries.
set(BSL_CACHE ${BUILD_STATIC_LIBS})
set(BUILD_STATIC_LIBS ON)
if (BRLCAD_ENABLE_TK)
  # For FindTCL.cmake
  set(TCL_ENABLE_TK ON CACHE BOOL "enable tk")
endif (BRLCAD_ENABLE_TK)
include(${CMAKE_CURRENT_SOURCE_DIR}/tcl.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tk.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/itcl.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/itk.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/iwidgets.cmake)
set(BUILD_STATIC_LIBS ${BSL_CACHE})

# A lot of code depends on knowing about Tk being active,
# so we set a flag in the configuration header to pass
# on that information.
if (COMMAND CONFIG_H_APPEND)
  CONFIG_H_APPEND(BRLCAD "#cmakedefine HAVE_TK\n")
endif (COMMAND CONFIG_H_APPEND)


# The above libraries are all compilation targets with libraries, and so have
# build outputs that must be distributed with BRL-CAD.  The following are
# header-only libraries, and as such may remain in place - they are needed only
# for building.  Hence, the only question we need to answer for these are
# whether we are using a system version or we need to define the variables for
# local including.
Third_Party_Headers(Eigen EIGEN FIND_NAME Eigen3)
include("${CMAKE_CURRENT_SOURCE_DIR}/Eigen.dist")
CMAKEFILES_IN_DIR(Eigen_ignore_files Eigen)
set(SYS_INCLUDE_PATTERNS ${SYS_INCLUDE_PATTERNS} Eigen CACHE STRING "Bundled system include dirs" FORCE)


std_build_rpath()

if (COMMAND CMAKEFILES)
  CMAKEFILES(
    CMakeLists.txt
    CMake/ExternalProject_Target.cmake
    CMake/FindBSON.cmake
    CMake/FindITCL.cmake
    CMake/FindLEMON.cmake
    CMake/FindNETPBM.cmake
    CMake/FindOPENNURBS.cmake
    CMake/FindPERPLEX.cmake
    CMake/FindPOLY2TRI.cmake
    CMake/FindPROJ4.cmake
    CMake/FindRE2C.cmake
    CMake/FindREGEX.cmake
    CMake/FindSTEPCODE.cmake
    CMake/FindTCL.cmake
    CMake/FindUTAHRLE.cmake
    CMake/Path_Setup.cmake
    CMake/RPath_Setup.cmake
    CMake/ThirdParty.cmake
    CMake/ThirdPartyExecutable.cmake
    CMake/ThirdPartyHeaders.cmake
    CMake/buildpath_replace.cxx.in
    CMake/rpath_replace.cxx.in
    CMake/tcl_replace.cxx.in
    Eigen.dist
    bson.cmake
    bson.dist
    gdal.cmake
    gdal.dist
    itcl.cmake
    itcl3.dist
    itk.cmake
    itk3.dist
    iwidgets.cmake
    iwidgets.dist
    netpbm.cmake
    netpbm.dist
    perplex.cmake
    perplex.dist
    png.cmake
    png.dist
    proj-4.dist
    proj4.cmake
    regex.cmake
    regex.dist
    stepcode.cmake
    stepcode.dist
    tcl.cmake
    tcl.dist
    tk.cmake
    tk.dist
    zlib.cmake
    zlib.dist
    )
endif (COMMAND CMAKEFILES)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8


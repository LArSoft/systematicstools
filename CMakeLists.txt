# ======================================================================
#  systematicstools main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/systematicstools/ups/setup_for_development <-d|-p>
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================


# use cmake 3.3 or later
cmake_minimum_required (VERSION 3.20 FATAL_ERROR)

find_package(cetmodules REQUIRED)
project(systematicstools VERSION 01.03.01 LANGUAGES CXX)

# cetbuildtools contains our cmake modules

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic )

cet_report_compiler_flags(REPORT_THRESHOLD VERBOSE)

# these are minimum required versions, not the actual product versions
find_package(art_root_io REQUIRED EXPORT)
find_package(canvas_root_io REQUIRED EXPORT)
find_package(art REQUIRED EXPORT)
find_package(canvas REQUIRED EXPORT)
find_package(messagefacility REQUIRED EXPORT)
find_package(fhiclcpp REQUIRED EXPORT)
find_package(cetlib REQUIRED EXPORT)
find_package(cetlib_except REQUIRED EXPORT)
find_package(ROOT COMPONENTS Core Hist Matrix REQUIRED EXPORT)
find_package(Boost COMPONENTS headers filesystem REQUIRED EXPORT)
find_package(CLHEP COMPONENTS Random REQUIRED EXPORT)

# macros for dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)

# ADD SOURCE CODE SUBDIRECTORIES HERE
add_subdirectory(systematicstools)

SET(SYSTTOOLS_LIBS
  systematicstools_interface
  systematicstools_interpreters
  systematicstools_utility)


# tests
add_subdirectory(test)

# ups - table and config files

# packaging utility

cet_cmake_config()

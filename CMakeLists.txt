cmake_minimum_required(VERSION 3.9)
project(eComInit)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(GNUInstallDirs)
include(FindPkgConfig)
include(LemFlex)
include(MakeHeader)
include(WSRPCGen)

set(HDR ${PROJECT_SOURCE_DIR}/hdr)
set(SHARESRC ${PROJECT_SOURCE_DIR}/share)
set(VENDORSRC ${PROJECT_SOURCE_DIR}/vendor)

macro(SetBoth base value)
  set(${base} ${value})
  set(${base}_${value} TRUE)
endmacro()

if(UNIX AND NOT APPLE)
  if(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
    set(ECI_PLAT_LINUX TRUE)
    SetBoth(ECI_EVENT_DRIVER Poll)
    add_definitions("-D_GNU_SOURCE")
  elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*|FreeBSD")
    set(ECI_PLAT_BSD TRUE)
    SetBoth(ECI_EVENT_DRIVER KQueue)
    set(FREEBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
    set(ECI_PLAT_BSD TRUE)
    SetBoth(ECI_EVENT_DRIVER KQueue)
    set(NETBSD TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(ECI_PLAT_BSD TRUE)
    SetBoth(ECI_EVENT_DRIVER KQueue)
    set(OPENBSD TRUE)
    # no credential passing over Unix datagram sockets in OpenBSD
    set(ECI_SD_NOTIFY_TYPE "stream")
  elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*")
    set(ECI_PLAT_BSD TRUE)
    SetBoth(ECI_EVENT_DRIVER KQueue)
    set(DRAGONFLY TRUE)
  elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX")
    set(ECI_PLAT_HPUX TRUE)
    SetBoth(ECI_EVENT_DRIVER Poll)
    # no mdoc
    set(ECI_BUILD_MANUAL FALSE)
    # C++ compiler too old
    set(ECI_ENABLE_TVISION FALSE)
    # no peer authentication at all that I know of, at least in 11.11
    set(ECI_SD_NOTIFY_TYPE insecure)
    # make _r functions available
    add_definitions(-D_REENTRANT=1)
  endif()
elseif(APPLE)
  set(ECI_PLAT_BSD TRUE)
  # no credential passing over Unix datagram sockets in OS X?
  set(ECI_SD_NOTIFY_TYPE FALSE)
  set(OSX TRUE)
endif()

macro(FIfUnset flag enable)
    if(NOT DEFINED ${flag})
        set(${flag} ${enable})
    endif()
endmacro(FIfUnset)

find_package(Threads REQUIRED)

FIfUnset(ECI_BUILD_MANUAL TRUE)
FIfUnset(ECI_ENABLE_TVISION FALSE)
FIfUnset(ECI_SD_NOTIFY_TYPE "datagram")

add_subdirectory(vendor/libucl)
add_subdirectory(vendor/lemon)
if(ECI_ENABLE_TVISION)
add_subdirectory(vendor/tvision)
endif()

if(ECI_BUILD_MANUAL)
add_subdirectory(doc/manual)
endif()

add_subdirectory(lib)

add_subdirectory(cmd)

function(FShow name flag)
        message("  ${name}: ${${flag}}")
endfunction(FShow)

message("Feature settings:")
FShow("Build manual pages" ECI_BUILD_MANUAL)
FShow("SystemD-style notification interface kind" ECI_SD_NOTIFY_TYPE)
FShow("TVision frontend" ECI_ENABLE_TVISION)
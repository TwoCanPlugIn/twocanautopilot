# ---------------------------------------------------------------------------
# Author:      Steven Adler based on (jongough aka. Jon Gough) Copyright: License:     wxWidgets License
# ---------------------------------------------------------------------------

## ----- When changing this file do NOT change the order in which sections occur        ----- ##
## ----- Changes should only be made between the section blocks that identify where     ----- ##
## ----- these changes should be. The whole configuration process relies on this        ----- ##
## ----- sequence to be successful                                                      ----- ##
##

##
## ----- Modify section below to include all the details for your plugin ----- ##
##

set(CMLOC "CMakeLists: ")

# define minimum cmake version
cmake_minimum_required(VERSION 3.1.1)
if(COMMAND cmake_policy)
    if(POLICY CMP0043)
        cmake_policy(SET CMP0043 NEW)
    endif(POLICY CMP0043)
    cmake_policy(SET CMP0048 NEW)
    if(POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW)
    endif(POLICY CMP0077)
endif(COMMAND cmake_policy)


# define plugin name, owner and versions
set(VERBOSE_NAME "TwoCan Autopilot")
set(COMMON_NAME "TwoCan Autopilot")
set(TITLE_NAME "TWOCANAUTOPILOT")
set(PACKAGE_CONTACT "twocanplugin@hotmail.com")
set(PACKAGE "twocan_autopilot")
set(SHORT_DESCRIPTION "TwoCan Autopilot, a rudimentary autopilot controller")
set(LONG_DESCRIPTION "TwoCan Autopilot, a rudiomentary autopilot controller that uses the TwoCan plugin to communicate over NMEA 2000 networks with Raymarine, Garmin, B&G and Simrad autopilot computers") 
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_PATCH "1")
set(VERSION_TWEAK "0")
set(VERSION_DATE "31/01/2022")
set(OCPN_MIN_VERSION "ov50")
set(OCPN_API_VERSION_MAJOR "1")
set(OCPN_API_VERSION_MINOR "17")
set(TP_COMMENT "Release for O5 using CI")

set(PARENT "opencpn")

# The next line allows for setting up your own local webserver with git on it for testing purposes, the default is github.com
set(GIT_REPOSITORY_SERVER "github.com")

# Specifies cloudsmith user and repository names (note suffixes for repositories production, beta & alpha)
set(PROD "prod") # Standard Repos
set(BETA "beta") # Standard Repos
set(ALPHA "alpha") # Standard Repos

set(CLOUDSMITH_USER "steven-adler")
# set this if your cloudsmith repository name does not match your git repository name 
set(CLOUDSMITH_BASE_REPOSITORY "twocanautopilot")

set(XML_INFO_URL "")
set(XML_SUMMARY ${SHORT_DESCRIPTION})
set(XML_DESCRIPTION ${LONG_DESCRIPTION})

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

## Uses SVG graphics for the toolbar icons
option(PLUGIN_USE_SVG "Use SVG graphics" ON)

set(CMAKE_CXX_STANDARD 17)

# Prefer libGL.so to libOpenGL.so, see CMP0072
set(OpenGL_GL_PREFERENCE "LEGACY")
# Don't use local version of GLU library
set(USE_LOCAL_GLU FALSE)
option(USE_GL "Enable OpenGL support" OFF)
message(STATUS "${CMLOC}USE_GL: ${USE_GL}")

## Define the build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build" FORCE)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "" "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
message(STATUS "${CMLOC}Build type: ${CMAKE_BUILD_TYPE}")

##
## ----- The statements below are used to setup standard variables that are required by the CMAKE process - do not remove ----- ##
##

project(${PACKAGE})

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;")

include("PluginSetup")

##
## ----- Modify section below if there are special requirements for the plugin ----- ##
##

set(CMAKE_VERBOSE_MAKEFILE ON)

option(Plugin_CXX11 "Use c++11" OFF)


##
## ----- do not change next section - needed to configure build process ----- ##
##

include("PluginConfigure")

##
## ----- Change below to match project requirements for source, headers, etc. ----- ##
##

## TwoCan Autopilot Plugin does not understand/use this definition
##add_definitions(-DUSE_S57)

## TwoCan Autopilot Plugin uses SVG for toolbar icons
add_definitions(-DPLUGIN_USE_SVG)

## JSON used for plugin messaging
if(APPLE)
    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" OFF)
else(APPLE)
    option(PI_JSON_SCHEMA_VALIDATOR "Use JSON Schema validator" ON)
endif(APPLE)

## Need following for ocpn_plugin.h
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_SOURCE_DIR}/libs/ocpn-api)
## Need following for wxJSON
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/wxJSON/include)
## Need following for NMEA 183 Parsing
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/nmea183/inc)
## And the project include dirs
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/inc)

SET(SOURCES src/twocanautopilot_images.cpp
            src/twocanautopilot_plugin.cpp
            src/twocanautopilot_dialog.cpp
            src/twocanautopilot_dialogbase.cpp
)


SET(JSON
    libs/wxJSON/src/jsonval.cpp
    libs/wxJSON/src/jsonreader.cpp
    libs/wxJSON/src/jsonwriter.cpp
)

SET(NMEA183_SRC nmea183/src/apb.cpp
                nmea183/src/bod.cpp
                nmea183/src/bwr.cpp
                nmea183/src/dbt.cpp
                nmea183/src/dpt.cpp
                nmea183/src/dsc.cpp
                nmea183/src/dse.cpp
                nmea183/src/expid.cpp
                nmea183/src/gga.cpp
                nmea183/src/gll.cpp
                nmea183/src/gpwpl.cpp
                nmea183/src/gsv.cpp
                nmea183/src/hdg.cpp
                nmea183/src/hdm.cpp
                nmea183/src/hdt.cpp
                nmea183/src/hexvalue.cpp
                nmea183/src/lat.cpp
                nmea183/src/latlong.cpp
                nmea183/src/long.cpp
                nmea183/src/mob.cpp
                nmea183/src/mtw.cpp
                nmea183/src/mwv.cpp
                nmea183/src/nmea0183.cpp
                nmea183/src/response.cpp
                nmea183/src/rmb.cpp
                nmea183/src/rmc.cpp
                nmea183/src/rot.cpp
                nmea183/src/rpm.cpp
                nmea183/src/rsa.cpp
                nmea183/src/rte.cpp
                nmea183/src/sentence.cpp
                nmea183/src/talkerid.cpp
                nmea183/src/vdm.cpp
                nmea183/src/vdr.cpp
                nmea183/src/vhw.cpp
                nmea183/src/vlw.cpp
                nmea183/src/vtg.cpp
                nmea183/src/wcv.cpp
                nmea183/src/wpl.cpp
                nmea183/src/xdr.cpp
                nmea183/src/xte.cpp
                nmea183/src/zda.cpp)

SET(NMEA183_INC nmea183/inc/apb.hpp
                nmea183/inc/bod.hpp
                nmea183/inc/bwr.hpp
                nmea183/inc/dbt.hpp
                nmea183/inc/dpt.hpp
                nmea183/inc/dsc.hpp
                nmea183/inc/dse.hpp
                nmea183/inc/gga.hpp
                nmea183/inc/gll.hpp
                nmea183/inc/gpwpl.hpp
                nmea183/inc/gsv.hpp
                nmea183/inc/hdg.hpp
                nmea183/inc/hdm.hpp
                nmea183/inc/hdt.hpp
                nmea183/inc/latlong.hpp
                nmea183/inc/mob.hpp
                nmea183/inc/mtw.hpp
                nmea183/inc/mwv.hpp
                nmea183/inc/nmea0183.h
                nmea183/inc/nmea0183.hpp
                nmea183/inc/response.hpp
                nmea183/inc/rmb.hpp
                nmea183/inc/rmc.hpp
                nmea183/inc/rot.hpp
                nmea183/inc/rpm.hpp
                nmea183/inc/rsa.hpp
                nmea183/inc/rte.hpp
                nmea183/inc/satinfo.h
                nmea183/inc/sentence.hpp
                nmea183/inc/vdm.hpp
                nmea183/inc/vdr.hpp
                nmea183/inc/vhw.hpp
                nmea183/inc/vlw.hpp
                nmea183/inc/vtg.hpp
                nmea183/inc/wcv.hpp
                nmea183/inc/wpl.hpp
                nmea183/inc/xdr.hpp
                nmea183/inc/xte.hpp
                nmea183/inc/zda.hpp)


TARGET_SOURCES(${PACKAGE_NAME} PUBLIC ${SOURCES} ${JSON} ${NMEA183_SRC} ${NMEA183_INC})

##
## ----- do not change next section - needed to configure build process ----- ##
##

include("PluginInstall")
include("PluginLocalization")
include("PluginPackage")


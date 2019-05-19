# OpenSTA, Static Timing Analyzer
# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.9)

project(STA VERSION 2.0.15)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules CACHE STRING "CMake module path")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

set(STA_HOME ${PROJECT_SOURCE_DIR})
message(STATUS "STA version: ${PROJECT_VERSION}")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

# Compiler specific options.
if (CMAKE_CXX_COMPILER_ID MATCHES AppleClang|Clang)
  set(STA_COMPILE_OPTIONS -Wall -Wcast-qual -Wunused-parameter -Wredundant-decls -Wno-deprecated-register)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(STA_COMPILE_OPTIONS -Wall -Wcast-qual -Wunused-parameter -Wredundant-decls)
endif()

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build CXX_FLAGS: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message(STATUS "STA CXX_FLAGS: ${STA_COMPILE_OPTIONS}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

################################################################
#
# Source files.
#
################################################################

set(STA_SOURCE
  app/StaMain.cc
  app/StaApp_wrap.cc
  app/TclInitVar.cc
  app/StaApp_wrap.cc
  
  dcalc/ArcDelayCalc.cc
  dcalc/ArnoldiDelayCalc.cc
  dcalc/ArnoldiReduce.cc
  dcalc/DcalcAnalysisPt.cc
  dcalc/DelayCalc.cc
  dcalc/DmpCeff.cc
  dcalc/DmpDelayCalc.cc
  dcalc/GraphDelayCalc.cc
  dcalc/GraphDelayCalc1.cc
  dcalc/LumpedCapDelayCalc.cc
  dcalc/NetCaps.cc
  dcalc/RCDelayCalc.cc
  dcalc/SimpleRCDelayCalc.cc
  dcalc/UnitDelayCalc.cc
  
  graph/DelayFloat.cc
  graph/DelayNormal1.cc
  graph/DelayNormal2.cc
  graph/Graph.cc
  graph/GraphCmp.cc
  
  liberty/EquivCells.cc
  liberty/FuncExpr.cc
  liberty/InternalPower.cc
  liberty/LeakagePower.cc
  liberty/Liberty.cc
  liberty/LibertyBuilder.cc
  liberty/LibertyExpr.cc
  liberty/LibertyExpr.hh
  liberty/LibertyExprLex.cc
  liberty/LibertyExprParse.cc
  liberty/LibertyExprPvt.hh
  liberty/LibertyLex.cc
  liberty/LibertyParse.cc
  liberty/LibertyParser.cc
  liberty/LibertyReader.cc
  liberty/LinearModel.cc
  liberty/Sequential.cc
  liberty/TableModel.cc
  liberty/TimingArc.cc
  liberty/TimingRole.cc
  liberty/Transition.cc
  liberty/Units.cc
  liberty/Wireload.cc
  
  network/ConcreteLibrary.cc
  network/ConcreteNetwork.cc
  network/HpinDrvrLoad.cc
  network/Network.cc
  network/NetworkCmp.cc
  network/ParseBus.cc
  network/PortDirection.cc
  network/SdcNetwork.cc
  network/VerilogNamespace.cc
  
  parasitics/ConcreteParasitics.cc
  parasitics/EstimateParasitics.cc
  parasitics/NullParasitics.cc
  parasitics/Parasitics.cc
  parasitics/ReduceParasitics.cc
  parasitics/SpefLex.cc
  parasitics/SpefNamespace.cc
  parasitics/SpefParse.cc
  parasitics/SpefReader.cc
  parasitics/SpefReaderPvt.hh
  
  sdc/Clock.cc
  sdc/ClockGatingCheck.cc
  sdc/ClockGroups.cc
  sdc/ClockInsertion.cc
  sdc/ClockLatency.cc
  sdc/CycleAccting.cc
  sdc/DataCheck.cc
  sdc/DeratingFactors.cc
  sdc/DisabledPorts.cc
  sdc/ExceptionPath.cc
  sdc/InputDrive.cc
  sdc/PinPair.cc
  sdc/PortDelay.cc
  sdc/PortExtCap.cc
  sdc/RiseFallMinMax.cc
  sdc/RiseFallValues.cc
  sdc/Sdc.cc
  sdc/SdcCmdComment.cc
  sdc/WriteSdc.cc
  
  sdf/ReportAnnotation.cc
  sdf/SdfReader.cc
  sdf/SdfParse.cc
  sdf/SdfLex.cc
  sdf/SdfWriter.cc
  
  search/Bfs.cc
  search/CheckMaxSkews.cc
  search/CheckMinPeriods.cc
  search/CheckMinPulseWidths.cc
  search/CheckSlewLimits.cc
  search/CheckTiming.cc
  search/ClkInfo.cc
  search/ClkSkew.cc
  search/Corner.cc
  search/Crpr.cc
  search/FindRegister.cc
  search/GatedClk.cc
  search/Genclks.cc
  search/Latches.cc
  search/Levelize.cc
  search/Path.cc
  search/PathAnalysisPt.cc
  search/PathEnd.cc
  search/PathEnum.cc
  search/PathEnumed.cc
  search/PathExpanded.cc
  search/PathGroup.cc
  search/PathRef.cc
  search/PathVertex.cc
  search/PathVertexRep.cc
  search/Power.cc
  search/Property.cc
  search/ReportPath.cc
  search/Search.cc
  search/SearchPred.cc
  search/Sim.cc
  search/Sta.cc
  search/StaState.cc
  search/Tag.cc
  search/TagGroup.cc
  search/VertexVisitor.cc
  search/VisitPathEnds.cc
  search/VisitPathGroupVertices.cc
  search/WorstSlack.cc
  search/WritePathSpice.cc
  
  util/Debug.cc
  util/Error.cc
  util/Fuzzy.cc
  util/Machine.cc
  util/MinMax.cc
  util/PatternMatch.cc
  util/Report.cc
  util/ReportStd.cc
  util/ReportTcl.cc
  util/Stats.cc
  util/StringSeq.cc
  util/StringSet.cc
  util/StringUtil.cc
  util/TokenParser.cc
  
  verilog/VerilogReader.cc
  verilog/VerilogLex.cc
  verilog/VerilogParse.cc
  )

set(STA_HEADERS
  app/StaMain.hh
  
  dcalc/ArcDelayCalc.hh
  dcalc/Arnoldi.hh
  dcalc/ArnoldiDelayCalc.hh
  dcalc/ArnoldiReduce.hh
  dcalc/DelayCalc.hh
  dcalc/DcalcAnalysisPt.hh
  dcalc/DmpCeff.hh
  dcalc/DmpDelayCalc.hh
  dcalc/GraphDelayCalc.hh
  dcalc/GraphDelayCalc1.hh
  dcalc/LumpedCapDelayCalc.hh
  dcalc/NetCaps.hh
  dcalc/RCDelayCalc.hh
  dcalc/SimpleRCDelayCalc.hh
  dcalc/UnitDelayCalc.hh
  
  graph/Delay.hh
  graph/DelayFloat.hh
  graph/DelayNormal1.hh
  graph/DelayNormal2.hh
  graph/Graph.hh
  graph/GraphClass.hh
  graph/GraphCmp.hh
  
  liberty/EquivCells.hh
  liberty/FuncExpr.hh
  liberty/InternalPower.hh
  liberty/LeakagePower.hh
  liberty/Liberty.hh
  liberty/LibertyBuilder.hh
  liberty/LibertyClass.hh
  liberty/LibertyParser.hh
  liberty/LibertyReader.hh
  liberty/LibertyReaderPvt.hh
  liberty/LinearModel.hh
  liberty/Sequential.hh
  liberty/TableModel.hh
  liberty/TimingArc.hh
  liberty/TimingModel.hh
  liberty/TimingRole.hh
  liberty/Transition.hh
  liberty/Units.hh
  liberty/Wireload.hh
  
  network/ConcreteLibrary.hh
  network/ConcreteNetwork.hh
  network/HpinDrvrLoad.hh
  network/MakeConcreteNetwork.hh
  network/Network.hh
  network/NetworkClass.hh
  network/NetworkCmp.hh
  network/ParseBus.hh
  network/PortDirection.hh
  network/SdcNetwork.hh
  network/VerilogNamespace.hh
  
  parasitics/ConcreteParasitics.hh
  parasitics/ConcreteParasiticsPvt.hh
  parasitics/EstimateParasitics.hh
  parasitics/MakeConcreteParasitics.hh
  parasitics/NullParasitics.hh
  parasitics/Parasitics.hh
  parasitics/ParasiticsClass.hh
  parasitics/ReduceParasitics.hh
  parasitics/SpefNamespace.hh
  parasitics/SpefReader.hh
  
  sdc/Clock.hh
  sdc/ClockGatingCheck.hh
  sdc/ClockGroups.hh
  sdc/ClockInsertion.hh
  sdc/ClockLatency.hh
  sdc/CycleAccting.hh
  sdc/DataCheck.hh
  sdc/DeratingFactors.hh
  sdc/DisabledPorts.hh
  sdc/ExceptionPath.hh
  sdc/InputDrive.hh
  sdc/MinMaxValues.hh
  sdc/PinPair.hh
  sdc/PortDelay.hh
  sdc/PortExtCap.hh
  sdc/RiseFallMinMax.hh
  sdc/RiseFallValues.hh
  sdc/Sdc.hh
  sdc/SdcClass.hh
  sdc/SdcCmdComment.hh
  sdc/WriteSdc.hh
  sdc/WriteSdcPvt.hh
  
  sdf/ReportAnnotation.hh
  sdf/Sdf.hh
  sdf/SdfReader.hh
  sdf/SdfWriter.hh
  
  search/Bfs.hh
  search/CheckMaxSkews.hh
  search/CheckMinPeriods.hh
  search/CheckMinPulseWidths.hh
  search/CheckSlewLimits.hh
  search/CheckTiming.hh
  search/ClkInfo.hh
  search/ClkSkew.hh
  search/Corner.hh
  search/Crpr.hh
  search/FindRegister.hh
  search/GatedClk.hh
  search/Genclks.hh
  search/Latches.hh
  search/Levelize.hh
  search/Path.hh
  search/PathAnalysisPt.hh
  search/PathEnd.hh
  search/PathEnum.hh
  search/PathEnumed.hh
  search/PathExpanded.hh
  search/PathRef.hh
  search/PathGroup.hh
  search/PathVertex.hh
  search/PathVertexRep.hh
  search/Power.hh
  search/Property.hh
  search/ReportPath.hh
  search/Search.hh
  search/SearchClass.hh
  search/SearchPred.hh
  search/Sim.hh
  search/Sta.hh
  search/StaState.hh
  search/Tag.hh
  search/TagGroup.hh
  search/VertexVisitor.hh
  search/VisitPathEnds.hh
  search/VisitPathGroupVertices.hh
  search/WorstSlack.hh
  search/WritePathSpice.hh
  
  util/Debug.hh
  util/DisallowCopyAssign.hh
  util/EnumNameMap.hh
  util/Error.hh
  util/Fuzzy.hh
  util/Hash.hh
  util/HashSet.hh
  util/Iterator.hh
  util/Machine.hh
  util/Map.hh
  util/MinMax.hh
  util/Mutex.hh
  util/ObjectIndex.hh
  util/PatternMatch.hh
  util/Pool.hh
  util/Report.hh
  util/ReportStd.hh
  util/ReportTcl.hh
  util/Set.hh
  util/StaConfig.hh
  util/Stats.hh
  util/StringSeq.hh
  util/StringSet.hh
  util/StringUtil.hh
  util/ThreadForEach.hh
  util/TokenParser.hh
  util/UnorderedMap.hh
  util/UnorderedSet.hh
  util/Vector.hh
  util/Zlib.hh
  
  verilog/Verilog.hh
  verilog/VerilogReader.hh
  )

# Source files.
set(STA_TCL_FILES
  tcl/Util.tcl
  tcl/Graph.tcl
  tcl/Liberty.tcl
  tcl/Link.tcl
  tcl/Network.tcl
  tcl/NetworkEdit.tcl
  tcl/Sdc.tcl
  tcl/Search.tcl
  tcl/Cmds.tcl
  tcl/Variables.tcl
  tcl/Sta.tcl
  tcl/Power.tcl
  tcl/Splash.tcl
  dcalc/DelayCalc.tcl
  parasitics/Parasitics.tcl
  sdf/Sdf.tcl
  verilog/Verilog.tcl
  )

set(STA_SWIG_FILES
  tcl/NetworkEdit.i
  tcl/StaException.i
  sdf/Sdf.i
  dcalc/DelayCalc.i
  parasitics/Parasitics.i
  verilog/Verilog.i
  tcl/StaTcl.i
  app/StaApp.i
  )

################################################################
#
# Library dependencies
#
################################################################

# Zlib
include(FindZLIB)
# translate cmake bool to ifdef bool
if (ZLIB_FOUND)
  set(ZLIB 1)
else()
  set(ZLIB 0)
endif()

################################################################
#
# Locate CUDD bdd packagte
# -DCUDD=0 to not use CUDD.
# Look for library in CUDD/lib,     CUDD/cudd/lib
# Look for header  in CUDD/include, CUDD/cudd/include
#
if("${CUDD}" STREQUAL "" OR "${CUDD}" STREQUAL "0")
  set(CUDD_INCLUDE "")
  set(CUDD_LIB "")
  set(CUDD_FOUND FALSE)
  set(CUDD 0)
  message(STATUS "CUDD library: not found")
else()
  find_library(CUDD_LIB NAMES cudd PATHS ${CUDD}/lib ${CUDD}/lib/cudd)
  if (CUDD_LIB)
    message(STATUS "CUDD library: ${CUDD_LIB}")
    get_filename_component(CUDD_LIB_DIR "${CUDD_LIB}" PATH)
    get_filename_component(CUDD_LIB_PARENT1 "${CUDD_LIB_DIR}" PATH)
    get_filename_component(CUDD_LIB_PARENT2 "${CUDD_LIB_PARENT1}" PATH)
    find_file(CUDD_HEADER cudd.h
      PATHS ${CUDD}/include ${CUDD_LIB_PARENT2}/include/cudd)
    if (CUDD_HEADER)
      get_filename_component(CUDD_INCLUDE "${CUDD_HEADER}" PATH)
      message(STATUS "CUDD header: ${CUDD_HEADER}")
      # CUDD referenced by StaConfig.hh.cmake
      set(CUDD 1)
    else()
      message(STATUS "CUDD header: not found")
    endif()
  else()
    set(CUDD_INCLUDE "")
    set(CUDD_LIB "")
    set(CUDD_FOUND FALSE)
    set(CUDD 0)
    message(STATUS "CUDD library: not found")
  endif()
endif()

if("${SSTA}" STREQUAL "")
  set(SSTA 0)
endif()
message(STATUS "SSTA: ${SSTA}")

# configure a header file to pass some of the CMake settins
configure_file(${STA_HOME}/util/StaConfig.hh.cmake
  ${STA_HOME}/util/StaConfig.hh
  )


################################################################
#
# Locate TCL library.
#
# Note that the cmake findTcl module is hopeless for OSX
# because there doesn't appear to be a way to override
# searching OSX system directories before unix directories.

set(TCL_POSSIBLE_NAMES tcl87 tcl8.7
  tcl86 tcl8.6
  tcl85 tcl8.5
  tcl84 tcl8.4
  tcl83 tcl8.3
  tcl82 tcl8.2
  )

# tcl lib path guesses.
if (NOT TCL_LIB_PATHS)
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TCL_LIB_PATHS /usr/local/lib)
    set(TCL_NO_DEFAULT_PATH TRUE)
  endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TCL_LIB_PATHS /usr/lib
    /usr/local/lib
    )
  set(TCL_NO_DEFAULT_PATH FALSE)
endif()

if (NOT TCL_LIB)
  # bagbiter cmake doesn't have a way to pass NO_DEFAULT_PATH as a parameter.
  if (TCL_NO_DEFAULT_PATH)
    find_library(TCL_LIB
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      NO_DEFAULT_PATH
      )
  else()
    find_library(TCL_LIB
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      )
  endif()
endif()
message(STATUS "TCL lib: ${TCL_LIB}")

get_filename_component(TCL_LIB_DIR "${TCL_LIB}" PATH)
get_filename_component(TCL_LIB_PARENT1 "${TCL_LIB_DIR}" PATH)
get_filename_component(TCL_LIB_PARENT2 "${TCL_LIB_PARENT1}" PATH)

# Locate tcl.h
if (NOT TCL_HEADER)
  find_file(TCL_HEADER tcl.h
    PATHS ${TCL_LIB_PARENT1} ${TCL_LIB_PARENT2}
    PATH_SUFFIXES include include/tcl
    NO_DEFAULT_PATH
    )
endif()
message(STATUS "TCL header: ${TCL_HEADER}")
get_filename_component(TCL_HEADER_DIR "${TCL_HEADER}" PATH)

################################################################
#
# Flex/bison scanner/parsers 
#
################################################################

find_package(FLEX)
find_package(BISON)
find_package(Readline REQUIRED)

# LibertyExpr scan/parse.
bison_target(LibertyExprParser liberty/LibertyExprParse.yy ${STA_HOME}/liberty/LibertyExprParse.cc
  DEFINES_FILE ${STA_HOME}/liberty/LibertyExprParse.hh
  COMPILE_FLAGS --name-prefix=LibertyExprParse_
  )

flex_target(LibertyExprLex liberty/LibertyExprLex.ll ${STA_HOME}/liberty/LibertyExprLex.cc
  DEFINES_FILE ${STA_HOME}/liberty/LibertyExprLex.hh
  COMPILE_FLAGS --prefix=LibertyExprLex_
  )

add_flex_bison_dependency(LibertyExprLex LibertyExprParser)

# Liberty scan/parse.
bison_target(LibertyParser liberty/LibertyParse.yy ${STA_HOME}/liberty/LibertyParse.cc
  DEFINES_FILE ${STA_HOME}/liberty/LibertyParse.hh
  COMPILE_FLAGS --name-prefix=LibertyParse_
  )

flex_target(LibertyLex liberty/LibertyLex.ll ${STA_HOME}/liberty/LibertyLex.cc
  DEFINES_FILE ${STA_HOME}/liberty/LibertyLex.hh
  COMPILE_FLAGS --prefix=LibertyLex_
  )

add_flex_bison_dependency(LibertyLex LibertyParser)

# Spef scan/parse.
bison_target(SpefParser parasitics/SpefParse.yy ${STA_HOME}/parasitics/SpefParse.cc
  DEFINES_FILE ${STA_HOME}/parasitics/SpefParse.hh
  COMPILE_FLAGS --name-prefix=SpefParse_
  )

flex_target(SpefLex parasitics/SpefLex.ll ${STA_HOME}/parasitics/SpefLex.cc
  DEFINES_FILE ${STA_HOME}/parasitics/SpefLex.hh
  COMPILE_FLAGS --prefix=SpefLex_
  )

add_flex_bison_dependency(SpefLex SpefParser)

# Verilog scan/parse.
bison_target(VerilogParser verilog/VerilogParse.yy ${STA_HOME}/verilog/VerilogParse.cc
  DEFINES_FILE ${STA_HOME}/verilog/VerilogParse.hh
  COMPILE_FLAGS --name-prefix=VerilogParse_
  )

flex_target(VerilogLex verilog/VerilogLex.ll ${STA_HOME}/verilog/VerilogLex.cc
  DEFINES_FILE ${STA_HOME}/verilog/VerilogLex.hh
  COMPILE_FLAGS --prefix=VerilogLex_
  )

add_flex_bison_dependency(VerilogLex VerilogParser)

# Sdf scan/parse.
bison_target(SdfParser sdf/SdfParse.yy ${STA_HOME}/sdf/SdfParse.cc
  DEFINES_FILE ${STA_HOME}/sdf/SdfParse.hh
  COMPILE_FLAGS --name-prefix=SdfParse_
  )

flex_target(SdfLex sdf/SdfLex.ll ${STA_HOME}/sdf/SdfLex.cc
  DEFINES_FILE ${STA_HOME}/sdf/SdfLex.hh
  COMPILE_FLAGS --prefix=SdfLex_
  )

add_flex_bison_dependency(SdfLex SdfParser)


################################################################

include(FindSWIG)

add_custom_command(OUTPUT ${STA_HOME}/app/StaApp_wrap.cc
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -o ${STA_HOME}/app/StaApp_wrap.cc ${STA_HOME}/app/StaApp.i
  COMMAND ${STA_HOME}/etc/SwigCleanup.tcl ${STA_HOME}/app/StaApp_wrap.cc
  WORKING_DIRECTORY ${STA_HOME}
  DEPENDS ${STA_SWIG_FILES}
  )

################################################################

# TCL files included as part of the executable are shoved into TclInitVar.cc.
# These files are encoded and shipped as part of the executable 
# so that they do not have to be installed on the client host.
add_custom_command(OUTPUT ${STA_HOME}/app/TclInitVar.cc
  COMMAND etc/TclEncode.tcl app/TclInitVar.cc tcl_inits  ${STA_TCL_FILES}
  WORKING_DIRECTORY ${STA_HOME}
  DEPENDS ${STA_TCL_FILES} etc/TclEncode.tcl
  )

################################################################

set(STA_INCLUDE_DIRS
  app
  dcalc
  dcalc/verilog
  graph
  liberty
  network
  parasitics
  sdc
  sdf
  search
  util
  verilog
  ${TCL_HEADER_DIR}
  ${CUDD_INCLUDE}
  )

###########################################################
# Library
###########################################################

# compatibility with configure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${STA_HOME}/app)

add_library(OpenSTA ${STA_SOURCE})

target_include_directories(OpenSTA PUBLIC ${STA_INCLUDE_DIRS})

target_compile_features(OpenSTA
  PUBLIC cxx_auto_type
  )

###########################################################
# Executable
###########################################################

# compatibility with configure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${STA_HOME}/app)

# Note executable and lib name cannot be the same because
# on osx something is case insensitive. Using STA for the
# lib name results in "No rule to make target ../depend.
add_executable(sta app/Main.cc)
target_link_libraries(sta
  OpenSTA
  ${TCL_LIB}
  ${CUDD_LIB}
  )
if (ZLIB_FOUND)
  target_link_libraries(sta ${ZLIB_LIBRARIES})
endif()

target_link_libraries(sta ${Readline_LIBRARY})

message(STATUS "STA executable: ${STA_HOME}/app/sta")

# g++ std::thread apparently still needs -pthreads.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(sta -pthread)
endif()

target_compile_options(OpenSTA PUBLIC ${STA_COMPILE_OPTIONS})
target_compile_options(sta PUBLIC ${STA_COMPILE_OPTIONS})
message(STATUS "Compiler options: ${STA_COMPILE_OPTIONS}")

################################################################
# Install
# cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>

# executable
install(TARGETS sta DESTINATION bin)

# library
install(TARGETS OpenSTA DESTINATION lib)

# include
install(FILES ${STA_HEADERS} DESTINATION include)

################################################################

add_custom_target(tags etags -o TAGS ${STA_SOURCE} ${STA_HEADERS} ${STA_TCL_FILES} ${SWIG_TCL_FILES}
  WORKING_DIRECTORY ${STA_HOME}
  DEPENDS ${STA_SOURCE} ${STA_HEADERS} ${STA_TCL_FILES} ${SWIG_TCL_FILES}
  )

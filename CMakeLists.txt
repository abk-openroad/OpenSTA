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

project(STA VERSION 2.0.17)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS ON)

set(STA_HOME ${PROJECT_SOURCE_DIR})
message(STATUS "STA version: ${PROJECT_VERSION}")

set(CMAKE_PREFIX_PATH $ENV{GALOIS_PREFIX_PATH}/lib/CMake/Galois ${CMAKE_PREFIX_PATH})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC STA_GIT_SHA1)
message(STATUS "STA git sha: ${STA_GIT_SHA1}")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

# Compiler specific options.
# Note -Wno-deprecated-register is to suppress bison errors.
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
find_package(Galois REQUIRED)
set(CMAKE_CXX_COMPILER ${Galois_CXX_COMPILER})
if (CMAKE_CXX_COMPILER_ID MATCHES AppleClang|Clang)
  set(STA_COMPILE_OPTIONS ${Galois_CXX_FLAGS} -Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security -Wno-deprecated-register)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(STA_COMPILE_OPTIONS ${Galois_CXX_FLAGS} -Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security)
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
  liberty/LibertyExprPvt.hh
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
  parasitics/SpefNamespace.cc
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
  util/DispatchQueue.cc
  util/Error.cc
  util/Fuzzy.cc
  util/Hash.cc
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
  verilog/VerilogWriter.cc
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
  liberty/LibertyExpr.hh
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
  util/DispatchQueue.hh
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
  util/ObjectId.hh
  util/ObjectTable.hh
  util/PatternMatch.hh
  util/Report.hh
  util/ReportStd.hh
  util/ReportTcl.hh
  util/Set.hh
  util/StaConfig.hh
  util/Stats.hh
  util/StringSeq.hh
  util/StringSet.hh
  util/StringUtil.hh
  util/TokenParser.hh
  util/UnorderedMap.hh
  util/UnorderedSet.hh
  util/Vector.hh
  util/Zlib.hh
  
  verilog/VerilogReaderPvt.hh
  verilog/VerilogReader.hh
  verilog/VerilogWriter.hh
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
# Flex/bison scanner/parsers
#
################################################################

find_package(FLEX)
find_package(BISON)

# LibertyExpr scan/parse.
flex_target(LibertyExprLex liberty/LibertyExprLex.ll ${CMAKE_CURRENT_BINARY_DIR}/LibertyExprLex.cc
  COMPILE_FLAGS --prefix=LibertyExprLex_
  )
bison_target(LibertyExprParser liberty/LibertyExprParse.yy ${CMAKE_CURRENT_BINARY_DIR}/LibertyExprParse.cc
  COMPILE_FLAGS --name-prefix=LibertyExprParse_
  )
add_flex_bison_dependency(LibertyExprLex LibertyExprParser)

# Liberty scan/parse.
flex_target(LibertyLex liberty/LibertyLex.ll ${CMAKE_CURRENT_BINARY_DIR}/LibertyLex.cc
  COMPILE_FLAGS --prefix=LibertyLex_
  )
bison_target(LibertyParser liberty/LibertyParse.yy ${CMAKE_CURRENT_BINARY_DIR}/LibertyParse.cc
  COMPILE_FLAGS --name-prefix=LibertyParse_
  )
add_flex_bison_dependency(LibertyLex LibertyParser)

# Spef scan/parse.
flex_target(SpefLex parasitics/SpefLex.ll ${CMAKE_CURRENT_BINARY_DIR}/SpefLex.cc
  COMPILE_FLAGS --prefix=SpefLex_
  )
bison_target(SpefParser parasitics/SpefParse.yy ${CMAKE_CURRENT_BINARY_DIR}/SpefParse.cc
  COMPILE_FLAGS --name-prefix=SpefParse_
  )
add_flex_bison_dependency(SpefLex SpefParser)

# Verilog scan/parse.
flex_target(VerilogLex verilog/VerilogLex.ll ${CMAKE_CURRENT_BINARY_DIR}/VerilogLex.cc
  COMPILE_FLAGS --prefix=VerilogLex_
  )
bison_target(VerilogParser verilog/VerilogParse.yy ${CMAKE_CURRENT_BINARY_DIR}/VerilogParse.cc
  COMPILE_FLAGS --name-prefix=VerilogParse_
  )
add_flex_bison_dependency(VerilogLex VerilogParser)

# Sdf scan/parse.
flex_target(SdfLex sdf/SdfLex.ll ${CMAKE_CURRENT_BINARY_DIR}/SdfLex.cc
  COMPILE_FLAGS --prefix=SdfLex_
  )
bison_target(SdfParser sdf/SdfParse.yy ${CMAKE_CURRENT_BINARY_DIR}/SdfParse.cc
  COMPILE_FLAGS --name-prefix=SdfParse_
  )
add_flex_bison_dependency(SdfLex SdfParser)


################################################################

set(SWIG_REQUIRED_VERSION "2.0")
find_package(SWIG REQUIRED)
if(${SWIG_VERSION} VERSION_LESS ${SWIG_REQUIRED_VERSION})
  message(FATAL_ERROR "SWIG version ${SWIG_VERSION} must be at least ${SWIG_REQUIRED_VERSION}")
endif()

set(STA_WRAP ${CMAKE_CURRENT_BINARY_DIR}/StaApp_wrap.cc)

add_custom_command(OUTPUT ${STA_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${STA_HOME}/tcl -I${STA_HOME}/sdf -I${STA_HOME}/dcalc -I${STA_HOME}/parasitics -I${STA_HOME}/verilog -o ${STA_WRAP} ${STA_HOME}/app/StaApp.i
  COMMAND ${STA_HOME}/etc/SwigCleanup.tcl ${STA_WRAP}
  WORKING_DIRECTORY ${STA_HOME}
  DEPENDS ${STA_SWIG_FILES}
  )

################################################################

set(STA_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/StaTclInitVar.cc)

# TCL files included as part of the executable are shoved into StaTclInitVar.cc.
# These files are encoded and shipped as part of the executable 
# so that they do not have to be installed on the client host.
add_custom_command(OUTPUT ${STA_TCL_INIT}
  COMMAND etc/TclEncode.tcl ${STA_TCL_INIT} tcl_inits ${STA_TCL_FILES}
  WORKING_DIRECTORY ${STA_HOME}
  DEPENDS ${STA_TCL_FILES} etc/TclEncode.tcl
  )

################################################################
#
# Library dependencies
#
################################################################

# OSX tcl is deprecated and prints a warning, so look for a user installed
# version before using the system version.
# I tried to override the library search order instead but failed.
# CMAKE_FIND_FRAMEWORK LAST bypasses the version in the framework directory
# but not the one in /usr/lib.
# This calls cmake/FindTCL.cmake
# Do not use REQUIRED because it also requires TK.
find_package(TCL)

# Zlib
include(FindZLIB)
# Translate cmake bool to StaConfig.h ifdef bool
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

set(STA_INCLUDE_DIRS
  app
  dcalc
  graph
  liberty
  network
  parasitics
  sdc
  sdf
  search
  util
  verilog
  ${CUDD_INCLUDE}
  ${Galois_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  )

###########################################################
# Library
###########################################################

# compatibility with configure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${STA_HOME}/app)

add_library(OpenSTA
  ${STA_SOURCE}

  ${STA_WRAP}
  ${STA_TCL_INIT}

  ${FLEX_LibertyExprLex_OUTPUTS}
  ${BISON_LibertyExprParser_OUTPUTS}
  ${FLEX_LibertyLex_OUTPUTS}
  ${BISON_LibertyParser_OUTPUTS}

  ${FLEX_SpefLex_OUTPUTS}
  ${BISON_SpefParser_OUTPUTS}

  ${FLEX_SdfLex_OUTPUTS}
  ${BISON_SdfParser_OUTPUTS}

  ${FLEX_VerilogLex_OUTPUTS}
  ${BISON_VerilogParser_OUTPUTS}
)

target_include_directories(OpenSTA
  PUBLIC ${STA_INCLUDE_DIRS}
  SYSTEM ${TCL_INCLUDE_PATH}
  )

target_compile_features(OpenSTA
  PUBLIC cxx_auto_type
  )

target_compile_options(OpenSTA PUBLIC ${STA_COMPILE_OPTIONS})
target_link_libraries(OpenSTA PUBLIC ${Galois_LIBRARIES} ${Boost_LIBRARIES} Threads::Threads)

###########################################################
# Executable
###########################################################

# compatibility with configure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${STA_HOME}/app)

# Note executable and lib name cannot be the same because
# on osx something is case insensitive. Using STA for the
# lib name results in "No rule to make target ../depend.
add_executable(sta app/Main.cc ${STA_WRAP})

target_link_libraries(sta
  OpenSTA
  ${TCL_LIBRARY}
  ${CUDD_LIB}
  )

if (ZLIB_FOUND)
  target_link_libraries(sta ${ZLIB_LIBRARIES})
endif()

message(STATUS "STA executable: ${STA_HOME}/app/sta")

# g++ std::thread apparently still needs -pthreads.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(sta -pthread)
endif()

target_compile_options(sta PUBLIC ${STA_COMPILE_OPTIONS})

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

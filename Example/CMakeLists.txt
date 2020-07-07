cmake_minimum_required(VERSION 3.6)

#only thing we do here is set the required path(s) and then include the TelemetryCore.cmake file
#which will build all native libraries

set(V_CORE_DIR ${CMAKE_SOURCE_DIR}../../../LiveVideo10ms/VideoCore)

include(${CMAKE_SOURCE_DIR}/../TelemetryCore/TelemetryCore.cmake)

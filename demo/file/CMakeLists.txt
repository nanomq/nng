#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required (VERSION 2.8.7)

project(mqtt_nftp)

find_package(nng CONFIG REQUIRED)

find_package(Threads)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mqtt_nftp_recver mqtt_nftp_recver.c)
add_executable(mqtt_nftp_sender mqtt_nftp_sender.c)
target_link_libraries(mqtt_nftp_recver nng nftp-codec hashtable)
target_link_libraries(mqtt_nftp_sender nng nftp-codec hashtable)
target_link_libraries(mqtt_nftp_recver ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(mqtt_nftp_sender ${CMAKE_THREAD_LIBS_INIT})

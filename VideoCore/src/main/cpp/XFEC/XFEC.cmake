# Add the XFEC sources and compile library

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(DIR_XFEC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src)

add_library(XFEC_lib
        SHARED
        #src/raw_socket.cc
        ${DIR_XFEC_SOURCES}/fec.c
        ${DIR_XFEC_SOURCES}/fec.cc
        ${DIR_XFEC_SOURCES}/test_fec.cc
        #src/radiotap/radiotap.c
        #src/transfer_stats.cc
        )

# Logging and Android
target_link_libraries(XFEC_lib
        ${log-lib}
        log
        android
        )
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(DIR_XFEC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src)

add_library(XFEC_lib
        SHARED
        #src/raw_socket.cc
        ${DIR_XFEC_SOURCES}/fec.c
        #src/fec.cc
        #src/radiotap/radiotap.c
        #src/transfer_stats.cc
        )
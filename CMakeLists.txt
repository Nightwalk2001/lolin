cmake_minimum_required(VERSION 3.13)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_VERSION 23)

set(PIO_PACKAGES .pio/libdeps/nodemcuv2)

project("lolin" C CXX)

include(CMakeListsPrivate.txt)

include_directories(${PIO_PACKAGES}/PubSubClient/src)
include_directories(".pio/libdeps/nodemcuv2/Cheap Stepper/src")
include_directories(${PIO_PACKAGES}/Time)
include_directories(${PIO_PACKAGES}/ArduinoJson/src)
include_directories(${PIO_PACKAGES}/TaskScheduler/src)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeListsUser.txt)
    include(CMakeListsUser.txt)
endif ()

add_custom_target(
        Production ALL
        COMMAND platformio -c clion run "$<$<NOT:$<CONFIG:All>>:-e${CMAKE_BUILD_TYPE}>"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
        Debug ALL
        COMMAND platformio -c clion debug "$<$<NOT:$<CONFIG:All>>:-e${CMAKE_BUILD_TYPE}>"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(Z_DUMMY_TARGET ${SRC_LIST} src/config.hpp)

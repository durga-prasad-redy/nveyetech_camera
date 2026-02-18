cmake_minimum_required(VERSION 3.12)
project(M5S_HTTP_SERVER)

# DEBUG, RELEASE, MINSIZEREL, RELWITHDEBINFO
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("====== Debug mode ======")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -g -fasynchronous-unwind-tables -DEPOLL -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -g -fasynchronous-unwind-tables -DEPOLL")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
    add_definitions("-DDEBUG")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("====== RelWithDebInfo mode ======")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3 -Wall -Wextra -DEPOLL -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wextra -DEPOLL")
    add_definitions("-DDEBUG")
else ()
    message("====== Release mode ======")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O3 -Wall -Wextra -DEPOLL -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Wextra -DEPOLL")
    add_definitions("-DNDEBUG")
endif ()

# Platform configuration
# set(VIENNA_PLATFORM true CACHE BOOL "target to Vienna platform")
# set(PLATFORM_DIR vienna)

# Set paths
set(CMAKE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
# set(MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../modules")
# set(MODULE_LIB "${MODULE_PATH}/libs/${PLATFORM_DIR}")
# set(VTCS_SW_DEV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vtcs_root_${PLATFORM_DIR}/")
# set(VTCS_TOOLKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../toolkit")

IF(CMAKE_CROSSCOMPILING)
	SET(HOST_CONFIG --host=arm-linux)	
	if(${CMAKE_VERSION} VERSION_GREATER 2.8.11)
		GET_FILENAME_COMPONENT(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} DIRECTORY) # This is for CMake > 2.8.11
	ELSE()
		GET_FILENAME_COMPONENT(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} PATH) # This is for CMake <= 2.8.11
	ENDIF()	
	SET(ADDITIONAL_ENV_CONFIG CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} LD=${CMAKE_LINKER} NM=${CMAKE_NM} AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} STRIP=${CMAKE_STRIP} OBJDUMP=${CMAKE_OBJDUMP} PKG_CONFIG=${MY_TOOLCHAIN_DIR}/pkg-config)
ENDIF(CMAKE_CROSSCOMPILING)


include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
        ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../motocam_api_libs
        ${CMAKE_CURRENT_SOURCE_DIR}/../motocam_api_libs/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../motocam_api_libs/include/l1
        ${CMAKE_CURRENT_SOURCE_DIR}/../motocam_api_libs/include/l2
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../cryptographic/install_temp_open_src_3rd/include

)

# Set output directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM_DIR})

# Source files
set(SRC_LIST
        main.cpp
        src/main/net.cpp
        src/server/web_server.cpp
        src/session/session_manager.cpp
        src/session/generic_lru.cpp
        src/session/fifo_queue.cpp
        src/handlers/auth_handler.cpp
        src/handlers/device_handler.cpp
        src/handlers/provision_handler.cpp
        src/handlers/proxy_handler.cpp
        src/handlers/upload_handler.cpp
        src/handlers/metrics_handler.cpp
        src/utils/http_utils.cpp
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cryptographic/install_temp_open_src_3rd/lib)
set(MOTOCAM_APIS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../motocam_api_libs/build_vienna/libmotocam_apis.a")
set(CIVETWEB_LIB     "${CMAKE_CURRENT_SOURCE_DIR}/lib/libcivetweb.a")


if (NOT MOTOCAM_APIS_LIB)
    message(FATAL_ERROR "Could not find MOTOCAM_APIS_LIB shared library")
else ()
    message(STATUS "Found MOTOCAM_APIS_LIB at: ${MOTOCAM_APIS_LIB}")
endif ()
if (NOT CIVETWEB_LIB)
    message(FATAL_ERROR "Could not find CIVETWEB_LIB shared library")
else ()
    message(STATUS "Found CIVETWEB_LIB at: ${CIVETWEB_LIB}")
endif ()
# Create executable
add_executable(m5s_http_server ${SRC_LIST})
target_link_libraries(m5s_http_server ${MOTOCAM_APIS_LIB} ${CIVETWEB_LIB} pthread ssl crypto)

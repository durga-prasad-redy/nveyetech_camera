cmake_minimum_required(VERSION 3.12)
project(VTCS_LIB_APP)

# --------------------------------------------------
# Language Standards (IMPORTANT for SonarQube)
# --------------------------------------------------

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Build Type Configuration
# --------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("====== Debug mode ======")
    add_compile_options(-Wall -Wextra -pthread -g -fasynchronous-unwind-tables)
    add_link_options(-rdynamic)
    add_definitions(-DDEBUG)
    set(DEBUG_ENABLE true)

elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("====== RelWithDebInfo mode ======")
    add_compile_options(-O3 -Wall -Wextra -pthread)
    add_definitions(-DDEBUG)
    set(DEBUG_ENABLE true)

else()
    message("====== Release mode ======")
    add_compile_options(-O3 -Wall -Wextra -pthread)
    add_definitions(-DNDEBUG)
    set(DEBUG_ENABLE false)
endif()

# --------------------------------------------------
# Platform Configuration
# --------------------------------------------------

set(VIENNA_PLATFORM true CACHE BOOL "target to Vienna platform")
set(PLATFORM_DIR vienna)

set(CMAKE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

set(MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../modules")
set(MODULE_LIB "${MODULE_PATH}/libs/${PLATFORM_DIR}")
set(MODULE_OPEN_SRC_3RD_INSTALL_DIR "${MODULE_PATH}/install_temp_open_src_3rd")

set(VTCS_SW_DEV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../vtcs_root_${PLATFORM_DIR}/")
set(VTCS_TOOLKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../toolkit")
set(VTCS_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# --------------------------------------------------
# Cross Compilation Support
# --------------------------------------------------

if(CMAKE_CROSSCOMPILING)

    set(HOST_CONFIG --host=arm-linux)

    if(${CMAKE_VERSION} VERSION_GREATER 2.8.11)
        get_filename_component(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} DIRECTORY)
    else()
        get_filename_component(MY_TOOLCHAIN_DIR ${CMAKE_C_COMPILER} PATH)
    endif()

    set(ADDITIONAL_ENV_CONFIG
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        LD=${CMAKE_LINKER}
        NM=${CMAKE_NM}
        AR=${CMAKE_AR}
        RANLIB=${CMAKE_RANLIB}
        STRIP=${CMAKE_STRIP}
        OBJDUMP=${CMAKE_OBJDUMP}
        PKG_CONFIG=${MY_TOOLCHAIN_DIR}/pkg-config
    )

endif()

# --------------------------------------------------
# Include & Link Directories
# --------------------------------------------------

include_directories(
    ${MODULE_PATH}
    ${MODULE_OPEN_SRC_3RD_INSTALL_DIR}/include
    ${VTCS_SW_DEV_ROOT}/include
    ${VTCS_SW_DEV_ROOT}/include/vmf
    ${VTCS_TOOLKIT_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/l1
    ${CMAKE_CURRENT_SOURCE_DIR}/include/l2
)

link_directories(
    ${MODULE_LIB}
    ${MODULE_OPEN_SRC_3RD_INSTALL_DIR}/lib
    ${VTCS_SW_DEV_ROOT}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --------------------------------------------------
# Output Directory
# --------------------------------------------------

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM_DIR})

include(${CMAKE_MODULE_DIR}/ConditionalAddSubDir.cmake)

# --------------------------------------------------
# Source Files
# --------------------------------------------------

file(GLOB SRC_L2
    "${CMAKE_CURRENT_SOURCE_DIR}/src/l2/motocam_*.cpp"
)

file(GLOB SRC_L1
    "${CMAKE_CURRENT_SOURCE_DIR}/src/l1/motocam_*.cpp"
)

set(SRC_LIST
    src/motocam_api_l2.cpp
    src/motocam_api_l1.cpp
    ${SRC_L2}
    ${SRC_L1}
)

# --------------------------------------------------
# Prebuilt Firmware Library
# --------------------------------------------------

set(MOTOCAM_FW_LIB
    "${CMAKE_CURRENT_SOURCE_DIR}/../motocam_fw_libs/build_vienna/libmotocam_fw.a"
)

add_library(motocam_fw STATIC IMPORTED GLOBAL)
set_target_properties(motocam_fw PROPERTIES
    IMPORTED_LOCATION "${MOTOCAM_FW_LIB}"
)

# --------------------------------------------------
# Main Static Library
# --------------------------------------------------

set(LIBRARY_NAME motocam_apis)

add_library(${LIBRARY_NAME} STATIC ${SRC_LIST})

target_link_libraries(${LIBRARY_NAME}
    PUBLIC motocam_fw
)

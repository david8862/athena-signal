# project
cmake_minimum_required(VERSION 3.5)
project(AthenaSignal LANGUAGES C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_BUILD_TYPE "Debug")
#SET(CMAKE_BUILD_TYPE "Release")

# ---[ AthenaSignal version
set(ATHENASIGNAL_TARGET_VERSION   "1.0.0"  CACHE STRING "AthenaSignal logical version")
set(ATHENASIGNAL_TARGET_SOVERSION "1"      CACHE STRING "AthenaSignal soname version")
add_definitions(-DATHENASIGNAL_VERSION=${ATHENASIGNAL_TARGET_VERSION})

#set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# https://www.zhihu.com/question/27090458
# https://blog.csdn.net/m0_37870649/article/details/109139204
option(ATHENASIGNAL_BUILD_FAST "build lib & binary with gcc -Ofast optimization" OFF)
option(ATHENASIGNAL_BUILD_TEST_COVERAGE "build lib & binary for test coverage" OFF)
option(ATHENASIGNAL_BUILD_ASAN "build lib & binary for asan analysis" OFF)
option(ATHENASIGNAL_BUILD_FOR_GPROF "build lib & binary for gprof analysis" OFF)

# --- [ Add some build optimization
if (ATHENASIGNAL_BUILD_FAST)
    message(STATUS "Build with gcc -Ofast optimization")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -Wall")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -g")
endif()

if (ATHENASIGNAL_BUILD_TEST_COVERAGE)
    message(STATUS "Add flags in build for test coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -fPIC")
endif()

if (ATHENASIGNAL_BUILD_ASAN)
    message(STATUS "Add flags in build for asan analysis")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-recover=address -fno-var-tracking -fno-omit-frame-pointer -fno-stack-protector")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize-recover=address -fno-var-tracking -fno-omit-frame-pointer -fno-stack-protector")
endif()

if(ATHENASIGNAL_BUILD_FOR_GPROF)
    message(STATUS "Add '-pg' in build for gprof analysis")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

set(LIBATHENASIGNAL athenasignal)

file(GLOB_RECURSE LIBATHENASIGNAL_SRCS    ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/*.c)

add_library(${LIBATHENASIGNAL} SHARED ${LIBATHENASIGNAL_SRCS})
target_include_directories(${LIBATHENASIGNAL} PUBLIC ${CMAKE_HOME_DIRECTORY}/athena_signal/)


# --- [ Build samples
file (GLOB SAMPLE_SRCS   ${CMAKE_HOME_DIRECTORY}/samples/*.c)
foreach (source ${SAMPLE_SRCS})
  get_filename_component(name ${source} NAME_WE)
  add_executable(${name} ${source})
  target_include_directories(${name} PRIVATE ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels)
  target_link_libraries(${name} PUBLIC ${LIBATHENASIGNAL} -lm)
endforeach ()


# --- [ Install

# ---[ Install options
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# 获取所有.h文件，包括子目录
file(GLOB LIBATHENASIGNAL_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/*.h)
file(GLOB LIBATHENASIGNAL_SHARE_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_share/*.h)
file(GLOB LIBATHENASIGNAL_AEC_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_aec/*.h)
file(GLOB LIBATHENASIGNAL_AEC_TDE_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_aec/dios_ssp_aec_tde/*.h)
file(GLOB LIBATHENASIGNAL_AGC_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_agc/*.h)
file(GLOB LIBATHENASIGNAL_DOA_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_doa/*.h)
file(GLOB LIBATHENASIGNAL_GSC_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_gsc/*.h)
file(GLOB LIBATHENASIGNAL_HPF_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_hpf/*.h)
file(GLOB LIBATHENASIGNAL_MVDR_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_mvdr/*.h)
file(GLOB LIBATHENASIGNAL_NS_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_ns/*.h)
file(GLOB LIBATHENASIGNAL_VAD_HDRS   ${CMAKE_HOME_DIRECTORY}/athena_signal/kernels/dios_ssp_vad/*.h)

set_target_properties(${LIBATHENASIGNAL} PROPERTIES
        VERSION   ${ATHENASIGNAL_TARGET_VERSION}
        SOVERSION ${ATHENASIGNAL_TARGET_SOVERSION})
install(TARGETS ${LIBATHENASIGNAL} EXPORT AthenaSignalTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${LIBATHENASIGNAL_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${LIBATHENASIGNAL_SHARE_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_share)
install(FILES ${LIBATHENASIGNAL_AEC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_aec)
install(FILES ${LIBATHENASIGNAL_AEC_TDE_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_aec/dios_ssp_aec_tde)
install(FILES ${LIBATHENASIGNAL_AGC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_agc)
install(FILES ${LIBATHENASIGNAL_DOA_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_doa)
install(FILES ${LIBATHENASIGNAL_GSC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_gsc)
install(FILES ${LIBATHENASIGNAL_HPF_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_hpf)
install(FILES ${LIBATHENASIGNAL_MVDR_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_mvdr)
install(FILES ${LIBATHENASIGNAL_NS_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_ns)
install(FILES ${LIBATHENASIGNAL_VAD_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dios_ssp_vad)

set(ATHENASIGNAL_SAMPLES athena_signal_bf_sample athena_signal_doa_sample athena_signal_aec_sample athena_signal_agc_sample athena_signal_hpf_sample athena_signal_aec_bf_agc_sample)
install(TARGETS ${ATHENASIGNAL_SAMPLES} DESTINATION ${CMAKE_INSTALL_BINDIR}/samples)

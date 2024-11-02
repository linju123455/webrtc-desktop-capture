cmake_minimum_required(VERSION 3.5)
project(wasm-display-test C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -DWEBRTC_LINUX=1 -DWEBRTC_POSIX=1 -g -rdynamic -w -pthread -lrt --std=c++17")

include_directories(${CMAKE_BINARY_DIR})
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/linux-include)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/boringssl/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/linux-lib)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/out/new_Debug64/obj)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/out/Debug64_no_ssl/obj)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/out/Debug64/obj/third_party/boringssl)


find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    message(${OPENSSL_INCLUDE_DIR})
    link_libraries(${OPENSSL_LIBRARIES})
endif()

find_package(PkgConfig)
include_directories(${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

# 添加源文件
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 添加可执行文件
add_executable(wasm-display-test ${SOURCE_FILES})

target_link_libraries(wasm-display-test webrtc ssl crypto boost_system  ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES} dl)
cmake_minimum_required(VERSION 3.10)
project(Luxtep_V02)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformIO.cmake)

# 헤더 파일이 있는 include 디렉토리 추가
include_directories(
    include
    src
    components/driver_24x35
    components/protocol_uart
)

file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "components/driver_24x35/*.cpp"
    "components/protocol_uart/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 필요한 경우 라이브러리 링크 (PlatformIO를 통해 관리되는 라이브러리는 필요 없음)
# target_link_libraries(${PROJECT_NAME} ...)
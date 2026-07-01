cmake_minimum_required(VERSION 3.24)
project(HaloServerBrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ServerDetailsDialog.cpp
)

set(HEADERS
    src/MainWindow.h
    src/ServerDetailsDialog.h
    src/ServerItem.h
)

add_executable(HaloServerBrowser WIN32 ${SOURCES} ${HEADERS})

target_include_directories(HaloServerBrowser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(HaloServerBrowser PRIVATE Qt6::Core Qt6::Widgets)

if (MSVC)
    target_compile_options(HaloServerBrowser PRIVATE /W4 /permissive-)
else()
    target_compile_options(HaloServerBrowser PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS HaloServerBrowser RUNTIME DESTINATION .)

set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "HaloServerBrowser")
set(CPACK_PACKAGE_VENDOR "Chalwk")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_NSIS_DISPLAY_NAME "HaloServerBrowser")
set(CPACK_NSIS_PACKAGE_NAME "HaloServerBrowser")
include(CPack)
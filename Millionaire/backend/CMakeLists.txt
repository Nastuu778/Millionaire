cmake_minimum_required(VERSION 3.12)
project(MillionaireBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

# Основная библиотека
add_library(core STATIC
    question.cpp
    question_manager.cpp
)

target_include_directories(core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core PRIVATE SQLite::SQLite3)

# Исполняемый файл
add_executable(MillionaireBackend main.cpp)
target_link_libraries(MillionaireBackend PRIVATE core)
cmake_minimum_required(VERSION 3.10)
project(MillionaireGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
set(SOURCES
    game_session.cpp
    question.cpp
    question_manager.cpp
    main.cpp
)

# Исполняемый файл
add_executable(millionaire ${SOURCES})

# Подключение SQLite (если используется)
find_package(SQLite3 REQUIRED)
target_link_libraries(millionaire PRIVATE SQLite::SQLite3)

# Настройки для сборки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(millionaire PRIVATE -Wall -Wextra -g)
else()
    target_compile_options(millionaire PRIVATE -O2)
endif()
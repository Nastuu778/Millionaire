cmake_minimum_required(VERSION 3.10)
project(quze_core)

# Включение подробного логирования
set(CMAKE_VERBOSE_MAKEFILE ON)

# Определение режима сборки
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "===============================================")
message(STATUS "Starting configuration for ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
message(STATUS "===============================================")

# Использование переменных от cmake-js
if(DEFINED CMAKE_JS_INC)
    set(NODE_INCLUDE_DIR "${CMAKE_JS_INC}")
    message(STATUS "Using CMAKE_JS_INC for Node headers: ${NODE_INCLUDE_DIR}")
else()
    # Fallback для локальной разработки
    set(NODE_ROOT_DIR "$ENV{HOME}/.cmake-js/node-x64/v${NODE_VERSION}")
    set(NODE_INCLUDE_DIR "${NODE_ROOT_DIR}/include/node")
    message(WARNING "CMAKE_JS_INC not defined! Using fallback: ${NODE_INCLUDE_DIR}")
endif()

# Путь к node-addon-api
set(NAPI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")
message(STATUS "NAPI include path: ${NAPI_INCLUDE_DIR}")

# Добавляем путь к заголовочным файлам проекта
set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src")
message(STATUS "Project include path: ${PROJECT_INCLUDE_DIR}")

# Обновите проверку заголовочного файла:
set(REQUIRED_HEADERS
    "${NODE_INCLUDE_DIR}/node.h"
    "${NODE_INCLUDE_DIR}/node_api.h"
    "${NAPI_INCLUDE_DIR}/napi.h"
    "${PROJECT_INCLUDE_DIR}/quze_core.h"  # Теперь ищем в src
)

foreach(HEADER ${REQUIRED_HEADERS})
    if(NOT EXISTS ${HEADER})
        message(FATAL_ERROR "Required header not found: ${HEADER}")
    else()
        message(STATUS "Header found: ${HEADER}")
    endif()
endforeach()

# Включаем найденные пути
include_directories(
    ${NODE_INCLUDE_DIR}
    ${NAPI_INCLUDE_DIR}
    ${PROJECT_INCLUDE_DIR}  # Добавляем путь к заголовкам проекта
)

# Настройки компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(NAPI_VERSION=6)
add_compile_definitions(BUILDING_NODE_EXTENSION)

# Параметры компилятора для отладки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    message(STATUS "Debug build configuration")
else()
    add_compile_options(-O3)
    message(STATUS "Release build configuration")
endif()

# Поиск и линковка SQLite3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

message(STATUS "SQLite3 version: ${SQLITE3_VERSION}")
message(STATUS "SQLite3 include dirs: ${SQLITE3_INCLUDE_DIRS}")
message(STATUS "SQLite3 libraries: ${SQLITE3_LIBRARIES}")

# Сборка основной библиотеки
add_library(${PROJECT_NAME} SHARED 
    src/quze_core.cpp
    src/quze_core.h  # Указываем правильный путь к заголовочному файлу
)

# Линковка с SQLite3
target_link_libraries(${PROJECT_NAME} PRIVATE ${SQLITE3_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${SQLITE3_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}  # Добавляем путь к заголовкам для цели
)

# Настройки выходного файла
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    OUTPUT_NAME "${PROJECT_NAME}"
)

# Настройка выходной директории
if(CMAKE_JS_LIB AND NOT "${CMAKE_JS_LIB}" STREQUAL "")
    set(LIBRARY_OUTPUT_PATH "${CMAKE_JS_LIB}")
    message(STATUS "Using CMAKE_JS_LIB output path: ${CMAKE_JS_LIB}")
else()
    set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
    message(STATUS "Using default output path: ${LIBRARY_OUTPUT_PATH}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_OUTPUT_PATH}"
)

# Добавляем флаги специфичные для Node.js
target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -fPIC
)

# Логирование финальных настроек
message(STATUS "===============================================")
message(STATUS "Build configuration summary:")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Node.js Headers: ${NODE_INCLUDE_DIR}")
message(STATUS "NAPI Headers: ${NAPI_INCLUDE_DIR}")
message(STATUS "Project Headers: ${PROJECT_INCLUDE_DIR}")
message(STATUS "SQLite3 Include: ${SQLITE3_INCLUDE_DIRS}")
message(STATUS "SQLite3 Libraries: ${SQLITE3_LIBRARIES}")
message(STATUS "Output Directory: ${LIBRARY_OUTPUT_PATH}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Source Files: src/quze_core.cpp")
message(STATUS "Header Files: include/quze_core.h")
message(STATUS "===============================================")
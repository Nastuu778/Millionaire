cmake_minimum_required(VERSION 3.16)
project(Millionaire LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql Quick Widgets)

qt_add_executable(app
    src/main.cpp
    src/core/question.cpp
    src/core/question_manager.cpp
    src/core/game_session.cpp
)

target_link_libraries(app PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Quick
    Qt6::Widgets
)

qt_add_resources(app "resources"
    PREFIX "/"
    FILES
        qml/main.qml
        images/logo.png
        sounds/correct.wav
)
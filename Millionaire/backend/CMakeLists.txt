cmake_minimum_required(VERSION 3.10)
project(MillionaireQuiz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-D_GLIBCXX_USE_CXX11_ABI=1)

add_executable(main
    main.cpp
    question.cpp
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
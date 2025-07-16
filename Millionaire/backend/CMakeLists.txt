cmake_minimum_required(VERSION 3.12)

# Принудительно устанавливаем Release как конфигурацию по умолчанию
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

project(Millionaire)

#Явно указываем путь к pybind11
set(pybind11_DIR "C:/Dev/Python/Lib/site-packages/pybind11/share/cmake/pybind11")

#Ищем Python с обязательными компонентами
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

#Проверяем найденные компоненты
message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python includes: ${Python_INCLUDE_DIRS}")
message(STATUS "Python libraries: ${Python_LIBRARIES}")

# Создаем модуль
pybind11_add_module(quiz_core
    python_binding.cpp
    question.cpp
    question_manager.cpp
    game_session.cpp
)

#Подключаем 
target_link_libraries(quiz_core PRIVATE
    ${Python_LIBRARIES}
    sqlite3
)

#Для Windows используем .pyd
set_target_properties(quiz_core PROPERTIES SUFFIX ".pyd")
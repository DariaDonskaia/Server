# CMakeList.txt: проект CMake для Server; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)

# Добавьте источник в исполняемый файл этого проекта.
add_executable (Server "Server.cpp" "Server.h")

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
find_package(Threads REQUIRED)

target_link_libraries(Server ${CMAKE_THREAD_LIBS_INIT})
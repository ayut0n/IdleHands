cmake_minimum_required(VERSION 3.10)
project(IdleHands)

set(CMAKE_CXX_STANDARD 17)

# Заставляем MinGW вшить все свои библиотеки (libstdc++, libgcc и т.д.) внутрь EXE
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Подключаем файл с иконкой
set(RESOURCE_FILE ${CMAKE_SOURCE_DIR}/resource.rc)

# Флаг WIN32 гарантирует запуск без черного окна консоли
add_executable(IdleHands WIN32 main.cpp ${IMGUI_SOURCES} ${RESOURCE_FILE})

target_include_directories(IdleHands PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${CMAKE_SOURCE_DIR})

# ИСПРАВЛЕНИЕ ЗДЕСЬ:
# Меняем 'glfw' на 'glfw3' (это заставляет MinGW взять статический libglfw3.a).
# Также добавляем 'gdi32', так как статическому GLFW требуются базовые окна Windows.
target_link_libraries(IdleHands PRIVATE glfw3 opengl32 gdi32 ${CMAKE_SOURCE_DIR}/discord-rpc.lib)
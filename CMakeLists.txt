# Сборка через CMake — в дополнение к TrainerForC++.sln.
#
# Visual Studio остаётся основным способом. CMake нужен, чтобы:
#   * собирать и гонять тесты без студии (CI, mingw, clang-cl);
#   * не зависеть от vcpkg: imgui и googletest подтягиваются сами.
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
#   build/TrainerTests.exe
#
# Уже скачанные исходники можно подсунуть без сети:
#   -DFETCHCONTENT_SOURCE_DIR_IMGUI=path/to/imgui
#   -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=path/to/googletest
cmake_minimum_required(VERSION 3.20)
project(TrainerForC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

if(NOT WIN32)
    message(FATAL_ERROR "Трейнер работает только под Windows. Для кросс-сборки: "
                        "-DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake")
endif()

option(TRAINER_BUILD_TESTS "Собирать тесты" ON)

include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.1-docking  # как в vcpkg: docking-experimental
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

# --- Сторонние библиотеки ---

file(GLOB_RECURSE ASMJIT_SOURCES CONFIGURE_DEPENDS
    libs/asmjit/asmjit/core/*.cpp
    libs/asmjit/asmjit/x86/*.cpp
    libs/asmjit/asmjit/axl/*.cpp)
list(FILTER ASMJIT_SOURCES EXCLUDE REGEX "_test\\.cpp$")
file(GLOB ASMTK_SOURCES CONFIGURE_DEPENDS libs/asmtk/asmtk/*.cpp)
file(GLOB_RECURSE ZYDIS_SOURCES CONFIGURE_DEPENDS libs/zydis/src/*.c libs/zycore/src/*.c)

add_library(trainer_thirdparty STATIC ${ASMJIT_SOURCES} ${ASMTK_SOURCES} ${ZYDIS_SOURCES})
target_include_directories(trainer_thirdparty PUBLIC
    libs/stb libs/asmjit libs/asmtk
    libs/zydis/include libs/zydis/src libs/zycore/include)
target_compile_definitions(trainer_thirdparty PUBLIC
    ZYDIS_STATIC_BUILD ZYCORE_STATIC_BUILD ASMJIT_STATIC ASMTK_STATIC ASMJIT_NO_AARCH64)

add_library(trainer_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(trainer_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(trainer_imgui PUBLIC d3d11 d3dcompiler dxgi dwmapi)

# --- Ядро: всё, кроме точки входа ---
#
# Отдельной библиотекой, чтобы тесты линковались с тем же кодом, что и
# трейнер. Реестр читов подключается объектами, а не из статической
# библиотеки: иначе линкер выкинет GameCheats.cpp — на него никто не
# ссылается, он только регистрирует читы статическими инициализаторами.
file(GLOB TRAINER_CORE_SOURCES CONFIGURE_DEPENDS
    core/*.cpp patches/*.cpp cheats/*.cpp platform/*.cpp ui/*.cpp app/Application.cpp)
file(GLOB TRAINER_REGISTRY_SOURCES CONFIGURE_DEPENDS cheats/registry/*.cpp)

add_library(trainer_core STATIC ${TRAINER_CORE_SOURCES})
target_include_directories(trainer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(trainer_core PUBLIC UNICODE _UNICODE NOMINMAX)
target_link_libraries(trainer_core PUBLIC trainer_thirdparty trainer_imgui
    psapi winmm ole32 shell32 user32 gdi32
    $<IF:$<BOOL:${MSVC}>,xaudio2,xaudio2_8>)

add_library(trainer_registry OBJECT ${TRAINER_REGISTRY_SOURCES})
target_link_libraries(trainer_registry PUBLIC trainer_core)

if(MSVC)
    target_compile_options(trainer_core PUBLIC /utf-8 /W3)
    target_compile_options(trainer_thirdparty PRIVATE /utf-8 /W0)
else()
    target_compile_options(trainer_thirdparty PRIVATE -w)
    target_compile_options(trainer_core PRIVATE -Wall -Wno-unknown-pragmas)
endif()

# Ресурсы. Пути в .rc записаны студией через "\\" — windres на Linux их не
# понимает, поэтому при кросс-сборке берём копию с прямыми слешами.
set(TRAINER_RC ${CMAKE_CURRENT_SOURCE_DIR}/TrainerForC++.rc)
if(CMAKE_HOST_UNIX)
    file(READ ${TRAINER_RC} _rc)
    string(REPLACE "\\\\" "/" _rc "${_rc}")
    string(REPLACE "\"ResourceExtern/" "\"${CMAKE_CURRENT_SOURCE_DIR}/ResourceExtern/" _rc "${_rc}")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/TrainerForC++.rc "${_rc}")
    set(TRAINER_RC ${CMAKE_CURRENT_BINARY_DIR}/TrainerForC++.rc)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS TrainerForC++.rc)
endif()

add_executable(Trainer WIN32 app/main.cpp ${TRAINER_RC})
target_include_directories(Trainer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
    # Точка входа — wWinMain, а mingw по умолчанию ищет WinMain.
    target_link_options(Trainer PRIVATE -municode -static)
endif()
target_link_libraries(Trainer PRIVATE trainer_core trainer_registry)

# --- Тесты ---

if(TRAINER_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
        GIT_SHALLOW    TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB TRAINER_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(TrainerTests ${TRAINER_TEST_SOURCES})
    target_link_libraries(TrainerTests PRIVATE trainer_core trainer_registry GTest::gtest_main)

    if(NOT MSVC)
        target_link_options(TrainerTests PRIVATE -static)
    endif()

    enable_testing()
    add_test(NAME TrainerTests COMMAND TrainerTests)
endif()

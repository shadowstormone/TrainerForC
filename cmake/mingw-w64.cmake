# Кросс-сборка под Windows из Linux: mingw-w64.
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
# Тесты затем запускаются через wine: wine build/TrainerTests.exe
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Заголовки Windows SDK в коде написаны как в MSVC (Windows.h, TlHelp32.h),
# а у mingw они в нижнем регистре. На регистрозависимой ФС кладём рядом
# ссылки с «виндовыми» именами.
set(_shim "${CMAKE_BINARY_DIR}/mingw-case-shim")
file(MAKE_DIRECTORY "${_shim}")
foreach(_name Windows TlHelp32 Psapi KnownFolders)
    string(TOLOWER "${_name}" _lower)
    if(NOT EXISTS "${_shim}/${_name}.h")
        file(CREATE_LINK "/usr/x86_64-w64-mingw32/include/${_lower}.h" "${_shim}/${_name}.h" SYMBOLIC)
    endif()
endforeach()
set(CMAKE_CXX_FLAGS_INIT "-isystem ${_shim}")
set(CMAKE_C_FLAGS_INIT "-isystem ${_shim}")

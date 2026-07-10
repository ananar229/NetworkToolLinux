# Native 64-bit Windows build (not recommended for ReactOS - its x86_64
# support is still very experimental; use mingw-toolchain.cmake for that).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Link against classic msvcrt.dll instead of this gcc's default Universal
# CRT (api-ms-win-crt-*.dll forwarders, only present on Windows 10+) - see
# mingw-toolchain.cmake for the full explanation.
set(CRT_FLAGS "-mcrtdll=msvcrt-os")
set(CMAKE_C_FLAGS_INIT "${CRT_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CRT_FLAGS} -static -static-libgcc -static-libstdc++")

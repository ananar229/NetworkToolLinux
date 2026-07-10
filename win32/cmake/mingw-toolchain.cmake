# Cross-compilation toolchain for building the Win32 port on Linux with
# MinGW-w64. Targets the classic Win32 API surface (no .NET/UWP) and
# builds a 32-bit (i686) binary by default: ReactOS's x86_64 support is
# still very experimental, while a 32-bit .exe runs fine on ReactOS, on
# 32-bit Windows, and on 64-bit Windows via WoW64 - one build, everywhere.
# For a native 64-bit Windows-only build, use mingw-toolchain-x86_64.cmake
# instead.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(TOOLCHAIN_PREFIX i686-w64-mingw32)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# This mingw-w64-gcc defaults to linking against the Universal CRT (the
# api-ms-win-crt-*.dll "API set" forwarder DLLs) rather than the classic
# msvcrt.dll. Those forwarders only exist on Windows 10+ (or with the UCRT
# redistributable installed) - launching a UCRT-linked .exe on ReactOS or
# older Windows fails with "The procedure entry point ... could not be
# located in ... api-ms-win-crt-*.dll". -mcrtdll=msvcrt-os links against
# the classic msvcrt.dll instead, which every Windows version and ReactOS
# ships.
set(CRT_FLAGS "-mcrtdll=msvcrt-os")
set(CMAKE_C_FLAGS_INIT "${CRT_FLAGS}")

# Static-link the MinGW runtime so the .exe doesn't need libgcc/libstdc++
# DLLs alongside it - keeps a single portable NetworkTool.exe.
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CRT_FLAGS} -static -static-libgcc -static-libstdc++")

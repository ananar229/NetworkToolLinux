cmake_minimum_required(VERSION 3.16)
project(NetworkToolWin32 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Target the NT4/2000-era Win32 API surface (WINVER 0x0500 = Windows 2000)
# so the same source builds and runs unmodified on ReactOS, not just on
# recent Windows.
add_definitions(-DUNICODE -D_UNICODE -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0500 -DWIN32_LEAN_AND_MEAN)

add_executable(NetworkTool WIN32
    src/main.c
    src/MainWindow.c
    src/AboutDialog.c
    src/SettingsDialog.c
    src/common/OutputEdit.c
    src/common/ResolveIPv4.c
    src/common/TcpQuery.c
    src/common/Translations.c
    src/common/OuiVendors.c
    src/common/DialogResize.c
    src/tabs/InfoTab.c
    src/tabs/NetstatTab.c
    src/tabs/PingTab.c
    src/tabs/LookupTab.c
    src/tabs/TracerouteTab.c
    src/tabs/WhoisTab.c
    src/tabs/FingerTab.c
    src/tabs/PortScanTab.c
    src/tabs/SpeedTab.c
    src/tabs/LanScanTab.c
    resources/app.rc
)

target_include_directories(NetworkTool PRIVATE src src/common)
target_link_options(NetworkTool PRIVATE -municode)

target_link_libraries(NetworkTool PRIVATE
    comctl32
    ws2_32
    iphlpapi
    dnsapi
    winhttp
    gdi32
    comdlg32
)

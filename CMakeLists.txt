cmake_minimum_required(VERSION 3.16)
project(NetworkTool VERSION 1.5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)

add_executable(NetworkTool
    main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/AboutDialog.h
    src/AboutDialog.cpp
    src/SettingsDialog.h
    src/SettingsDialog.cpp
    src/common/LanguageManager.h
    src/common/LanguageManager.cpp
    src/common/CommandRunner.h
    src/common/CommandRunner.cpp
    src/common/OutputConsole.h
    src/common/OutputConsole.cpp
    src/common/WhoisClient.h
    src/common/WhoisClient.cpp
    src/common/NativeTraceroute.h
    src/common/NativeTraceroute.cpp
    src/common/ProcNetInfo.h
    src/common/ProcNetInfo.cpp
    src/common/OuiVendors.h
    src/common/OuiVendors.cpp
    src/tabs/InfoTab.h
    src/tabs/InfoTab.cpp
    src/tabs/NetstatTab.h
    src/tabs/NetstatTab.cpp
    src/tabs/PingTab.h
    src/tabs/PingTab.cpp
    src/tabs/LookupTab.h
    src/tabs/LookupTab.cpp
    src/tabs/TracerouteTab.h
    src/tabs/TracerouteTab.cpp
    src/tabs/WhoisTab.h
    src/tabs/WhoisTab.cpp
    src/tabs/FingerTab.h
    src/tabs/FingerTab.cpp
    src/tabs/PortScanTab.h
    src/tabs/PortScanTab.cpp
    src/tabs/SpeedTestTab.h
    src/tabs/SpeedTestTab.cpp
    src/tabs/LanScanTab.h
    src/tabs/LanScanTab.cpp
    resources/icons.qrc
)

target_link_libraries(NetworkTool PRIVATE Qt6::Widgets Qt6::Network)
target_compile_definitions(NetworkTool PRIVATE NETWORKTOOL_VERSION="${PROJECT_VERSION}")

qt6_add_translations(NetworkTool
    TS_FILES
        translations/NetworkTool_de.ts
        translations/NetworkTool_en.ts
        translations/NetworkTool_fr.ts
        translations/NetworkTool_it.ts
        translations/NetworkTool_es.ts
    RESOURCE_PREFIX "/i18n"
)

install(TARGETS NetworkTool RUNTIME DESTINATION bin)
install(FILES packaging/networktool.desktop DESTINATION share/applications)
install(FILES resources/icons/networktool_16.png
    DESTINATION share/icons/hicolor/16x16/apps RENAME networktool.png)
install(FILES resources/icons/networktool_32.png
    DESTINATION share/icons/hicolor/32x32/apps RENAME networktool.png)
install(FILES resources/icons/networktool_48.png
    DESTINATION share/icons/hicolor/48x48/apps RENAME networktool.png)
install(FILES resources/icons/networktool_64.png
    DESTINATION share/icons/hicolor/64x64/apps RENAME networktool.png)
install(FILES resources/icons/networktool_128.png
    DESTINATION share/icons/hicolor/128x128/apps RENAME networktool.png)
install(FILES resources/icons/networktool_256.png
    DESTINATION share/icons/hicolor/256x256/apps RENAME networktool.png)
install(FILES resources/icons/networktool_512.png
    DESTINATION share/icons/hicolor/512x512/apps RENAME networktool.png)
install(FILES resources/icons/networktool_1024.png
    DESTINATION share/icons/hicolor/1024x1024/apps RENAME networktool.png)

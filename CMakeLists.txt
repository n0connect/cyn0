cmake_minimum_required(VERSION 3.16)

project(Cyn0 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/QHotkey/QHotkey
)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/Animations.cpp
    src/JsonActions.cpp
    src/JsonDbParser.cpp
    src/JsonParser.cpp
    src/KeyboardShortcuts.cpp
    src/PayloadCopySystem.cpp
    src/PersonalNotes.cpp
    src/PersonalNotesManager.cpp
    src/SearchInsideJson.cpp
    src/TrayIcon.cpp
    src/UiDesign.cpp
    third_party/QHotkey/QHotkey/qhotkey.cpp
)

# Header files
set(HEADERS
    include/mainwindow.h
    include/Animations.h
    include/JsonParser.h
    include/PersonalNotes.h
    include/UiDesign.h
    third_party/QHotkey/QHotkey/qhotkey.h
    third_party/QHotkey/QHotkey/qhotkey_p.h
)

# Form files
set(FORMS
    ui/mainwindow.ui
)

# Resource files
set(RESOURCES
    resources/qrc/command-list.qrc
    resources/qrc/fonts.qrc
    resources/qrc/icon.qrc
    resources/qrc/json-files.qrc
    resources/qrc/json-files-legacy.qrc
    resources/qrc/style-sheet.qrc
)

# Platform specific hotkey sources
if(WIN32)
    list(APPEND SOURCES third_party/QHotkey/QHotkey/qhotkey_win.cpp)
elseif(APPLE)
    list(APPEND SOURCES third_party/QHotkey/QHotkey/qhotkey_mac.cpp)
    # Qt 6.9.x Mac ARM fix equivalent
    add_compile_options(-include arm_acle.h)
else()
    list(APPEND SOURCES third_party/QHotkey/QHotkey/qhotkey_x11.cpp)
    find_package(Qt6 COMPONENTS GuiPrivate REQUIRED)
endif()

# Application Icon for Mac / Plist
if(APPLE)
    set_source_files_properties(packaging/macos/Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION .)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES} packaging/macos/Info.plist)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist)
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
endif()

# Linking Phase
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Platform specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Carbon" "-framework CoreFoundation")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE X11)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt::GuiPrivate)
endif()

# Installation target
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

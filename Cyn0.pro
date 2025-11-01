QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
INCLUDEPATH += $$PWD/QHotkey/QHotkey

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# QHotkey kaynak kodlarını projeye ekle
SOURCES += \
    Animations.cpp \
    JsonActions.cpp \
    JsonDbParser.cpp \
    JsonParser.cpp \
    KeyboardShortcuts.cpp \
    PayloadCopySystem.cpp \
    PersonalNotes.cpp \
    PersonalNotesManager.cpp \
    SearchInsideJson.cpp \
    TrayIcon.cpp \
    UiDesign.cpp \
    main.cpp \
    mainwindow.cpp \
    QHotkey/QHotkey/qhotkey.cpp

# Platform-specific QHotkey implementations
win32 {
    SOURCES += QHotkey/QHotkey/qhotkey_win.cpp
    LIBS += -luser32
}

macx {
    SOURCES += QHotkey/QHotkey/qhotkey_mac.cpp
    LIBS += -framework Carbon -framework CoreFoundation
    QMAKE_INFO_PLIST = Info.plist
}

unix:!macx {
    SOURCES += QHotkey/QHotkey/qhotkey_x11.cpp
    # Qt 6 doesn't have x11extras, use native X11
    LIBS += -lX11
}

HEADERS += \
    Animations.h \
    JsonParser.h \
    PersonalNotes.h \
    mainwindow.h \
    QHotkey/QHotkey/qhotkey.h \
    QHotkey/QHotkey/qhotkey_p.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    icons/icon.ico \
    style.qss \
    arglist\commands.json

RESOURCES += \
    fonts.qrc \
    icon.qrc \
    json-files.qrc \
    style-sheet.qrc

#-------------------------------------------------
#
# Project created by QtCreator 2013-09-16T12:21:58
#
#-------------------------------------------------

QT       += core

TARGET = samugen-gui
CONFIG   -= app_bundle

TEMPLATE = app

ParentDirectory = ../../../..

UI_DIR = "$$ParentDirectory/build/samugen-gui"
MOC_DIR = "$$ParentDirectory/build/samugen-gui"
OBJECTS_DIR = "$$ParentDirectory/build/samugen-gui"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}

SOURCES += main.cpp \
    samugengui.cpp

HEADERS += \
    samugengui.h

INCLUDEPATH += /usr/local/include $$ParentDirectory/src $$ParentDirectory/src/client
LIBS += -L$$ParentDirectory/lib -lsac
QT += network

target.path = /usr/local/bin
INSTALLS += target

# For version without JACK, uncomment the next line
#DEFINES += SAC_NO_JACK
contains(DEFINES, SAC_NO_JACK) {
    message(samugen-gui: SAC_NO_JACK defined: configuring without JACK...)
}
else {
    message(samugen-gui: configuring with JACK...)
    LIBS += -ljack
}


message(samugen-gui.pro complete)

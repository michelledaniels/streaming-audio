#-------------------------------------------------
#
# Project created by QtCreator 2014-01-15T15:29:08
#
#-------------------------------------------------

QT       += network

QT       -= gui

TARGET = samrender
TEMPLATE = lib

ParentDirectory = ../..

UI_DIR = "$$ParentDirectory/build/libsamrender"
MOC_DIR = "$$ParentDirectory/build/libsamrender"
OBJECTS_DIR = "$$ParentDirectory/build/libsamrender"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/lib/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/lib"
}

DEFINES += LIBSAMRENDER_LIBRARY
CONFIG += staticlib

# For debugging, comment out
#CONFIG -= warn_on
#DEFINES += QT_NO_DEBUG_OUTPUT

# uncomment to build for 32-bit arch (mac only)
#CONFIG+=x86

SOURCES += samrenderer.cpp \
        ../osc.cpp

HEADERS += samrenderer.h\
        libsamrender_global.h \
        ../osc.h

INCLUDEPATH += /usr/local/include

#target.path = /usr/local/lib
#INSTALLS += target

message(libsamrender.pro complete)

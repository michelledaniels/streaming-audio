#-------------------------------------------------
#
# Project created by QtCreator 2012-09-28T13:39:53
#
#-------------------------------------------------

QT       += network

QT       -= gui

TARGET = sac
TEMPLATE = lib

ParentDirectory = ../..

UI_DIR = "$$ParentDirectory/build"
MOC_DIR = "$$ParentDirectory/build"
OBJECTS_DIR = "$$ParentDirectory/build"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/lib/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/lib"
}

DEFINES += LIBSAC_LIBRARY
CONFIG += staticlib

# For debugging, comment out
CONFIG -= warn_on
DEFINES += QT_NO_DEBUG_OUTPUT

# For version without JACK, uncomment the next line
#DEFINES += SAC_NO_JACK
contains(DEFINES, SAC_NO_JACK) {
    message(libsac: SAC_NO_JACK defined: building without JACK...)
}
else {
    message(libsac: building with JACK...)
    LIBS += -ljack
}

SOURCES += \
    sam_client.cpp \
    sac_audio_interface.cpp \
    rtpsender.cpp \
    ../rtp.cpp \
    ../rtcp.cpp \
    ../osc.cpp

HEADERS +=\
    libsac_global.h \
    sam_client.h \
    sac_audio_interface.h \
    rtpsender.h \
    ../rtp.h \
    ../rtcp.h \
    ../osc.h

INCLUDEPATH += /usr/local/include ../

target.path = /usr/local/lib
INSTALLS += target

message(libsac.pro complete)

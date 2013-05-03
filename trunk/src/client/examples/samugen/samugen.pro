#-------------------------------------------------
#
# Project created by QtCreator 2012-11-16T15:50:58
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = samugen
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

ParentDirectory = ../../../..

UI_DIR = "$$ParentDirectory/build/samugen"
MOC_DIR = "$$ParentDirectory/build/samugen"
OBJECTS_DIR = "$$ParentDirectory/build/samugen"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}

SOURCES += main.cpp

INCLUDEPATH += /usr/local/include $$ParentDirectory/src $$ParentDirectory/src/client
LIBS += -L$$ParentDirectory/lib -lsac
QT += network

target.path = /usr/local/bin
INSTALLS += target

# For version without JACK, uncomment the next line
#DEFINES += SAC_NO_JACK
contains(DEFINES, SAC_NO_JACK) {
    message(samugen: SAC_NO_JACK defined: building without JACK...)
}
else {
    message(samugen: building with JACK...)
    LIBS += -ljack
}


message(samugen.pro complete)

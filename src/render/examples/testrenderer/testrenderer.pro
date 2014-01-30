#-------------------------------------------------
#
# Project created by QtCreator 2014-01-23T13:00:26
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = testrenderer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# For debugging, comment out
CONFIG -= warn_on
DEFINES += QT_NO_DEBUG_OUTPUT

ParentDirectory = ../../../..

UI_DIR = "$$ParentDirectory/build/testrenderer"
MOC_DIR = "$$ParentDirectory/build/testrenderer"
OBJECTS_DIR = "$$ParentDirectory/build/testrenderer"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}

SOURCES += main.cpp \
    testrenderer.cpp

INCLUDEPATH += /usr/local/include $$ParentDirectory/src $$ParentDirectory/src/render
LIBS += -L$$ParentDirectory/lib -lsamrender -ljack
QT += network

target.path = /usr/local/bin
INSTALLS += target

message(testrenderer.pro complete)

HEADERS += \
    testrenderer.h

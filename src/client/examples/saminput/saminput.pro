#-------------------------------------------------
#
# Project created by QtCreator 2012-11-16T15:50:58
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = saminput
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

ParentDirectory = ../../../..

UI_DIR = "$$ParentDirectory/build/saminput"
MOC_DIR = "$$ParentDirectory/build/saminput"
OBJECTS_DIR = "$$ParentDirectory/build/saminput"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}

SOURCES += main.cpp

INCLUDEPATH += /usr/local/include $$ParentDirectory/src $$ParentDirectory/src/client
LIBS += -L$$ParentDirectory/lib -lsac -ljack
QT += network

target.path = /usr/local/bin
INSTALLS += target

message(saminput.pro complete)

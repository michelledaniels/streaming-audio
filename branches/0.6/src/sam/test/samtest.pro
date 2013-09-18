#-------------------------------------------------
#
# Project created by QtCreator 2012-10-01T15:02:28
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = samtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

ParentDirectory = ../../..

UI_DIR = "$$ParentDirectory/build/samtest"
MOC_DIR = "$$ParentDirectory/build/samtest"
OBJECTS_DIR = "$$ParentDirectory/build/samtest"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}

SOURCES += samtest_main.cpp

HEADERS += \
    samtester.h

INCLUDEPATH += $$ParentDirectory/src $$ParentDirectory/src/client
LIBS += -L$$ParentDirectory/lib -lsac -ljack

message(samtest.pro complete)

ParentDirectory = ../../

UI_DIR = "$$ParentDirectory/build"
MOC_DIR = "$$ParentDirectory/build"
OBJECTS_DIR = "$$ParentDirectory/build"

CONFIG(debug, debug|release) {
    DESTDIR = "$$ParentDirectory/bin/debug"
}
CONFIG(release, debug|release) {
    DESTDIR = "$$ParentDirectory/bin"
}


SOURCES += sam_main.cpp \
    sam.cpp \
    sam_app.cpp \
    jack_util.cpp \
    ../osc.cpp \
    ../rtp.cpp \
    ../rtcp.cpp \
    rtpreceiver.cpp \
    samui.cpp

HEADERS += sam.h \
    sam_app.h \
    jack_util.h \
    ../osc.h \
    ../rtp.h \
    ../rtcp.h \
    rtpreceiver.h \
    samui.h

FORMS += \
    samui.ui

TARGET = sam

INCLUDEPATH += /usr/local/include $$ParentDirectory/src $$ParentDirectory/client

# For debugging, comment out
CONFIG -= warn_on
DEFINES += QT_NO_DEBUG_OUTPUT

LIBS += -ljack
#QT -= gui
QT += network

macx {
  message(Mac OS X)
  CONFIG -= app_bundle
}
linux-g++ {
  message(Linux)
}
linux-g++-64 {
  message(Linux 64bit)
}

target.path = /usr/local/bin
INSTALLS += target

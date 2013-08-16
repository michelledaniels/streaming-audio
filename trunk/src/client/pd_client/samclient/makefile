NAME=samclient~
CSYM=samclient_tilde

QTSDK_DIR = /usr/local/Trolltech/Qt-4.8.5
SA_DIR = /Users/zachary/Calit2/software/sam/streaming-audio/zach/streaming-audio

ARCH = -arch i386 -arch x86_64

CUSTOMINCLUDES = -I$(QTSDK_DIR)/mkspecs/macx-g++ -I$(QTSDK_DIR)/lib/QtCore.framework/Versions/4/Headers -I$(QTSDK_DIR)/include/QtCore -I$(QTSDK_DIR)/lib/QtNetwork.framework/Versions/4/Headers -I$(QTSDK_DIR)/include/QtNetwork -I$(QTSDK_DIR)/include -I/usr/local/include -F$(QTSDK_DIR)/lib -I$(SA_DIR)/src -I$(SA_DIR)/src/client

CUSTOMLIBS = -F$(QTSDK_DIR)/lib -framework QtCore -framework QtNetwork -L$(SA_DIR)/lib -lsac

include ../makefile.subdir

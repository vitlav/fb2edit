HEADERS = \
    fb2main.h \
    fb2read.h \
    fb2doc.h

SOURCES = \
    fb2app.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2doc.cpp

RESOURCES = \
    fb2edit.qrc

TARGET = fb2edit

VERSION = 0.01.1

QT += xml

LIBS += -lqscintilla2

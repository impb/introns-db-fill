#-------------------------------------------------
#
# Project created by QtCreator 2015-07-20T15:25:11
#
#-------------------------------------------------

QT       += core sql
QT       -= gui

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_LIBS += -lz

TARGET = introns_db_fill
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    gbkparser.cpp \
    database.cpp \
    gzipreader.cpp \
    iniparser.cpp \
    logger.cpp

HEADERS += \
    gbkparser.h \
    structures.h \
    database.h \
    gzipreader.h \
    iniparser.h \
    logger.h

RESOURCES +=

DISTFILES += \
    create_database.sql \
    README.md

isEmpty(PREFIX): PREFIX = /usr/local

INSTALLS = binary
binary.files = $${TARGET}
binary.path = $$PREFIX/bin




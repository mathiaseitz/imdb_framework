TARGET = generate_filelist
TEMPLATE = app
include(../../common.pri)

#CONFIG += console
#CONFIG -= qt
#QT += core
#QT += gui

SOURCES += main.cpp io/filelist.cpp

HEADERS += util/types.hpp \
io/cmdline.hpp \
io/filelist.hpp

TARGET = generate_mapping
TEMPLATE = app
include(../../common.pri)

LIBS += -lboost_filesystem-mt -lboost_system-mt

CONFIG += console

SOURCES += main.cpp io/filelist.cpp

HEADERS += util/types.hpp \
    io/property_writer.hpp \
    io/cmdline.hpp \
    io/filelist.hpp

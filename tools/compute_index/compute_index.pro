TEMPLATE = app
TARGET = compute_index
include(../../common.pri)

CONFIG += console

QMAKE_CXXFLAGS += -fopenmp
LIBS += -lgomp

LIBS += -lboost_iostreams-mt

HEADERS += search/inverted_index.hpp \
util/quantizer.hpp

SOURCES = main.cpp \
util/quantizer.cpp \
search/inverted_index.cpp \
search/tf_idf.cpp

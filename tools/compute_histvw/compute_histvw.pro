TEMPLATE = app
TARGET = compute_histvw
include(../../common.pri)

CONFIG += console

# uncomment following lines to enable openmp parallelization
#QMAKE_CXXFLAGS += -fopenmp
#LIBS += -lgomp

SOURCES += main.cpp \
io/filelist.cpp \
util/quantizer.cpp


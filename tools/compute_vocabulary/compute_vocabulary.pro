TEMPLATE = app
TARGET = compute_vocabulary
include(../../common.pri)

CONFIG += qt \
    warn_on \
    thread \
    console

SOURCES = main.cpp
LIBS += -lboost_thread-mt

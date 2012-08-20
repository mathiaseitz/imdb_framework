TARGET = image_search
include(../../common.pri)

CONFIG += console
TEMPLATE = app
LIBS += -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgproc

SOURCES += main.cpp \
search/linear_search_manager.cpp \
search/bof_search_manager.cpp \
search/inverted_index.cpp \
search/tf_idf.cpp \
descriptors/generator.cpp \
descriptors/shog.cpp \
descriptors/galif.cpp \
descriptors/utilities.cpp \
descriptors/image_sampler.cpp \
io/filelist.cpp \
util/quantizer.cpp

HEADERS +=

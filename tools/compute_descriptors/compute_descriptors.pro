TARGET = compute_descriptors
include(../../common.pri)

CONFIG += console

TEMPLATE = app

LIBS += -lboost_thread-mt \
        -lopencv_highgui \
        -lopencv_features2d \
        -lopencv_core \
        -lopencv_imgproc

SOURCES += main.cpp \
    io/filelist.cpp \
    descriptors/generator.cpp \
    descriptors/tinyimage.cpp \
    descriptors/gist.cpp \
    descriptors/shog.cpp \
    descriptors/galif.cpp \
    descriptors/image_sampler.cpp \
    descriptors/utilities.cpp \
    compute_descriptors.cpp \
    ordered_push_back.cpp


HEADERS += util/types.hpp \
    io/io.hpp \
    io/property_writer.hpp \
    io/cmdline.hpp \
    io/filelist.hpp \
    descriptors/image_sampler.hpp \
    descriptors/utilities.hpp \
    descriptors/generator.hpp \
    descriptors/tinyimage.hpp \
    descriptors/gist.hpp \
    descriptors/shog.hpp \
    descriptors/galif.hpp \
    compute_descriptors.hpp \
    ordered_push_back.hpp

CONFIG(release, release|debug) {
    SUBFOLDER = "release"
    DEFINES += NDEBUG
    QMAKE_CXXFLAGS_RELEASE += -O3
    CONFIG += sse sse2
}
else {
    SUBFOLDER = "debug"
}

DESTDIR = $$SUBFOLDER
OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

macx: CONFIG -= app_bundle
macx: QMAKE_CXXFLAGS += -Wno-missing-field-initializers

message("Using build directory: " $$DESTDIR)


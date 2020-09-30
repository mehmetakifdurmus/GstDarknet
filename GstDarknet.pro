TEMPLATE = lib
TARGET = gstdarknet
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        gstdarknet.c

#GStreamer
INCLUDEPATH += /usr/include/gstreamer-1.0/ \
               /usr/include/glib-2.0/ \
               /usr/lib/glib-2.0/include/

LIBS += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstbase-1.0

#Darknet
INCLUDEPATH += $(HOME)/darknet/include/


HEADERS += \
    gstdarknet.h

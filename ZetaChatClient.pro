QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Lab2_client
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
        myclient.cpp \
        myserver.cpp

HEADERS  += dialog.h \
    myclient.h \
    myserver.h


FORMS    += dialog.ui

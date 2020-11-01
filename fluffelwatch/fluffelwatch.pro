#-------------------------------------------------
#
# Project created by QtCreator 2020-10-31T12:36:36
#
#-------------------------------------------------

QT       += core gui gui-private

QMAKE_LFLAGS += -no-pie

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fluffelwatch
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -L/usr/X11/lib -lX11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    splitdata.cpp \
    fluffeltimer.cpp \
    qxt/qxtglobalshortcut_x11.cpp \
    qxt/qxtglobalshortcut.cpp \
    fluffelmemorythread.cpp

HEADERS += \
        mainwindow.h \
    splitdata.h \
    fluffeltimer.h \
    qxt/qxtglobalshortcut_p.h \
    qxt/qxtglobalshortcut.h \
    qxt/xcbkeyboard.h \
    fluffelmemorythread.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    fluffelwatch.qrc

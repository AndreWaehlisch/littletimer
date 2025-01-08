QT += core gui widgets

win32:qtHaveModule(gui-private) {
    QT          += gui-private
    DEFINES     += LITTLETIMER_DO_WIN_BRINGTOFRONT
}

qtHaveModule(winextras) {
    QT          += winextras
    DEFINES     += LITTLETIMER_DO_WIN_TASKBAR_PROGRESSBAR
}

TARGET       = littletimer
TEMPLATE     = app
SOURCES     += main.cpp mainwindow.cpp simpletimer.cpp

HEADERS     += mainwindow.h simpletimer.h

FORMS       += mainwindow.ui
RESOURCES   += ../resource/images.qrc
CONFIG      += c++17

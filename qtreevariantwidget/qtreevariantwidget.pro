#-------------------------------------------------
#
# Project created by QtCreator 2015-05-12T19:11:37
#
#-------------------------------------------------

QT = core gui widgets

TARGET = qtreevariantwidget
TEMPLATE = lib

CONFIG += static
CONFIG += c++11

INCLUDEPATH += ../qvariantmodel
DEPENDPATH += ../qvariantmodel

SOURCES += \
    qtreevariantwidget.cpp \
    qtreevariantitemdelegate.cpp

HEADERS += \
    qtreevariantwidget.h \
    qtreevariantitemdelegate.h

FORMS += \
    qtreevariantwidget.ui

#-------------------------------------------------
#
# Project created by QtCreator 2015-05-12T19:11:37
#
#-------------------------------------------------

QT = core gui widgets

TARGET = QVariantEditor
TEMPLATE = app

CONFIG += c++11

INCLUDEPATH += ../qtreevariantwidget ../qvariantmodel
DEPENDPATH += ../qtreevariantwidget ../qvariantmodel
LIBS += -L../qtreevariantwidget -lqtreevariantwidget -L../qvariantmodel -lqvariantmodel

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS += mainwindow.h

FORMS += mainwindow.ui


OTHER_FILES += \
    icons/QVariantEditorTheme/index.theme

RESOURCES += \
    theme.qrc


TRANSLATIONS += \
    ../qve_fr_FR.ts

TradFile.path = $$OUT_PWD
TradFile.files += qve_fr_FR.qm
TradFile.files += $$[QT_INSTALL_TRANSLATIONS]/qt_fr.qm

INSTALLS += TradFile

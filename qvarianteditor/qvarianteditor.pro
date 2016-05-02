#-------------------------------------------------
#
# Project created by QtCreator 2015-05-12T19:11:37
#
#-------------------------------------------------

QT = core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QVariantEditor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qtreevariantwidget.cpp

HEADERS  += mainwindow.h \
    qtreevariantwidget.h

FORMS    += mainwindow.ui \
    qtreevariantwidget.ui


OTHER_FILES += \
    icons/QVariantEditorTheme/index.theme

RESOURCES += \
    theme.qrc


TRANSLATIONS += \
    qve_fr_FR.ts

TradFile.path = $$OUT_PWD
TradFile.files += qve_fr_FR.qm
TradFile.files += $$[QT_INSTALL_TRANSLATIONS]/qt_fr.qm

INSTALLS += TradFile

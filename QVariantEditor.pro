#-------------------------------------------------
#
# Project created by QtCreator 2015-05-12T19:11:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QVariantEditor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qvarianttree.cpp \
    qvarianttreeitemmodel.cpp \
    qvariantitemdelegate.cpp \
    qtablevarianttree.cpp

HEADERS  += mainwindow.h \
    qvarianttree.h \
    qvarianttreeitemmodel.h \
    qvariantitemdelegate.h \
    qtablevarianttree.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    icons/QVariantEditorTheme/index.theme

TRANSLATIONS += \
    qve_fr_FR.ts

RESOURCES += \
    theme.qrc

TradFile.path = $$OUT_PWD
TradFile.files += qve_fr_FR.qm

INSTALLS += TradFile

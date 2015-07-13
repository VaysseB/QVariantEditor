QT = core testlib

TARGET   = tst_treegsd
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


HEADERS += \
    tst_treegsd.h

SOURCES += \
    tst_treegsd.cpp


INCLUDEPATH += "$$_PRO_FILE_PWD_/../qvarianttree"
DEPENDPATH  += "$$_PRO_FILE_PWD_/../qvarianttree"
LIBS += -L"$$OUT_PWD/../qvarianttree/" -lqvarianttree

#include "mainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>


inline QPoint operator +(const QPoint& p, const QSize& s) {
    return QPoint(p.x() + s.width(), p.y() + s.height());
}


#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QVariantEditor");
    app.setApplicationVersion("1.1");
    app.setApplicationDisplayName(app.applicationName());

    QString locale = QLocale::system().name();
    QString appTranslationsPath = QDir(app.applicationDirPath())
            .filePath(QLatin1Literal("translations"));
    // translations QVariantEditor
    QTranslator translatorQve;
    QString qmQve = QString("qve_") + locale;
    if (translatorQve.load(qmQve, appTranslationsPath))
        Q_ASSERT(app.installTranslator(&translatorQve));

    // translations Qt
    QTranslator translatorQt;
    QString qmQt = QString("qt_") + locale;
    if (translatorQt.load(qmQt, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        Q_ASSERT(app.installTranslator(&translatorQt));
    else if (translatorQt.load(qmQt, appTranslationsPath))
        Q_ASSERT(app.installTranslator(&translatorQt));
    else {
        qmQt = QString("qt_") + locale.section('_', 0, 0);
        if (translatorQt.load(qmQt, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            Q_ASSERT(app.installTranslator(&translatorQt));
        else if (translatorQt.load(qmQt, appTranslationsPath))
            Q_ASSERT(app.installTranslator(&translatorQt));
    }

    // icon theme (if none)
    if (QIcon::themeName().isEmpty()) {
        QStringList themePaths = QIcon::themeSearchPaths();
        themePaths.append(QLatin1Literal("icons"));
        QIcon::setThemeSearchPaths(themePaths);
        QIcon::setThemeName("elementary-xfce-lighten");
    }

    //
    MainWindow w;
    w.show();

    // centering mainwindow
    const QSize size = w.size();
    const QRect screenRect = QApplication::desktop()->availableGeometry(
                QApplication::desktop()->primaryScreen());
    w.move(screenRect.topLeft() + (screenRect.size() - size) * 0.5);

    return app.exec();
}

#include "mainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QTranslator>
#include <QLibraryInfo>


inline QPoint operator +(const QPoint& p, const QSize& s) {
    return QPoint(p.x() + s.width(), p.y() + s.height());
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QVariantEditor");
    app.setApplicationVersion("1.0");
    app.setApplicationDisplayName(app.applicationName());

    // traduction
    QString locale = QLocale::system().name();
    QTranslator translatorQve;
    if (translatorQve.load(QString("qve_") + locale,
                           app.applicationDirPath()))
        app.installTranslator(&translatorQve);
    QTranslator translatorQt;
    if (translatorQt.load(QString("qt_") + locale.section('_', 0, 0),
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath)) ||
            translatorQt.load(QString("qt_") + locale.section('_', 0, 0),
                              app.applicationDirPath()))
        app.installTranslator(&translatorQt);

    // icon theme (if empty)
    if (QIcon::themeName().isEmpty())
        QIcon::setThemeName("QVariantEditorTheme");

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

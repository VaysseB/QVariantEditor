#include "mainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QTranslator>


#include "qvarianttree.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // traduction
    QString locale = QLocale::system().name();
    QTranslator translator;
    if (translator.load(QString("qve_") + locale,
                        app.applicationDirPath()))
        app.installTranslator(&translator);

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
    w.move(QPoint(screenRect.left() + (screenRect.width() - size.width()) * 0.5,
                  screenRect.top() + (screenRect.height() - size.height()) * 0.5));

    return app.exec();
}

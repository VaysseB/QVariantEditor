#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTreeVariantWidget;


namespace Ui {
class mainwindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    virtual ~MainWindow();

protected:
    /**
     * @brief Catch close event to prevent unwanted close without saving.
     */
    void closeEvent(QCloseEvent *event);

private slots:
    void new_();
    void open();
    void save(bool forceAsk = false);
    void saveAs() { save(true); }
    void close();
    void quit();
    void about();

    void updateWithTab(int index);
    void tabNameChanged();
    void tabModified();

    void openRecentFile(); // use sender
    void clearRecentFiles();

    void actionShowSidebar();

private:
    void connectMenu();
    void bindMenuActions();

    void openNewTab(const QString& filename);

    void connectTab(QTreeVariantWidget* tvw);

    void createRecentFileActions();
    void addToRecentFiles(const QString& filename);
    void updateRecentFileActions();

private:
    Ui::mainwindow *ui;

    enum { MaxRecentFiles = 5 };
    QAction* m_recentFileActs[MaxRecentFiles];
    QAction* mp_recentFileSeparator;
    QAction* mp_clearRecentFiles;
};

#endif // MAINWINDOW_H

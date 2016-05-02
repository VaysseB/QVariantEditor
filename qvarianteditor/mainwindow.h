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

private:
    Ui::mainwindow *ui;
};

#endif // MAINWINDOW_H

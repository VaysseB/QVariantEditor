#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMessageBox>

class QVariantTree;
class QVariantTreeItemModel;


namespace Ui {
class mainwindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void new_();
    void open();
    void save(bool force = false);
    void saveAs() { save(true); }
    void close();
    bool quit();

    void about();

    void modelChanged();

    void fullReload();

private:
    void clear();

    // Return Cancel / Save / actionButton
    QMessageBox::StandardButton askBeforeLoseDatas(
            const QString& title,
            const QString& question,
            QMessageBox::StandardButton actionButton
            = QMessageBox::NoButton);


    void reloadUI();
    void reloadMenu();

    void initTable();

    bool isSomeDatas() const;

    QVariantTreeItemModel* model() const;

    QStringList displayableAddress() const;

    enum MessageDisplayType {
        ShowTemporary = 0x00,
        ClearAndShowTemporary = 0x01,
        ShowPermanent = 0x02
    };

    void showStatusMessage(QString message,
                           MessageDisplayType type,
                           int timeout = 0);
    void clearStatusTemporaryMessage();
    void clearStatusPermanentMessage();

    void setTitle(QString title = QString());

private:
    Ui::mainwindow *ui;

    QString _permanentMessage;

    QString _currentFilename;
};

#endif // MAINWINDOW_H

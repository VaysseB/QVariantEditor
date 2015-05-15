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
    /**
     * @brief Catch close event to prevent unwanted close without saving.
     */
    void closeEvent(QCloseEvent *event);

private slots:
    /**
     * @brief Clear the window for a new content.
     */
    void new_();
    /**
     * @brief Ask for opening a file.
     */
    void open();
    /**
     * @brief Save the current content.
     * @param force If true, it will ask where to save
     */
    void save(bool force = false);
    /**
     * @brief Force asking to save if required.
     * Provided for convenience.
     */
    void saveAs() { save(true); }
    /**
     * @brief Close the current edit file.
     */
    void close();
    /**
     * @brief Quit properly the application.
     * @return True if it is ok to quit.
     */
    bool quit();

    /**
     * @brief Display the about window.
     */
    void about();

    /**
     * @brief Register that the window is modified.
     * Called whenever there is a change in the tree.
     */
    void modelChanged();

    /**
     * @brief Reload the window after any movment into to tree.
     */
    void fullReload();

private:
    /**
     * @brief Clear all.
     * Empty the table, the model, the current filepath.
     */
    void clear();

    /**
     * @brief Ask to save if needed.
     * @param title The window title
     * @param question The question to save
     * @param actionButton Auxillary button (with Cancel and Save)
     * @return The button returned
     */
    QMessageBox::StandardButton askBeforeLoseDatas(
            const QString& title,
            const QString& question,
            QMessageBox::StandardButton actionButton
            = QMessageBox::NoButton);

    /**
     * @brief Update the UI. Disable / Enable widgets depending on the
     * current situation.
     */
    void reloadUI();
    /**
     * @brief Update the menu. Disable / Enable actions depending on the
     * current situation.
     */
    void reloadMenu();

    /**
     * @brief Inner helper for retrieving the tree model.
     * @return The tree model
     */
    QVariantTreeItemModel* model() const;

    /**
     * @brief Return the ordered list of keys of the address of the tree.
     * @return Ordered keys of the address
     */
    QStringList displayableAddress() const;

    /**
     * @brief Enum to use when changing the status bar message.
     */
    enum MessageDisplayType {
        /** @brief Show temporary the given message. */
        ShowTemporary = 0x00,
        /** @brief Clear permanent message and show temporary the given message. */
        ClearAndShowTemporary = 0x01,
        /** @brief Show permanently the given message. */
        ShowPermanent = 0x02
    };

    /**
     * @brief Display the given message into the statusbar.
     * @param message The message to show
     * @param type Permanent / Temporary
     * @param timeout Timeout for temporary message
     */
    void showStatusMessage(QString message,
                           MessageDisplayType type,
                           int timeout = 0);
    /**
     * @brief Clear statusbar message. If a permanent exists, the message
     * is displayed.
     */
    void clearStatusTemporaryMessage();
    /**
     * @brief Clear statusbar message and the permanent message.
     */
    void clearStatusPermanentMessage();

    /**
     * @brief Set the window title.
     * If empty, it guess it's a new file.
     * If null, there is no file.
     * @param title The title to edit
     */
    void setTitle(QString title = QString());

private:
    Ui::mainwindow *ui;

    /**
     * @brief Storage of the permanent message.
     * Statusbar can only display "temporary" message.
     */
    QString _permanentMessage;

    /**
     * @brief Current edit filepath.
     * If empty, it may be a new file as nothing to edit.
     */
    QString _currentFilePath;
};

#endif // MAINWINDOW_H

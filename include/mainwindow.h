#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "qlistwidget.h"
#include <QMainWindow>
// QHotkey for all platforms
#include <QHotkey>
#include <QApplication>
#include <QDebug>
// "Minimize to Tray" (Sistem Tepsisine Küçültme)
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <Animations.h>
#include <JsonParser.h>
#include <QGraphicsDropShadowEffect>
#include <QStack>
#include <QStandardPaths>
#include <QDir>
#include "DatabaseManager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Programin GUI kisminin calistigi Class yapisi.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void toggleVisibility();
    void on_lineEdit_textChanged(const QString &text);
    void on_listWidget_itemActivated(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QHotkey *hotkey;                                // QHotkey for all platforms
    bool isVisibleFlag;
    bool isFirstRun;
    QString m_customFontFamily;
    bool m_isShowingContent;
    QString currentPath;
    QStack<QString> pathHistory;
    QStack<int> selectedIndexHistory;               // Liste seçim hafızası
    QJsonObject commandData;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QAction *restoreAction;
    QAction *quitAction;

    QStringList pathHistory;
    QStack<int> selectedIndexHistory;               // Liste seçim hafızası
    QJsonObject commandData;
    QStringList currentPayloads;
    int currentPayloadIndex;

    void showCommandInfo(const QString &keyword);
    void createTrayIcon();
    void fixUiCenter();
    void loadCommandData(const QString& filePath);
    void showMainMenu();
    void loadCustomFont();
    void navigateBack();
    QString getDisplayName(const QString &key);
    void recursiveSearchInAllJsons(const QString &keyword, QStringList &matches, const QString &currentJsonPath = "", const QString &pathPrefix = "");
    QJsonObject loadJsonFile(const QString &filePath);
    void getAllJsonPaths(const QString &startPath, QStringList &jsonPaths, const QString &pathPrefix = "");

    void setupKeyboardShortcuts();

    // Payload kopyalama fonksiyonları
    void extractPayloads(const QString &content);
    void highlightPayload(int index);
    void copyCurrentPayload();
    void selectNextPayload();
    void selectPreviousPayload();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
    // SQLite ve Veri Migration Yönetimi
    void initSQLiteMigration();
    void recursiveDBSearchMigration(const QString &searchPath, const QString &pathPrefix);
};

#endif // MAINWINDOW_H

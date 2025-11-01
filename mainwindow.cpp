#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>
#include <QEvent>
#include <QPropertyAnimation>

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isVisibleFlag(true)
    , isFirstRun(true)
    , m_isShowingContent(false)
    , currentPath(":/json/index.json")
{
    // Font Yükleme
    loadCustomFont();

    // UI Settings
    ui->setupUi(this);
    ui->lineEdit->installEventFilter(this);
    ui->infoBrowser->installEventFilter(this);

    // TASKBAR'DAN GİZLEME İÇİN
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // Pencere boyutunu sabit yap (resize edilemesin)
    setFixedSize(547, 296);

    // Set Visual Things
    fixUiCenter();

    // Personal Notes sistemini başlat --
    initPersonalNotes();

    // Tray Icon
    createTrayIcon();
    trayIcon->show();

    // QHotkey setup for all platforms
    hotkey = new QHotkey(QKeySequence("Shift+Space"), true, this);
    if(!hotkey->isRegistered()){
        QMessageBox::critical(this, "Error",
            "Failed to register the global hotkey (Shift+Space)!\n\n"
            "Possible reasons:\n"
            "- Another application is using this combination\n"
#ifdef Q_OS_MAC
            "- Accessibility permissions not granted (check System Preferences > Security & Privacy > Accessibility)\n"
#endif
            "\nPlease close other applications or change the hotkey combination.",
            QMessageBox::Ok);
        QApplication::quit();
    }
    connect(hotkey, &QHotkey::activated, this, &MainWindow::toggleVisibility, Qt::QueuedConnection);

    // Tray Icon bağlantıları
    connect(restoreAction, &QAction::triggered, this, [=]() {
        qDebug() << "[Tray Menu] Clicked Open button.";
        toggleVisibility();
    });

    connect(quickNoteAction, &QAction::triggered, this, [=]() {
        qDebug() << "[Tray Menu] Clicked Quick Note button.";
        showQuickNoteDialog();
    });

    connect(quitAction, &QAction::triggered, this, [=]() {
        qDebug() << "[Tray Menu] Clicked Exit button.";
        if (isVisibleFlag) {
            toggleVisibility();
            QTimer::singleShot(400, []() {
                QApplication::quit();
            });
        } else {
            QApplication::quit();
        }
    });

    connect(trayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            toggleVisibility();
        }
    });

    // Diğer bağlantılar
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::on_lineEdit_textChanged);
    ui->listWidget->setFocusPolicy(Qt::NoFocus); // Focus alamaz, sadece lineEdit alır

    // Sadece mor outline ekle (diğer stilleri bozmadan)
    ui->listWidget->setStyleSheet("QListWidget { outline: 2px solid #667eea; }");

    // Klavye kısayollarını ayarla (SPACE tuşu ile menü)
    setupKeyboardShortcuts();

    if (isFirstRun) {
        loadCommandData(":/json/index.json");
        // Personal Notlar'ı ana menüye ekle
        mergePersonalNotesIntoMenu();
        showMainMenu();
        fadeInOnStart(this);
        isFirstRun = false;
    }
}

// Destructor
MainWindow::~MainWindow()
{
    // Program kapanırken yedekleme yap
    qDebug() << "[MainWindow] Destructor - Yedekleme yapılıyor";
    backupPersonalNotes();

    delete ui;
}

// Mevcut diğer fonksiyonların implementasyonları burada devam eder...
// (toggleVisibility, createTrayIcon, vb. fonksiyonlar aynı kalır)

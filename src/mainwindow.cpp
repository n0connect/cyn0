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

    // Database Başlat
    if (DatabaseManager::instance().initDatabase()) {
        qDebug() << "[Database] Başarılı şekilde init edildi.";
        initSQLiteMigration(); // Eğer boş ise QRC'den migrate et!
    }

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
        showMainMenu();
        fadeInOnStart(this);
        isFirstRun = false;
    }
}

// Destructor
MainWindow::~MainWindow()
{
    delete ui;
}

// Mevcut diğer fonksiyonların implementasyonları burada devam eder...
// (toggleVisibility, createTrayIcon, vb. fonksiyonlar aynı kalır)

// ====== SQLITE MIGRATION ======
void MainWindow::initSQLiteMigration()
{
    // Veritabanı içinde search_items veya documents dolu mu kontrol et
    // Doluysa zaten migrate edilmiştir (veya check data size)
    // Eğer index.json verisi documents tablosunda yoksa her şeyi içeri it
    QString rootData = DatabaseManager::instance().getDocument(":/json/index.json");
    if (rootData.isEmpty()) {
        qDebug() << "[Migration] Veritabanı boş, JSON'dan SQLite'a migration başlıyor...";
        
        // 1. Dosya tabanlı (Documents) Migration
        QStringList allPaths;
        getAllJsonPaths(":/json/index.json", allPaths, "");
        
        for (const QString &pathStr : allPaths) {
            QStringList parts = pathStr.split("|");
            if(parts.size() > 0) {
                QString path = parts[0];
                QFile file(path);
                if (file.open(QIODevice::ReadOnly)) {
                    QString jsonContent = QString(file.readAll());
                    DatabaseManager::instance().saveDocument(path, jsonContent);
                    file.close();
                }
            }
        }
        
        qDebug() << "[Migration] Klasörler oluşturuldu. Arama tablosu (Search Items) oluşturuluyor...";
        
        // 2. Arama Motoru (Search Items) detaylı parçalama (recursive)
        recursiveDBSearchMigration(":/json/index.json", "");
        
        qDebug() << "[Migration] Tüm içerikler başarıyla SQLite'a eklendi!";
    } else {
        qDebug() << "[Migration] Veritabanı zaten dolu, migration atlandı.";
    }
}

void MainWindow::recursiveDBSearchMigration(const QString &searchPath, const QString &pathPrefix)
{
    // Bu sadece bir defa çalışır (QRC üzerinden okur, search tablosuna doldurur)
    QFile file(searchPath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull() || !doc.isObject()) return;
    QJsonObject jsonData = doc.object();
    
    for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
        QString key = it.key();
        QJsonObject obj = it.value().toObject();
        
        bool isFolder = obj.contains("file");
        QString description = obj.contains("desc") ? obj["desc"].toString() : "";
        
        // Aslında description json içindeki objectleri stringe çevirerek daha karmaşık aranabilir de yapılabilir.
        // Fakat şimdilik sadece desc kaydediyoruz. Full content aranmak istenirse "content" de eklenebilir.
        // Hızlı arama motorumuz O(1) gibi çalışacak.
        
        // Arama kaydını kaydet:
        DatabaseManager::instance().saveSearchItem(key, description, isFolder, searchPath, !isFolder);
        
        if (isFolder) {
            QString fileName = obj["file"].toString();
            QString nextPath = ":/json/" + fileName;
            QString nextPrefix = pathPrefix.isEmpty() ? key : pathPrefix + " > " + key;
            recursiveDBSearchMigration(nextPath, nextPrefix);
        }
    }
}

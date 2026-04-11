#include "mainwindow.h"


// Sistem Tepsisinde ki icon ve ozelliklerinin ayarlanmasi
void MainWindow::createTrayIcon() {
    restoreAction = new QAction(tr("Göster/Gizle"), this);
    quitAction = new QAction(tr("Çıkış"), this);

    trayMenu = new QMenu(this);
    trayMenu->addAction(restoreAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);

    // Platform-agnostic icon loading (.ico works on all platforms in Qt)
    QIcon appIcon(":/icon/icon48x48.ico");
    if (appIcon.isNull()) {
        qWarning() << "Failed to load application icon!";
    }
    trayIcon->setIcon(appIcon);
    trayIcon->setToolTip("Cyber Notes");

    // Tray icon mesajı göster
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon->show();
        trayIcon->showMessage("Cyn0",
                              "Program sistem tepsisinde çalışıyor.\nShift+Space ile açabilirsiniz.",
                              QSystemTrayIcon::Information, 3000);
    } else {
        QMessageBox::critical(this, "Sistem Tepsisi",
                              "Sistem tepsisi bu sistemde kullanılamıyor!",
                              QMessageBox::Ok);
        QApplication::quit();
    }
}


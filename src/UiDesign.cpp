#include "qscreen.h"
#include <mainwindow.h>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QPainter>
#include <QFontDatabase>  // Font yüklemek için


// Applicationun GUI konumunu ekranin merkezine yakin bir yere sabitler.
void MainWindow::fixUiCenter() {
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    QPoint centerPoint = screenGeometry.center();

    // Ekranın ortasının 2 birim üstüne ayarla
    centerPoint.setY(centerPoint.y() - (screenGeometry.height() / 4));
    move(centerPoint - rect().center());
}


// Shortcut basildiginda animansyonlu bir gecis yapar.
void MainWindow::toggleVisibility() {
    toggleVisibilityAnimation(this, isVisibleFlag);
    isVisibleFlag = !isVisibleFlag;
    qDebug() << "Shortcut triggered Current value: [ " << isVisibleFlag << " ]";
}


// Özel font yükleme fonksiyonu
void MainWindow::loadCustomFont() {
    int fontId = QFontDatabase::addApplicationFont(":/fonts/resources/Segoe Fluent Icons.ttf");
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QString fontFamily = fontFamilies.at(0);

            // Widget'lar için font ayarla
            QFont customFont(fontFamily, 10);
            QApplication::setFont(customFont);

            // Font family'yi class member olarak sakla (HTML için kullanmak üzere)
            m_customFontFamily = fontFamily;

            qDebug() << "[Font] Özel font yüklendi: " << fontFamily;
        } else {
            qDebug() << "[Font] Font aileleri bulunamadı!";
        }
    } else {
        qDebug() << "[Font] Font yüklenemedi!";
        // Fallback font
        m_customFontFamily = "Arial, sans-serif";
    }
}

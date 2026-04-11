#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>
#include <QMenu>

// Klavye kısayollarını ayarla
void MainWindow::setupKeyboardShortcuts()
{
    // ESC ile uygulama arayüzünü gizle
    QShortcut *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (isVisibleFlag && ui->lineEdit->text().isEmpty() && !m_isShowingContent) {
            toggleVisibility();
        }
    });

    qDebug() << "[Shortcuts] Klavye kısayolları ayarlandı: ESC";
}

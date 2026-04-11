#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QClipboard>
#include <QGuiApplication>
#include <QTimer>

// Payload'ları içerikten çıkar
void MainWindow::extractPayloads(const QString &content)
{
    currentPayloads.clear();
    currentPayloadIndex = -1;

    // Regex pattern: <code>, <pre>, ```, backtick veya özel karakterlerle başlayan satırlar
    QRegularExpression payloadPattern(
        R"((?:<code[^>]*>([^<]+)</code>|<pre[^>]*>([^<]+)</pre>|```([^`]+)```|`([^`]+)`|^[\s]*([<>$#'\"/\\=\{\}\[\]]+.*)$))",
        QRegularExpression::MultilineOption
    );

    QRegularExpressionMatchIterator it = payloadPattern.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        for (int i = 1; i <= match.lastCapturedIndex(); ++i) {
            QString payload = match.captured(i).trimmed();
            if (!payload.isEmpty() && payload.length() > 3) {
                currentPayloads.append(payload);
            }
        }
    }

    // Eğer hiç payload bulunamadıysa, satır satır kontrol et
    if (currentPayloads.isEmpty()) {
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            // Potansiyel payload özellikleri: < > $ # / \ ' " { } [ ] içerir
            if (trimmed.length() > 5 &&
                (trimmed.contains('<') || trimmed.contains('>') ||
                 trimmed.contains('$') || trimmed.contains('#') ||
                 trimmed.contains('/') || trimmed.contains('\\') ||
                 trimmed.contains('\'') || trimmed.contains('"') ||
                 trimmed.contains('{') || trimmed.contains('[') ||
                 trimmed.startsWith("http") || trimmed.startsWith("ftp"))) {
                currentPayloads.append(trimmed);
            }
        }
    }

    qDebug() << "[PayloadCopy]" << currentPayloads.size() << "payload bulundu";

    // İlk payload'ı otomatik seç
    if (!currentPayloads.isEmpty()) {
        currentPayloadIndex = 0;
        highlightPayload(0);
    }
}

// Payload'ı vurgula
void MainWindow::highlightPayload(int index)
{
    if (index < 0 || index >= currentPayloads.size()) return;

    QString payload = currentPayloads[index];
    QString htmlContent = ui->infoBrowser->toHtml();

    // Tüm önceki vurgulamaları temizle
    htmlContent.replace(QRegularExpression(R"(<span style='background-color: #7C3AED; color: #FFFFFF; padding: 2px 6px; border-radius: 4px;'>([^<]+)</span>)"),
                       R"(\1)");

    // Şu anki payload'ı HTML'de bul ve vurgula (sadece ilk eşleşmeyi)
    QString escapedPayload = payload.toHtmlEscaped();
    QString highlighted = QString("<span style='background-color: #7C3AED; color: #FFFFFF; padding: 2px 6px; border-radius: 4px;'>%1</span>")
                             .arg(escapedPayload);

    // İlk eşleşmeyi bul ve vurgula
    int pos = htmlContent.indexOf(escapedPayload);
    if (pos != -1) {
        htmlContent.replace(pos, escapedPayload.length(), highlighted);
    }

    ui->infoBrowser->setHtml(htmlContent);

    // Vurgulanan yere scroll et
    QTextCursor cursor = ui->infoBrowser->document()->find(payload);
    if (!cursor.isNull()) {
        ui->infoBrowser->setTextCursor(cursor);
        ui->infoBrowser->ensureCursorVisible();
    }

    qDebug() << "[PayloadCopy] Payload vurgulandı:" << index + 1 << "/" << currentPayloads.size() << "-" << payload.left(30);
}

// Seçili payload'ı kopyala
void MainWindow::copyCurrentPayload()
{
    if (currentPayloadIndex < 0 || currentPayloadIndex >= currentPayloads.size()) return;

    QString payload = currentPayloads[currentPayloadIndex];
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(payload);

    qDebug() << "[PayloadCopy] Kopyalandı:" << payload.left(50) + "...";

    // Kullanıcıya bildirim (QSS ile stillendirilmiş geçici mesaj)
    QString originalHtml = ui->infoBrowser->toHtml();
    QString notification = QString(
        "<div style='position: fixed; top: 10px; right: 10px; "
        "background: rgba(124, 58, 237, 0.95); color: white; "
        "padding: 12px 20px; border-radius: 8px; "
        "font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3);'>"
        "✅ Kopyalandı! (%1/%2)"
        "</div>").arg(currentPayloadIndex + 1).arg(currentPayloads.size());

    ui->infoBrowser->setHtml(notification + originalHtml);

    // 1 saniye sonra eski haline döndür
    QTimer::singleShot(1000, this, [this, originalHtml]() {
        ui->infoBrowser->setHtml(originalHtml);
        highlightPayload(currentPayloadIndex);
    });
}

// Sonraki payload'a geç
void MainWindow::selectNextPayload()
{
    if (currentPayloads.isEmpty()) return;

    currentPayloadIndex++;
    if (currentPayloadIndex >= currentPayloads.size()) {
        currentPayloadIndex = 0;  // En başa dön
    }

    highlightPayload(currentPayloadIndex);
    qDebug() << "[PayloadCopy] Sonraki payload:" << currentPayloadIndex + 1 << "/" << currentPayloads.size();
}

// Önceki payload'a geç
void MainWindow::selectPreviousPayload()
{
    if (currentPayloads.isEmpty()) return;

    currentPayloadIndex--;
    if (currentPayloadIndex < 0) {
        currentPayloadIndex = currentPayloads.size() - 1;  // En sona git
    }

    highlightPayload(currentPayloadIndex);
    qDebug() << "[PayloadCopy] Önceki payload:" << currentPayloadIndex + 1 << "/" << currentPayloads.size();
}

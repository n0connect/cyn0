#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QClipboard>
#include <QGuiApplication>
#include <QTimer>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>

// Payload'ları JSON veri tabanından yapısal olarak çıkar
void MainWindow::extractPayloads(const QJsonObject &cmdObj)
{
    currentPayloads.clear();
    currentPayloadIndex = -1;

    // Ekstra seçimleri temizle (önceden kalma vurgular silinsin)
    ui->infoBrowser->setExtraSelections(QList<QTextEdit::ExtraSelection>());

    // Kopyalanabilir komutların barınabileceği JSON anahtarları
    QStringList keysToExtract = {"usage", "use", "command", "examples", "example", "payload", "syntax"};

    for (const QString &key : keysToExtract) {
        if (!cmdObj.contains(key)) continue;

        QJsonValue value = cmdObj[key];
        
        if (value.isString()) {
            QString payload = value.toString().trimmed();
            if (!payload.isEmpty()) {
                currentPayloads.append(payload);
            }
        } 
        else if (value.isArray()) {
            QJsonArray array = value.toArray();
            for (const QJsonValue &item : array) {
                if (item.isString()) {
                    QString payload = item.toString().trimmed();
                    if (!payload.isEmpty()) {
                        currentPayloads.append(payload);
                    }
                } else if (item.isObject()) {
                    QJsonObject nestedObj = item.toObject();
                    if (nestedObj.contains("command") && nestedObj["command"].isString()) {
                        QString payload = nestedObj["command"].toString().trimmed();
                        if (!payload.isEmpty()) {
                            currentPayloads.append(payload);
                        }
                    }
                }
            }
        }
    }

    qDebug() << "[PayloadCopy]" << currentPayloads.size() << "yapısal payload bulundu";

    if (!currentPayloads.isEmpty()) {
        currentPayloadIndex = 0;
        highlightPayload(0);
    }
}

// Payload'ı ExtraSelection ile vurgula
void MainWindow::highlightPayload(int index)
{
    if (index < 0 || index >= currentPayloads.size()) return;

    QString payload = currentPayloads[index];

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextDocument *doc = ui->infoBrowser->document();
    QTextCursor searchCursor(doc);

    // Aynı payload'dan birden fazla varsa doğru olana atla
    int occurrence = 0;
    for (int i = 0; i <= index; ++i) {
        if (currentPayloads[i] == payload) {
            occurrence++;
        }
    }

    QTextCursor foundCursor;
    for (int i = 0; i < occurrence; ++i) {
        foundCursor = doc->find(payload, searchCursor);
        if (!foundCursor.isNull()) {
            searchCursor = foundCursor;
        } else {
            break;
        }
    }

    if (!foundCursor.isNull()) {
        // ExtraSelection ile HTML yapısını bozmadan vurgula
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor("#7C3AED")); // Vurgu rengi
        selection.format.setForeground(Qt::white);
        selection.cursor = foundCursor;
        extraSelections.append(selection);
        
        ui->infoBrowser->setExtraSelections(extraSelections);

        // Native block selection (mavi renkli OS seçimi) oluşmaması için, sinyalleri kapatıp scroll yapalım.
        QSignalBlocker blocker(ui->infoBrowser);
        
        // Scroll pozisyonunu ayarlamak için geçici cursor ataması
        ui->infoBrowser->setTextCursor(foundCursor);
        ui->infoBrowser->ensureCursorVisible();
        
        // Kullanıcının Shift tuşuna basılı tutması sebebiyle çoklu metin bloğu seçilmemesi için:
        QTextCursor clearCursor = foundCursor;
        clearCursor.clearSelection(); 
        ui->infoBrowser->setTextCursor(clearCursor);
    }

    qDebug() << "[PayloadCopy] Payload vurgulandı:" << index + 1 << "/" << currentPayloads.size();
}

// Seçili payload'ı kopyala
void MainWindow::copyCurrentPayload()
{
    if (currentPayloadIndex < 0 || currentPayloadIndex >= currentPayloads.size()) return;

    QString payload = currentPayloads[currentPayloadIndex];
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(payload);

    qDebug() << "[PayloadCopy] Kopyalandı:" << payload.left(50) + "...";

    // Bildirimi kalıcı kılmak için QToolTip yerine uçuşan (floating) bir QLabel kullanıyoruz.
    // Çünkü QToolTip, klavye tuşu (Shift, C vs.) bırakıldığı an sistem tarafından otomatik gizlenir.
    QLabel *notification = new QLabel(ui->infoBrowser);
    QString text = QString("✅ Kopyalandı! (%1/%2)").arg(currentPayloadIndex + 1).arg(currentPayloads.size());
    notification->setText(text);
    notification->setStyleSheet(
        "background: rgba(124, 58, 237, 0.95); "
        "color: white; "
        "padding: 12px 20px; "
        "border-radius: 8px; "
        "font-weight: bold;"
    );
    notification->adjustSize();
    // InfoBrowser'ın sağ üst köşesine konumlandır
    notification->move(ui->infoBrowser->width() - notification->width() - 20, 20);
    notification->show();

    // 1.5 saniye sonra ekrandan sil
    QTimer::singleShot(1500, notification, &QLabel::deleteLater);
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
}

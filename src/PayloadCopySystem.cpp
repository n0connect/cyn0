#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

namespace {
bool isPayloadLikeKey(const QString &key)
{
    const QString lowered = key.toLower();
    static const QSet<QString> payloadKeys = {
        "usage", "use", "command", "commands", "example", "examples",
        "payload", "payloads", "syntax", "poc"
    };
    return payloadKeys.contains(lowered) || lowered.contains("payload");
}

void appendPayloadCandidate(QStringList &out, const QString &raw)
{
    const QString payload = raw.trimmed();
    if (!payload.isEmpty()) {
        out.append(payload);
    }
}

void collectStringLeaves(const QJsonValue &value, QStringList &out)
{
    if (value.isString()) {
        appendPayloadCandidate(out, value.toString());
        return;
    }

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            collectStringLeaves(item, out);
        }
        return;
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            collectStringLeaves(it.value(), out);
        }
    }
}

void collectPayloadCandidates(const QJsonValue &value, const QString &keyHint, QStringList &out)
{
    const QString loweredKey = keyHint.toLower();
    const bool payloadContext = isPayloadLikeKey(loweredKey);

    if (value.isString()) {
        if (payloadContext) {
            appendPayloadCandidate(out, value.toString());
        }
        return;
    }

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            // Arrays under payload-bearing keys are usually direct payload rows.
            if (payloadContext && item.isString()) {
                appendPayloadCandidate(out, item.toString());
                continue;
            }
            collectPayloadCandidates(item, keyHint, out);
        }
        return;
    }

    if (value.isObject()) {
        // payloads gibi container alanlarda nested tüm string'ler payload kabul edilir.
        if (loweredKey.contains("payload")) {
            collectStringLeaves(value, out);
            return;
        }

        const QJsonObject object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            const QString childKey = it.key();
            collectPayloadCandidates(it.value(), childKey, out);
        }
    }
}

QTextCursor cursorFromRange(QTextDocument *doc, int start, int length)
{
    QTextCursor cursor(doc);
    cursor.setPosition(start);
    cursor.setPosition(start + length, QTextCursor::KeepAnchor);
    return cursor;
}

QStringList orderedKeysForDisplay(const QJsonObject &object)
{
    QStringList keys = object.keys();
    const QStringList priorityOrder = {
        "desc", "description", "usage", "use", "command", "params", "parameters",
        "flags", "enumeration_options", "examples", "example", "note", "notes",
        "warning", "info"
    };

    QStringList ordered;
    for (const QString &priority : priorityOrder) {
        if (keys.contains(priority)) {
            ordered.append(priority);
            keys.removeOne(priority);
        }
    }

    ordered.append(keys);
    return ordered;
}
} // namespace

// Payload'ları JSON veri tabanından yapısal olarak çıkar
void MainWindow::extractPayloads(const QJsonObject &cmdObj)
{
    currentPayloads.clear();
    currentPayloadCursors.clear();
    currentPayloadIndex = -1;
    ui->infoBrowser->setExtraSelections({});

    QStringList candidates;
    const QStringList topLevelKeys = orderedKeysForDisplay(cmdObj);
    for (const QString &key : topLevelKeys) {
        collectPayloadCandidates(cmdObj.value(key), key, candidates);
    }

    qDebug() << "[PayloadCopy] Aday payload sayısı:" << candidates.size();

    QTextDocument *doc = ui->infoBrowser->document();
    QString plainText = doc->toPlainText();
    int searchPos = 0;

    for (const QString &candidate : candidates) {
        QTextCursor from(doc);
        if (searchPos > 0) {
            from.setPosition(searchPos);
        }

        QTextCursor match = doc->find(candidate, from, QTextDocument::FindCaseSensitively);

        // Satır sonu/boşluk normalize edilmiş içerikler için fallback.
        if (match.isNull()) {
            QString fallback = candidate;
            fallback.replace(QRegularExpression("\\s+"), " ");
            fallback = fallback.trimmed();

            if (!fallback.isEmpty()) {
                int idx = plainText.indexOf(fallback, searchPos, Qt::CaseSensitive);
                if (idx >= 0) {
                    match = cursorFromRange(doc, idx, fallback.length());
                }
            }
        }

        // Görünen içerikte bulunamayan adayları dışla; UX için görünen = seçilebilir.
        if (match.isNull()) {
            continue;
        }

        currentPayloads.append(candidate);
        currentPayloadCursors.append(match);
        searchPos = match.selectionEnd();
    }

    qDebug() << "[PayloadCopy] Görünür payload sayısı:" << currentPayloads.size();

    if (!currentPayloads.isEmpty()) {
        currentPayloadIndex = 0;
        highlightPayload(0);
    }
}

// Payload'ı ExtraSelection ile vurgula
void MainWindow::highlightPayload(int index)
{
    if (index < 0 || index >= currentPayloads.size() || index >= currentPayloadCursors.size()) {
        return;
    }

    const QTextCursor target = currentPayloadCursors[index];
    if (target.isNull()) {
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor("#7C3AED"));
    selection.format.setForeground(Qt::white);
    selection.cursor = target;
    ui->infoBrowser->setExtraSelections({selection});

    // Native text selection (Shift ile oluşan mavi seçim) yerine sadece caret taşı.
    QSignalBlocker blocker(ui->infoBrowser);
    QTextCursor caret = target;
    caret.setPosition(target.selectionEnd());
    ui->infoBrowser->setTextCursor(caret);
    ui->infoBrowser->ensureCursorVisible();

    qDebug() << "[PayloadCopy] Payload vurgulandı:" << index + 1 << "/" << currentPayloads.size();
}

// Seçili payload'ı kopyala
void MainWindow::copyCurrentPayload()
{
    if (currentPayloadIndex < 0 || currentPayloadIndex >= currentPayloads.size()) {
        return;
    }

    const QString payload = currentPayloads[currentPayloadIndex];
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(payload);

    qDebug() << "[PayloadCopy] Kopyalandı:" << payload.left(50) + "...";

    QLabel *notification = new QLabel(ui->infoBrowser);
    notification->setText(QString("Kopyalandi (%1/%2)")
                              .arg(currentPayloadIndex + 1)
                              .arg(currentPayloads.size()));
    notification->setStyleSheet(
        "background: rgba(124, 58, 237, 0.95); "
        "color: white; "
        "padding: 12px 20px; "
        "border-radius: 8px; "
        "font-weight: bold;");
    notification->adjustSize();
    notification->move(ui->infoBrowser->width() - notification->width() - 20, 20);
    notification->show();

    QTimer::singleShot(1500, notification, &QLabel::deleteLater);
}

// Sonraki payload'a geç
void MainWindow::selectNextPayload()
{
    if (currentPayloads.isEmpty()) return;

    currentPayloadIndex++;
    if (currentPayloadIndex >= currentPayloads.size()) {
        currentPayloadIndex = 0;
    }

    highlightPayload(currentPayloadIndex);
}

// Önceki payload'a geç
void MainWindow::selectPreviousPayload()
{
    if (currentPayloads.isEmpty()) return;

    currentPayloadIndex--;
    if (currentPayloadIndex < 0) {
        currentPayloadIndex = currentPayloads.size() - 1;
    }

    highlightPayload(currentPayloadIndex);
}

// -- JSON PARSER

#include <mainwindow.h>
#include <JsonParser.h>

// loadCommandData fonksiyonunu debug çıktıları ile güçlendir
void MainWindow::loadCommandData(const QString &jsonPath) {
    qDebug() << "[Debug] JSON yükleme başlatılıyor: " << jsonPath;

    QByteArray jsonData;
    
    // ÖNCE SQLITE VERİTABANINA BAK (AKILLI HİBRİT MİMARİ)
    QString dbContent = DatabaseManager::instance().getDocument(jsonPath);
    
    if (!dbContent.isEmpty()) {
        qDebug() << "[Debug] JSON SQLite deposundan başarıyla çekildi.";
        jsonData = dbContent.toUtf8();
    } else {
        qDebug() << "[Debug] SQLite'da bulunamadı, fallback (QRC/Yerel) deneniyor...";
        QFile file(jsonPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "[Debug] Dosya açılamadı: " << jsonPath;
            qDebug() << "[Debug] Dosya hatası: " << file.errorString();
            QMessageBox::critical(this, "JSON Hatası", "Dosya açılamadı: " + jsonPath + "\nHata: " + file.errorString(), QMessageBox::Ok);
            return;
        }

        jsonData = file.readAll();
        file.close();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "[Debug] JSON parse hatası: " << parseError.errorString();
        QMessageBox::critical(this, "JSON Hatası", "JSON parse hatası: " + parseError.errorString(), QMessageBox::Ok);
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "[Debug] JSON object değil";
        QMessageBox::critical(this, "JSON Hatası", "JSON formatı geçerli değil (nesne bekleniyordu).", QMessageBox::Ok);
        return;
    }

    commandData = doc.object();
    qDebug() << "[Debug] JSON başarıyla yüklendi: " << jsonPath;
    qDebug() << "[Debug] Komut sayısı: " << commandData.size();
    qDebug() << "[Debug] Anahtarlar: " << commandData.keys();
}


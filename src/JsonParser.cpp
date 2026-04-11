// -- JSON PARSER

#include <mainwindow.h>
#include <JsonParser.h>

// loadCommandData fonksiyonunu debug çıktıları ile güçlendir
void MainWindow::loadCommandData(const QString &jsonPath) {
    qDebug() << "[Debug] JSON yükleme başlatılıyor: " << jsonPath;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Debug] Dosya açılamadı: " << jsonPath;
        qDebug() << "[Debug] Dosya hatası: " << file.errorString();
        QMessageBox::critical(this, "JSON Hatası", "Dosya açılamadı: " + jsonPath + "\nHata: " + file.errorString(), QMessageBox::Ok);
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    qDebug() << "[Debug] Dosya okundu. Boyut: " << jsonData.size() << " bytes";
    qDebug() << "[Debug] İlk 100 karakter: " << jsonData.left(100);

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


#include <mainwindow.h>

QJsonObject MainWindow::loadJsonFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[Debug] JSON dosyası açılamadı: " << filePath;
        return QJsonObject();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "[Debug] Geçersiz JSON: " << filePath;
        return QJsonObject();
    }

    return doc.object();
}

// Tüm JSON dosya yollarını toplama fonksiyonu
void MainWindow::getAllJsonPaths(const QString &startPath, QStringList &jsonPaths, const QString &pathPrefix) {
    QJsonObject jsonData = loadJsonFile(startPath);
    if (jsonData.isEmpty()) return;

    // Mevcut dosyayı ekle
    jsonPaths.append(startPath + "|" + pathPrefix);

    // Alt dosyaları tara
    for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
        QJsonObject obj = it.value().toObject();
        if (obj.contains("file") && obj["file"].isString()) {
            QString fileName = obj["file"].toString();
            QString nextPath = ":/json/" + fileName;
            QString nextPrefix = pathPrefix.isEmpty() ? it.key() : pathPrefix + " > " + it.key();

            // Recursive çağrı
            getAllJsonPaths(nextPath, jsonPaths, nextPrefix);
        }
    }
}

// JSON değerini recursive olarak arar (tüm string değerler içinde)
bool searchInJsonValue(const QJsonValue &value, const QString &keyword) {
    if (value.isString()) {
        return value.toString().contains(keyword, Qt::CaseInsensitive);
    }
    else if (value.isArray()) {
        QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            if (searchInJsonValue(item, keyword)) {
                return true;
            }
        }
    }
    else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (searchInJsonValue(it.value(), keyword)) {
                return true;
            }
        }
    }
    return false;
}

// Tüm JSON dosyalarında recursive arama fonksiyonu
void MainWindow::recursiveSearchInAllJsons(const QString &keyword, QStringList &matches, const QString &currentJsonPath, const QString &pathPrefix) {
    QString searchPath = currentJsonPath.isEmpty() ? ":/json/index.json" : currentJsonPath;
    QJsonObject jsonData = loadJsonFile(searchPath);

    if (jsonData.isEmpty()) return;

    // Mevcut JSON'daki anahtarları kontrol et
    for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
        QString key = it.key();
        QJsonObject obj = it.value().toObject();

        bool keyMatch = false;
        bool contentMatch = false;

        // 1. Anahtar adında arama yap (öncelikli)
        if (key.contains(keyword, Qt::CaseInsensitive)) {
            keyMatch = true;
        }

        // 2. İçerikte arama yap (sadece klasör değilse)
        if (!keyMatch && !obj.contains("file")) {
            // Bu bir komut/içerik öğesi, tüm içeriğini ara
            contentMatch = searchInJsonValue(obj, keyword);
        }

        // Eşleşme varsa sonuçlara ekle
        if (keyMatch || contentMatch) {
            QString fullPath = pathPrefix.isEmpty() ? key : pathPrefix + " > " + key;
            QString matchType = keyMatch ? "başlık" : "içerik";

            if (obj.contains("file")) {
                // Bu bir klasör (sadece başlık eşleşmesi olabilir)
                matches.append("📁|" + fullPath + "|" + searchPath + "|" + key);
            } else {
                // Bu bir komut/içerik
                if (contentMatch) {
                    // İçerik eşleşmesi varsa (içerik) etiketi ekle
                    matches.append("📄|" + fullPath + " (içerik eşleşmesi)|" + searchPath + "|" + key);
                } else {
                    matches.append("📄|" + fullPath + "|" + searchPath + "|" + key);
                }
            }
        }

        // Eğer bu bir alt klasörse, recursive olarak ara
        if (obj.contains("file") && obj["file"].isString()) {
            QString fileName = obj["file"].toString();
            QString nextPath = ":/json/" + fileName;
            QString nextPrefix = pathPrefix.isEmpty() ? key : pathPrefix + " > " + key;

            recursiveSearchInAllJsons(keyword, matches, nextPath, nextPrefix);
        }
    }

    // PERSONAL NOTES ARAMASINI EKLE (sadece ilk çağrıda, yani index.json'da)
    if (currentJsonPath.isEmpty() || currentJsonPath == ":/json/index.json") {
        QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
        QJsonArray notesArray = personalNotes["notes"].toArray();

        for (int i = 0; i < notesArray.size(); ++i) {
            QJsonObject note = notesArray[i].toObject();
            QString noteTitle = note["title"].toString();
            QString noteContent = note["content"].toString();
            QString noteTags = note["tags"].toString();

            bool titleMatch = noteTitle.contains(keyword, Qt::CaseInsensitive);
            bool contentMatch = noteContent.contains(keyword, Qt::CaseInsensitive);
            bool tagsMatch = noteTags.contains(keyword, Qt::CaseInsensitive);

            if (titleMatch || contentMatch || tagsMatch) {
                QString matchInfo;
                if (titleMatch) {
                    matchInfo = "Personal Notlar > " + noteTitle;
                } else if (tagsMatch) {
                    matchInfo = "Personal Notlar > " + noteTitle + " (etiket eşleşmesi)";
                } else {
                    matchInfo = "Personal Notlar > " + noteTitle + " (içerik eşleşmesi)";
                }

                // Personal note için özel format
                matches.append("📝|" + matchInfo + "|personal_notes|" + noteTitle);

                qDebug() << "[Search] Personal note bulundu:" << noteTitle;
            }
        }
    }
}

// JsonDbParser.cpp

#include "ui_mainwindow.h"
#include <mainwindow.h>


// JSON iceriklerinden ana dizinde bulunan index.json dosyasini okur
void MainWindow::showMainMenu() {
    qDebug() << "[Debug] showMainMenu çağrıldı";

    // İçerikten geri dönülüyorsa (m_isShowingContent true ise), seçim hafızasını pop et
    if (m_isShowingContent && !selectedIndexHistory.isEmpty()) {
        qDebug() << "[Debug] İçerikten geri dönülüyor, seçim hafızası pop ediliyor";
        // Pop işlemini daha sonra yapacağız (seçimi geri yükledikten sonra)
    }

    // İçerik gösterimi bittiğinde flag'i sıfırla
    m_isShowingContent = false;
    qDebug() << "[Debug] m_isShowingContent = false (showMainMenu)";

    ui->listWidget->clear();
    ui->infoBrowser->hide();

    // Focus'u lineEdit'e geri ver
    ui->lineEdit->setFocus();
    qDebug() << "[Focus] showMainMenu - lineEdit'e focus verildi";

    if (commandData.isEmpty()) {
        qDebug() << "[Debug] commandData boş!";
        return;
    }

    QStringList keys = commandData.keys();
    qDebug() << "[Debug] Gösterilecek anahtarlar: " << keys;

    for (const QString &key : keys) {
        QListWidgetItem* item = new QListWidgetItem();

        QJsonObject obj = commandData[key].toObject();

        if (obj.contains("file")) {
            item->setText("📁 " + key);
            item->setToolTip("Alt kategori: " + obj["desc"].toString());
            qDebug() << "[Debug] Navigasyon öğesi eklendi: " << key;
        } else {
            item->setText("📄 " + key);
            item->setToolTip("Komut/İçerik");
            qDebug() << "[Debug] İçerik öğesi eklendi: " << key;
        }

        item->setData(Qt::UserRole, key);
        ui->listWidget->addItem(item);
    }

    if (ui->listWidget->count() > 0) {
        // Seçim hafızasından geri yükle
        if (!selectedIndexHistory.isEmpty()) {
            int savedIndex = selectedIndexHistory.top();
            if (savedIndex >= 0 && savedIndex < ui->listWidget->count()) {
                ui->listWidget->setCurrentRow(savedIndex);
                qDebug() << "[Debug] Seçim hafızadan geri yüklendi: " << savedIndex;
            } else {
                ui->listWidget->setCurrentRow(0);
            }
            // İçerikten geri dönülüyorsa hafızadan pop et
            // (Klasör değişikliğinde navigateBack zaten pop ediyor)
            // Eğer son eklenen index, mevcut pathHistory ile eşitse içerikten dönüyoruz demektir
            if (selectedIndexHistory.size() > pathHistory.size()) {
                selectedIndexHistory.pop();
                qDebug() << "[Debug] İçerikten dönüldüğü için seçim hafızası pop edildi";
            }
        } else {
            ui->listWidget->setCurrentRow(0);
        }
        ui->listWidget->show();
        qDebug() << "[Debug] Liste gösterildi. Toplam öğe: " << ui->listWidget->count();
    } else {
        qDebug() << "[Debug] Liste boş!";
    }
}


// showCommandInfo fonksiyonunu güncelle - nested objects için destek eklendi
void MainWindow::showCommandInfo(const QString &keyword)
{
    if (!commandData.contains(keyword)) return;

    QJsonObject cmdObj = commandData[keyword].toObject();



    // ========== Navigasyon Öğeleri (Klasörler) ==========
    if (cmdObj.contains("file")) {
        qDebug() << "[Debug] Bu bir navigasyon öğesi, ama içerik gösterilmeye çalışılıyor: " << keyword;

        QString html = "<div style='";
        html += "font-family: \"" + m_customFontFamily + "\", Arial, sans-serif; ";
        html += "font-size: 12px; ";
        html += "color: #333; ";
        html += "padding: 20px; ";
        html += "background-color: #f5f5f5; ";
        html += "border-radius: 5px; ";
        html += "text-align: center;";
        html += "'>";

        html += "<div style='font-size: 16px; margin-bottom: 10px;'>📁</div>";
        html += "<div style='font-weight: bold; margin-bottom: 10px; color: #555;'>";
        html += keyword.toHtmlEscaped();
        html += "</div>";

        if (cmdObj.contains("desc")) {
            html += "<div style='color: #777; font-style: italic;'>";
            html += cmdObj["desc"].toString().toHtmlEscaped();
            html += "</div>";
        }

        html += "<div style='margin-top: 15px; color: #999; font-size: 11px;'>";
        html += "Bu öğeye tıklayarak alt kategoriye geçebilirsiniz";
        html += "</div>";

        html += "</div>";

        m_isShowingContent = true;
        ui->infoBrowser->setHtml(html);
        ui->infoBrowser->show();
        ui->listWidget->hide();

        // Focus'u infoBrowser'a ver
        ui->infoBrowser->setFocus();
        qDebug() << "[Focus] Klasör/Navigasyon - infoBrowser'a focus verildi";

        return;
    }

    // ========== Normal İçerik Gösterimi ==========
    QString html = "<div style='";
    html += "font-family: \"" + m_customFontFamily + "\", Arial, sans-serif; ";
    html += "font-size: 11px; ";
    html += "color: #333; ";
    html += "padding: 10px; ";
    html += "background-color: #f5f5f5; ";
    html += "border-radius: 5px;";
    html += "'>";

    html += "<div style='";
    html += "font-family: \"" + m_customFontFamily + "\", Arial, sans-serif; ";
    html += "font-size: 14px; ";
    html += "font-weight: bold; ";
    html += "margin-bottom: 5px;";
    html += "'>";
    html += "Content Showing: " + keyword.toHtmlEscaped();
    html += "</div>";
    html += "</div>";

    QStringList keys = cmdObj.keys();
    QStringList priorityOrder = {"desc", "description", "usage", "use", "command", "params", "parameters",
                                 "flags", "enumeration_options", "examples", "example", "note", "notes",
                                 "warning", "info"};

    QStringList orderedKeys;
    for (const QString &priority : priorityOrder) {
        if (keys.contains(priority)) {
            orderedKeys.append(priority);
            keys.removeOne(priority);
        }
    }
    orderedKeys.append(keys);

    for (const QString &key : orderedKeys) {
        QJsonValue value = cmdObj[key];
        QString displayName = getDisplayName(key);

        if (value.isString()) {
            QString content = value.toString().toHtmlEscaped(); // Burada toHtmlEscaped() kullanıldı
            if (!content.isEmpty()) {
                html += "<div style='margin: 10px 0;'>";
                html += "<b style='color: #f4ed94;'>" + displayName + ":</b> ";
                html += "<span style='color: #f0f0f0;'>" + content + "</span>";
                html += "</div>";
            }
        }
        else if (value.isArray()) {
            QJsonArray array = value.toArray();
            if (!array.isEmpty()) {
                html += "<div style='margin: 10px 0;'>";
                html += "<b style='color: #f4ed94;'>" + displayName + ":</b>";
                html += "<ul style='margin: 5px 0 0 20px; color: #a899fe;'>";
                for (const QJsonValue &item : array) {
                    if (item.isString()) {
                        html += "<li style='margin: 2px 0;'>" + item.toString().toHtmlEscaped() + "</li>";
                    } else if (item.isObject()) {
                        // Array içindeki object'leri işle (examples gibi)
                        QJsonObject obj = item.toObject();
                        html += "<li style='margin: 5px 0;'>";
                        if (obj.contains("desc") && obj.contains("command")) {
                            html += "<b style='color: #f4ed94;'>" + obj["desc"].toString().toHtmlEscaped() + ":</b><br>";
                            html += "<code style='background-color: #2d2d2d; color: #f0f0f0; padding: 2px 4px; border-radius: 3px;'>";
                            html += obj["command"].toString().toHtmlEscaped();
                            html += "</code>";
                        } else {
                            // Diğer object formatları için genel işlem
                            QStringList objKeys = obj.keys();
                            for (const QString &objKey : objKeys) {
                                html += "<b style='color: #f4ed94;'>" + objKey.toHtmlEscaped() + ":</b> ";
                                html += obj[objKey].toString().toHtmlEscaped() + "<br>";
                            }
                        }
                        html += "</li>";
                    }
                }
                html += "</ul>";
                html += "</div>";
            }
        }
        else if (value.isObject()) {
            // Nested objects için yeni işlem (flags, enumeration_options vb.)
            QJsonObject nestedObj = value.toObject();
            if (!nestedObj.isEmpty()) {
                html += "<div style='margin: 10px 0;'>";
                html += "<b style='color: #f4ed94;'>" + displayName + ":</b>";
                html += "<div style='margin: 5px 0 0 20px;'>";

                QStringList nestedKeys = nestedObj.keys();
                for (const QString &nestedKey : nestedKeys) {
                    QJsonValue nestedValue = nestedObj[nestedKey];
                    if (nestedValue.isString()) {
                        html += "<div style='margin: 3px 0; color: #a899fe;'>";
                        html += "<b style='color: #f4ed94;'>" + nestedKey.toHtmlEscaped() + ":</b> ";
                        html += "<span style='color: #f0f0f0;'>" + nestedValue.toString().toHtmlEscaped() + "</span>";
                        html += "</div>";
                    } else if (nestedValue.isObject()) {
                        // Daha derin nested objects (mysql_important_tables gibi)
                        QJsonObject deepObj = nestedValue.toObject();
                        html += "<div style='margin: 5px 0; color: #a899fe;'>";
                        html += "<b style='color: #f4ed94;'>" + nestedKey.toHtmlEscaped() + ":</b>";
                        html += "<div style='margin: 3px 0 0 15px;'>";

                        QStringList deepKeys = deepObj.keys();
                        for (const QString &deepKey : deepKeys) {
                            html += "<div style='margin: 2px 0;'>";
                            html += "<b style='color: #f4ed94;'>" + deepKey.toHtmlEscaped() + ":</b> ";
                            html += "<span style='color: #f0f0f0;'>" + deepObj[deepKey].toString().toHtmlEscaped() + "</span>";
                            html += "</div>";
                        }
                        html += "</div>";
                        html += "</div>";
                    }
                }
                html += "</div>";
                html += "</div>";
            }
        }
    }

    html += "</div>";

    // İÇERİK GÖSTERİLDİĞİNİ İŞARETLE
    m_isShowingContent = true;
    qDebug() << "[Debug] İçerik gösteriliyor, m_isShowingContent = true";

    ui->infoBrowser->setHtml(html);
    ui->infoBrowser->show();
    ui->listWidget->hide();

    // Payload'ları çıkar ve otomatik seç
    QString plainText = ui->infoBrowser->toPlainText();
    extractPayloads(plainText);

    // Focus'u infoBrowser'a ver (scroll için)
    ui->infoBrowser->setFocus();
    qDebug() << "[Focus] Normal içerik - infoBrowser'a focus verildi";

    return; // Bu durumda return yap
}



// Anahtar adlarını güzelleştiren yardımcı fonksiyon
QString MainWindow::getDisplayName(const QString &key) {
    static QMap<QString, QString> displayNames = {
        {"desc", "Açıklama"},
        {"description", "Açıklama"},
        {"usage", "Kullanım"},
        {"use", "Kullanım"},
        {"command", "Komut"},
        {"params", "Parametreler"},
        {"parameters", "Parametreler"},
        {"examples", "Örnekler"},
        {"example", "Örnek"},
        {"note", "Not"},
        {"notes", "Notlar"},
        {"warning", "Uyarı"},
        {"info", "Bilgi"},
        {"syntax", "Sözdizimi"},
        {"options", "Seçenekler"},
        {"flags", "Bayraklar"},
        {"args", "Argümanlar"},
        {"arguments", "Argümanlar"},
        {"output", "Çıktı"},
        {"result", "Sonuç"},
        {"return", "Dönüş"},
        {"type", "Tip"},
        {"format", "Format"},
        {"version", "Sürüm"},
        {"author", "Yazar"},
        {"category", "Kategori"},
        {"tags", "Etiketler"},
        {"platform", "Platform"},
        {"os", "İşletim Sistemi"},
        {"required", "Gerekli"},
        {"optional", "İsteğe Bağlı"},
        {"default", "Varsayılan"},
        {"min", "Minimum"},
        {"max", "Maksimum"},
        {"path", "Yol"},
        {"url", "URL"},
        {"link", "Bağlantı"},
        {"source", "Kaynak"},
        {"target", "Hedef"},
        {"input", "Girdi"},
        {"config", "Yapılandırma"},
        {"settings", "Ayarlar"},
        {"permission", "İzin"},
        {"permissions", "İzinler"},
        {"security", "Güvenlik"},
        {"risk", "Risk"},
        {"level", "Seviye"},
        {"priority", "Öncelik"},
        {"status", "Durum"},
        {"state", "Hal"},
        {"mode", "Mod"},
        {"method", "Yöntem"},
        {"technique", "Teknik"},
        {"tool", "Araç"},
        {"tools", "Araçlar"},
        {"exploit", "İstismar"},
        {"payload", "Yük"},
        {"target_os", "Hedef İS"},
        {"cve", "CVE"},
        {"severity", "Önem Derecesi"},
        {"mitigation", "Önlem"},
        {"detection", "Tespit"},
        {"prevention", "Önleme"},
        {"enumeration_options", "Numaralandırma Seçenekleri"},
        {"common_vulnerabilities", "Yaygın Güvenlik Açıkları"},
        {"security_risk", "Güvenlik Riski"},
        {"required_permissions", "Gerekli İzinler"}
    };

    if (displayNames.contains(key.toLower())) {
        return displayNames[key.toLower()];
    }

    // Eğer özel tanım yoksa, ilk harfi büyük yapıp döndür
    QString result = key;
    if (!result.isEmpty()) {
        result[0] = result[0].toUpper();

        // Alt çizgileri boşluğa çevir
        result.replace('_', ' ');

        // Camel Case'i ayır
        for (int i = 1; i < result.length(); ++i) {
            if (result[i].isUpper() && result[i-1].isLower()) {
                result.insert(i, ' ');
                i++; // Eklenen boşluk için indeksi artır
            }
        }
    }

    return result;
}

// JsonActions.cpp - Düzeltilmiş Versiyon

#include "qevent.h"
#include "ui_mainwindow.h"
#include <mainwindow.h>
#include <QTimer>

// Arama fonksiyonunu güncelle - navigasyon öğelerini farklı göster
void MainWindow::on_lineEdit_textChanged(const QString &text)
{
    QString keyword = text.trimmed().toLower();
    ui->listWidget->clear();

    if (keyword.isEmpty()) {
        showMainMenu();
        return;
    }

    // Mevcut JSON'da başlık araması (öncelik için)
    const QStringList& keys = commandData.keys();
    QStringList currentMatches;

    for (const QString &key : keys) {
        QJsonObject obj = commandData[key].toObject();
        bool isNavigationItem = obj.contains("file");

        if (key.toLower() == keyword) {
            if (isNavigationItem) {
                // Klasörler için farklı işaretleme - showDirectContent = false
                currentMatches.prepend("📁|" + key + "|current|" + key + "|false");
            } else {
                // İçerik öğeleri için showDirectContent = true
                currentMatches.prepend("📄|" + key + "|current|" + key + "|true");
            }
        } else if (key.contains(keyword, Qt::CaseInsensitive)) {
            if (isNavigationItem) {
                // Klasörler için farklı işaretleme - showDirectContent = false
                currentMatches.append("📁|" + key + "|current|" + key + "|false");
            } else {
                // İçerik öğeleri için showDirectContent = true
                currentMatches.append("📄|" + key + "|current|" + key + "|true");
            }
        }
    }

    // Mevcut JSON'da açıklama ve alt yapı araması
    QStringList descriptionMatches;
    for (const QString &key : keys) {
        // Başlıkta zaten eşleşen öğeleri tekrar ekleme
        bool alreadyMatched = false;
        for (const QString &match : currentMatches) {
            QStringList parts = match.split("|");
            if (parts.size() >= 4 && parts[3] == key) {
                alreadyMatched = true;
                break;
            }
        }

        if (!alreadyMatched) {
            QJsonObject obj = commandData[key].toObject();
            bool isNavigationItem = obj.contains("file");

            // Açıklama kısmında arama
            if (obj.contains("desc") && obj["desc"].toString().contains(keyword, Qt::CaseInsensitive)) {
                if (isNavigationItem) {
                    descriptionMatches.append("📁|" + key + " (açıklama eşleşmesi)|current|" + key + "|false");
                } else {
                    descriptionMatches.append("📄|" + key + " (açıklama eşleşmesi)|current|" + key + "|true");
                }
            }

            // Sadece içerik öğeleri için alt yapılarda arama (klasörler için değil)
            if (!isNavigationItem) {
                // Alt yapılarda arama (commands, examples, vb.)
                if (obj.contains("commands")) {
                    QJsonArray commands = obj["commands"].toArray();
                    for (const QJsonValue &cmdValue : commands) {
                        QJsonObject cmdObj = cmdValue.toObject();
                        if (cmdObj.contains("command") &&
                            cmdObj["command"].toString().contains(keyword, Qt::CaseInsensitive)) {
                            descriptionMatches.append("📄|" + key + " (komut eşleşmesi)|current|" + key + "|true");
                            break;
                        }
                        if (cmdObj.contains("desc") &&
                            cmdObj["desc"].toString().contains(keyword, Qt::CaseInsensitive)) {
                            descriptionMatches.append("📄|" + key + " (komut açıklama eşleşmesi)|current|" + key + "|true");
                            break;
                        }
                    }
                }

                // Examples kısmında arama
                if (obj.contains("examples")) {
                    QJsonArray examples = obj["examples"].toArray();
                    for (const QJsonValue &exValue : examples) {
                        QJsonObject exObj = exValue.toObject();
                        if (exObj.contains("command") &&
                            exObj["command"].toString().contains(keyword, Qt::CaseInsensitive)) {
                            descriptionMatches.append("📄|" + key + " (örnek eşleşmesi)|current|" + key + "|true");
                            break;
                        }
                    }
                }
            }
        }
    }

    // Tüm JSON dosyalarında arama yap
    QStringList allMatches;
    recursiveSearchInAllJsons(keyword, allMatches);

    // Mevcut JSON'dan gelen sonuçları filtrele (tekrar etmesin)
    QStringList filteredAllMatches;
    for (const QString &match : allMatches) {
        QStringList parts = match.split("|");
        if (parts.size() >= 4) {
            QString jsonPath = parts[2];
            // Eğer mevcut JSON değilse ekle
            if (jsonPath != currentPath) {
                // Diğer JSON'lardan gelen sonuçlar için büyüteç emojisi ve showDirectContent = true
                QStringList newParts = parts;
                newParts[0] = "🔍";
                if (newParts.size() == 4) {
                    newParts.append("true"); // showDirectContent flag'i ekle
                }
                filteredAllMatches.append(newParts.join("|"));
            }
        }
    }

    // Sonuçları birleştir (önce mevcut JSON başlık eşleşmeleri, sonra açıklama eşleşmeleri, en son diğer JSON'lar)
    QStringList finalMatches = currentMatches + descriptionMatches + filteredAllMatches;

    // Exact match'leri en üste al
    QStringList exactMatches, otherMatches;
    for (const QString &match : finalMatches) {
        QStringList parts = match.split("|");
        if (parts.size() >= 4) {
            QString originalKey = parts[3];
            if (originalKey.toLower() == keyword) {
                exactMatches.append(match);
            } else {
                otherMatches.append(match);
            }
        }
    }

    finalMatches = exactMatches + otherMatches;

    // ListWidget'a sonuçları ekle
    for (const QString &match : finalMatches) {
        QStringList parts = match.split("|");
        if (parts.size() >= 5) { // Artık 5 parça var (showDirectContent dahil)
            QString icon = parts[0];
            QString displayText = parts[1];
            QString jsonPath = parts[2];
            QString originalKey = parts[3];
            bool showDirectContent = (parts[4] == "true");

            QListWidgetItem* item = new QListWidgetItem();

            // Mevcut JSON'dan geliyorsa normal göster
            if (jsonPath == "current" || jsonPath == currentPath) {
                item->setText(icon + " " + displayText);
                QJsonObject obj = commandData[originalKey].toObject();
                if (obj.contains("file")) {
                    item->setToolTip("Alt kategori: " + obj["desc"].toString());
                } else {
                    item->setToolTip("Komut/İçerik: " + obj["desc"].toString());
                }
                item->setData(Qt::UserRole, originalKey);
                item->setData(Qt::UserRole + 1, "current");
                item->setData(Qt::UserRole + 2, showDirectContent); // Doğru flag değeri
            } else {
                // Başka JSON'dan geliyorsa yolu göster
                item->setText(icon + " " + displayText);
                item->setToolTip("Yol: " + displayText);
                item->setData(Qt::UserRole, originalKey);
                item->setData(Qt::UserRole + 1, jsonPath);
                item->setData(Qt::UserRole + 2, showDirectContent); // Doğru flag değeri
            }

            ui->listWidget->addItem(item);
        }
    }

    if (ui->listWidget->count() > 0) {
        ui->listWidget->setCurrentRow(0);
        ui->listWidget->show();
        ui->infoBrowser->hide();
    } else {
        QListWidgetItem* item = new QListWidgetItem("Hiçbir yerde bulunamadı.");
        item->setFlags(Qt::NoItemFlags);
        ui->listWidget->addItem(item);
        ui->listWidget->show();
        ui->infoBrowser->hide();
    }
}

// keyPressEvent'i de güncelle - Enter tuşu için
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!ui->listWidget->isHidden() && ui->listWidget->count() > 0) {
        int currentRow = ui->listWidget->currentRow();

        if (event->key() == Qt::Key_Down) {
            currentRow = (currentRow + 1) % ui->listWidget->count();
            ui->listWidget->setCurrentRow(currentRow);
        }
        else if (event->key() == Qt::Key_Up) {
            currentRow = (currentRow - 1 + ui->listWidget->count()) % ui->listWidget->count();
            ui->listWidget->setCurrentRow(currentRow);
        }
        else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            QListWidgetItem *selected = ui->listWidget->currentItem();
            if (selected) {
                qDebug() << "[Debug] Enter tuşu ile seçim yapıldı - Arama metni: " << ui->lineEdit->text();
                // itemActivated sinyalini manuel olarak tetikle
                on_listWidget_itemActivated(selected);
            }
        }
    }

    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // infoBrowser'dan klavye kontrolleri
    if (obj == ui->infoBrowser && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // SHIFT BASILIYSA → PAYLOAD GEZİNME MODU
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            // SHIFT + Yukarı/Aşağı ok → Payload seçimi
            if (keyEvent->key() == Qt::Key_Down) {
                if (!currentPayloads.isEmpty()) {
                    selectNextPayload();
                    return true;
                }
            }
            else if (keyEvent->key() == Qt::Key_Up) {
                if (!currentPayloads.isEmpty()) {
                    selectPreviousPayload();
                    return true;
                }
            }
            // SHIFT + CTRL+C → Payload kopyala
            else if ((keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_C) ||
                     (keyEvent->modifiers() & Qt::MetaModifier && keyEvent->key() == Qt::Key_C)) {
                if (!currentPayloads.isEmpty()) {
                    copyCurrentPayload();
                    return true;
                }
            }
            // SHIFT + Enter → Kopyala ve kapat
            else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                if (!currentPayloads.isEmpty()) {
                    copyCurrentPayload();
                    QTimer::singleShot(500, this, [this]() {
                        toggleVisibility();
                    });
                    return true;
                }
            }
        }
        // SHIFT BASILMAMIŞSA → NORMAL SCROLL (varsayılan davranış)
        else {
            // Normal Yukarı/Aşağı → İçerikte scroll (Qt'nin varsayılan davranışı)
            if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
                // Qt'nin varsayılan scroll işlemini yap
                return QMainWindow::eventFilter(obj, event);
            }
            // CTRL+C / CMD+C (SHIFT olmadan) → Seçili metni kopyala (normal)
            else if ((keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_C) ||
                     (keyEvent->modifiers() & Qt::MetaModifier && keyEvent->key() == Qt::Key_C)) {
                // Qt'nin varsayılan kopyalama işlemi
                return QMainWindow::eventFilter(obj, event);
            }
            // GERİ DÖN: Backspace/Delete
            else if (keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete) {
                qDebug() << "[EventFilter] infoBrowser'da Backspace/Delete basıldı, geri dönülüyor";

                if (m_isShowingContent) {
                    m_isShowingContent = false;
                    currentPayloads.clear();
                    currentPayloadIndex = -1;
                    showMainMenu();
                    return true;
                } else {
                    navigateBack();
                    return true;
                }
            }
        }
    }

    if (obj == ui->lineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // YÖN TUŞLARI İÇİN NAVİGASYON (listWidget görünürse)
        if (!ui->listWidget->isHidden() && ui->listWidget->count() > 0) {
            int currentRow = ui->listWidget->currentRow();

            if (keyEvent->key() == Qt::Key_Down) {
                currentRow = (currentRow + 1) % ui->listWidget->count();
                ui->listWidget->setCurrentRow(currentRow);
                return true; // Event'i tüket
            }
            else if (keyEvent->key() == Qt::Key_Up) {
                currentRow = (currentRow - 1 + ui->listWidget->count()) % ui->listWidget->count();
                ui->listWidget->setCurrentRow(currentRow);
                return true; // Event'i tüket
            }
            else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                QListWidgetItem *selected = ui->listWidget->currentItem();
                if (selected) {
                    qDebug() << "[Debug] Enter tuşu ile seçim yapıldı";
                    on_listWidget_itemActivated(selected);
                    return true; // Event'i tüket
                }
            }
        }

        // BACKSPACE VE DELETE TUŞLARI
        if (keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete) {
            // Eğer lineEdit boşsa
            if (ui->lineEdit->text().isEmpty()) {

                // İÇERİK GÖSTERİLİYORSA - SADECE İÇERİKTEN ÇIK
                if (m_isShowingContent) {
                    qDebug() << "[Debug] İçerik gösteriliyordu, sadece içerikten çıkılıyor";
                    m_isShowingContent = false;
                    showMainMenu(); // Aynı dizinde liste göster
                    return true; // Event'i tüket
                }
                // İÇERİK GÖSTERİLMİYORSA - NORMAL NAVİGASYON
                else {
                    qDebug() << "[Debug] Normal navigasyon - bir üst dizine gidiliyor";
                    navigateBack();
                    return true; // Event'i tüket
                }
            }
            // LineEdit'te metin varsa, normal davranış
            else {
                ui->infoBrowser->hide();       // Açıklama kutusunu gizle
                if (!ui->lineEdit->text().isEmpty()) {
                    ui->listWidget->show();    // Listeyi tekrar göster
                }
                // İçerik flag'ini sıfırla (arama yapılırken)
                m_isShowingContent = false;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Güncellenmiş itemActivated fonksiyonu - Arama sonuçları için doğru davranış
void MainWindow::on_listWidget_itemActivated(QListWidgetItem *item) {
    if (!item) {
        qDebug() << "[Debug] Item null!";
        return;
    }

    QString key = item->data(Qt::UserRole).toString();
    QString jsonPath = item->data(Qt::UserRole + 1).toString();
    bool showDirectContent = item->data(Qt::UserRole + 2).toBool(); // Arama sonucu mu?

    // ARAMA DURUMUNU KONTROL ET (lineEdit temizlenmeden önce!)
    bool isSearchResult = !ui->lineEdit->text().trimmed().isEmpty();

    qDebug() << "[Debug] Seçilen item: " << key << " JSON: " << jsonPath << " DirectContent: " << showDirectContent << " IsSearchResult: " << isSearchResult;

    // Personal Notes arama sonucundan geldiyse
    if (jsonPath == "personal_notes") {
        qDebug() << "[Debug] Personal note arama sonucundan geldi, Personal Notlar'a yönlendiriliyor";

        // Arama kutusunu temizle
        ui->lineEdit->clear();

        // Personal Notlar menüsüne git
        currentPath = "personal_notes";

        // Personal Notlar içeriğini göster
        commandData = QJsonObject();

        QJsonObject createNewNote;
        createNewNote["desc"] = "Yeni bir kişisel not oluşturun";
        createNewNote["is_create_note_action"] = true;
        commandData["➕ Yeni Not Oluştur"] = createNewNote;

        QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
        QJsonArray notesArray = personalNotes["notes"].toArray();

        for (int i = 0; i < notesArray.size(); ++i) {
            QJsonObject note = notesArray[i].toObject();
            QString noteTitle = note["title"].toString();
            bool isFavorite = note["favorite"].toBool();

            // Favori ise ⭐ emoji ekle
            QString displayTitle = isFavorite ? ("⭐ " + noteTitle) : noteTitle;

            QJsonObject noteItem;
            noteItem["desc"] = "📅 " + note["created"].toString().left(10);
            noteItem["content"] = note["content"].toString();
            noteItem["created"] = note["created"].toString();
            noteItem["modified"] = note["modified"].toString();
            noteItem["tags"] = note["tags"].toString();
            noteItem["favorite"] = isFavorite;
            noteItem["note_id"] = note["id"].toString();
            noteItem["is_personal_note"] = true;

            commandData[displayTitle] = noteItem;
        }

        showMainMenu();

        // Aranan notu seç
        for (int i = 0; i < ui->listWidget->count(); ++i) {
            QListWidgetItem* listItem = ui->listWidget->item(i);
            if (listItem->data(Qt::UserRole).toString() == key) {
                ui->listWidget->setCurrentRow(i);
                break;
            }
        }

        return;
    }

    // Eğer başka bir JSON'dan geliyorsa, önce o JSON'u yükle
    if (jsonPath != "current" && jsonPath != currentPath && !jsonPath.isEmpty()) {
        qDebug() << "[Debug] Başka JSON'dan gelen sonuç, JSON yükleniyor: " << jsonPath;

        // İçerik flag'ini sıfırla
        m_isShowingContent = false;

        // Mevcut yolu geçmişe ekle
        pathHistory.push(currentPath);
        // Mevcut seçili index'i hafızaya al
        if (!ui->listWidget->isHidden() && ui->listWidget->count() > 0) {
            selectedIndexHistory.push(ui->listWidget->currentRow());
            qDebug() << "[Debug] Seçim hafızaya eklendi (JSON geçişi): " << ui->listWidget->currentRow();
        } else {
            selectedIndexHistory.push(0);  // Liste görünmüyorsa 0 varsayılan
        }

        currentPath = jsonPath;

        // Eski veriyi temizle
        commandData = QJsonObject();

        // Yeni JSON dosyasını yükle
        loadCommandData(jsonPath);

        if (commandData.isEmpty()) {
            qDebug() << "[Debug] JSON yüklenemedi!";
            QMessageBox::warning(this, "Hata", "Dosya yüklenemedi: " + jsonPath);

            // Geçmişten geri dön
            if (!pathHistory.isEmpty()) {
                currentPath = pathHistory.pop();
                loadCommandData(currentPath);
                showMainMenu();
            }
            return;
        }

        // Artık yeni JSON yüklendiğine göre, seçili öğeyi işle
        if (!commandData.contains(key)) {
            qDebug() << "[Debug] Key yeni JSON'da bulunamadı: " << key;
            showMainMenu();
            ui->lineEdit->clear();
            return;
        }
    }

    // Normal işleme devam et
    if (!commandData.contains(key)) {
        qDebug() << "[Debug] Key commandData'da bulunamadı: " << key;
        return;
    }

    QJsonObject selected = commandData[key].toObject();

    // *** ANA DÜZELTME: showDirectContent flag'ini dikkate al ***
    // Eğer arama yapılmışsa ve showDirectContent true ise direkt içerik göster
    if (isSearchResult && showDirectContent) {
        qDebug() << "[Debug] Arama sonucu - direkt içerik gösteriliyor: " << key;

        // Arama kutusunu temizle
        ui->lineEdit->clear();

        // İçeriği göster
        showCommandInfo(key);
        return; // Fonksiyondan çık
    }

    // Eğer arama yapılmışsa ve showDirectContent false ise (klasör) normal navigasyon yap
    if (isSearchResult && !showDirectContent) {
        qDebug() << "[Debug] Arama sonucu - klasör navigasyonu: " << key;
        // Arama kutusunu temizle
        ui->lineEdit->clear();
        // Alt kısımda normal navigasyon kodu çalışacak
    }

    // 'file' anahtarı kontrolü (alt klasör) - Normal navigasyon için
    if (selected.contains("file") && selected["file"].isString()) {
        QString fileName = selected["file"].toString();

        // ÖZEL: Personal Notlar klasörü kontrolü
        if (fileName == "personal_notes_virtual.json") {
            qDebug() << "[Debug] Personal Notlar klasörü açılıyor";

            // İçerik flag'ini sıfırla
            m_isShowingContent = false;

            // Mevcut yolu geçmişe ekle
            pathHistory.push(currentPath);
            selectedIndexHistory.push(ui->listWidget->currentRow());

            // Personal Notlar'ı commandData'ya yükle
            currentPath = "personal_notes";  // Özel işaretleyici

            // commandData'yı temizle ve notları ekle
            commandData = QJsonObject();

            // İLK OLARAK "Yeni Not Oluştur" seçeneğini ekle (sabit)
            QJsonObject createNewNote;
            createNewNote["desc"] = "Yeni bir kişisel not oluşturun";
            createNewNote["is_create_note_action"] = true;  // Bu özel bir aksiyon işareti
            commandData["➕ Yeni Not Oluştur"] = createNewNote;

            QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
            QJsonArray notesArray = personalNotes["notes"].toArray();

            qDebug() << "[Debug] Personal Notlar yükleniyor, toplam not:" << notesArray.size();

            // Her notu commandData'ya ekle
            for (int i = 0; i < notesArray.size(); ++i) {
                QJsonObject note = notesArray[i].toObject();
                QString title = note["title"].toString();
                QString noteKey = title;  // Başlık key olarak kullanılsın

                // Not içeriğini düzenle - showCommandInfo için uygun format
                QJsonObject noteItem;
                noteItem["desc"] = "📅 " + note["created"].toString().left(10);
                noteItem["content"] = note["content"].toString();
                noteItem["created"] = note["created"].toString();
                noteItem["modified"] = note["modified"].toString();
                noteItem["is_personal_note"] = true;

                commandData[noteKey] = noteItem;
            }

            // Menüyü göster
            showMainMenu();
            ui->lineEdit->clear();
            ui->infoBrowser->hide();
            return;
        }

        QString nextFile = ":/json/" + fileName;

        qDebug() << "[Debug] Alt klasöre geçiliyor: " << nextFile;

        // İçerik flag'ini sıfırla
        m_isShowingContent = false;

        // Mevcut yolu geçmişe ekle ve yeni yolu ayarla
        pathHistory.push(currentPath);
        // Mevcut seçili index'i hafızaya al
        selectedIndexHistory.push(ui->listWidget->currentRow());
        qDebug() << "[Debug] Seçim hafızaya eklendi: " << ui->listWidget->currentRow();

        currentPath = nextFile;

        // Eski veriyi temizle
        commandData = QJsonObject();

        // Yeni JSON dosyasını yükle
        loadCommandData(nextFile);

        if (commandData.isEmpty()) {
            qDebug() << "[Debug] Alt klasör JSON'u yüklenemedi!";
            QMessageBox::warning(this, "Hata", "Alt kategori dosyası yüklenemedi: " + nextFile);

            // Geçmişten geri dön
            if (!pathHistory.isEmpty()) {
                currentPath = pathHistory.pop();
                loadCommandData(currentPath);
                showMainMenu();
            }
            return;
        }

        // Yeni menüyü göster
        showMainMenu();
        ui->lineEdit->clear();
        ui->infoBrowser->hide();
    }
    else {
        // ÖZEL: "Yeni Not Oluştur" aksiyonu kontrolü
        if (selected.contains("is_create_note_action") && selected["is_create_note_action"].toBool()) {
            qDebug() << "[Debug] Yeni Not Oluştur aksiyonu tetiklendi";

            // PersonalNotesDialog'u aç
            PersonalNotesDialog *dialog = new PersonalNotesDialog(this);

            connect(dialog, &PersonalNotesDialog::noteSaved, this,
                    [this](const QString &title, const QString &content, const QString &tags, bool isFavorite, const QString &) {
                // Notu ekle
                addPersonalNote(title, content, tags, isFavorite);

                // Listeyi güncelle
                if (currentPath == "personal_notes") {
                    // commandData'yı yeniden oluştur
                    commandData = QJsonObject();

                    // "Yeni Not Oluştur" seçeneğini tekrar ekle
                    QJsonObject createNewNote;
                    createNewNote["desc"] = "Yeni bir kişisel not oluşturun";
                    createNewNote["is_create_note_action"] = true;
                    commandData["➕ Yeni Not Oluştur"] = createNewNote;

                    // Notları yeniden yükle
                    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
                    QJsonArray notesArray = personalNotes["notes"].toArray();

                    for (int i = 0; i < notesArray.size(); ++i) {
                        QJsonObject note = notesArray[i].toObject();
                        QString noteTitle = note["title"].toString();
                        bool isFavorite = note["favorite"].toBool();

                        // Favori ise ⭐ emoji ekle
                        QString displayTitle = isFavorite ? ("⭐ " + noteTitle) : noteTitle;

                        QJsonObject noteItem;
                        noteItem["desc"] = "📅 " + note["created"].toString().left(10);
                        noteItem["content"] = note["content"].toString();
                        noteItem["created"] = note["created"].toString();
                        noteItem["modified"] = note["modified"].toString();
                        noteItem["tags"] = note["tags"].toString();
                        noteItem["favorite"] = isFavorite;
                        noteItem["note_id"] = note["id"].toString();
                        noteItem["is_personal_note"] = true;

                        commandData[displayTitle] = noteItem;
                    }

                    // Listeyi yenile
                    showMainMenu();
                }
            });

            dialog->exec();
            dialog->deleteLater();

            ui->lineEdit->clear();
            return;
        }

        // Gerçek içerik - showCommandInfo çağır
        qDebug() << "[Debug] İçerik gösteriliyor: " << key;

        // İçerik gösterilirken de seçim hafızaya alınsın
        if (!ui->listWidget->isHidden() && ui->listWidget->count() > 0) {
            // Eğer pathHistory'ye yeni bir yol eklenmemişse, mevcut konumu kaydet
            // (İçerik açıldığında klasör değişmediği için path aynı kalır)
            if (selectedIndexHistory.isEmpty() || selectedIndexHistory.size() == pathHistory.size()) {
                selectedIndexHistory.push(ui->listWidget->currentRow());
                qDebug() << "[Debug] İçerik açılırken seçim hafızaya eklendi: " << ui->listWidget->currentRow();
            }
        }

        ui->lineEdit->clear(); // Arama kutusunu temizle
        showCommandInfo(key);
    }
}

// Pencere kapatma olayını override edin (X tuşuna basılırsa minimize etsin):
void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon->isVisible()) {
        // Pencereyi kapat düğmesine basılırsa sadece gizle
        hide();
        isVisibleFlag = false;

        // İlk defada bilgi mesajı göster
        static bool firstHide = true;
        if (firstHide) {
            trayIcon->showMessage("Cyn0",
                                  "Program sistem tepsisinde çalışmaya devam ediyor.\n"
                                  "Tamamen kapatmak için sağ tık menüden çıkış yapın.",
                                  QSystemTrayIcon::Information, 4000);
            firstHide = false;
        }

        event->ignore(); // Kapatma olayını iptal et
    } else {
        event->accept(); // Tray icon yoksa normal şekilde kapat
    }
}

void MainWindow::navigateBack() {
    qDebug() << "[Debug] navigateBack çağrıldı. Mevcut path: " << currentPath;
    qDebug() << "[Debug] PathHistory boyutu: " << pathHistory.size();

    // ÖZEL: Personal Notlar içindeyse ana dizine dön
    if (currentPath == "personal_notes") {
        qDebug() << "[Debug] Personal Notlar içinden çıkılıyor";

        // Geçmişi temizle
        if (!pathHistory.isEmpty()) {
            pathHistory.pop();
        }
        if (!selectedIndexHistory.isEmpty()) {
            // Personal Notlar seçili kalsın (index 0)
            selectedIndexHistory.top() = 0;
        }

        // Ana dizine dön
        currentPath = ":/json/index.json";
        commandData = QJsonObject();
        loadCommandData(currentPath);
        mergePersonalNotesIntoMenu();
        showMainMenu();
        return;
    }

    // Ana dizindeyse (index.json) veya geçmiş yoksa geri gitme
    if (currentPath == ":/json/index.json" || pathHistory.isEmpty()) {
        qDebug() << "[Debug] Ana dizindeyiz veya geçmiş yok, geri gidemez";
        return;
    }

    // Geçmişten bir önceki dizini al
    QString previousPath = pathHistory.pop();
    qDebug() << "[Debug] Geri dönülecek path: " << previousPath;

    // Seçim hafızasından pop (geri dönerken kullanmak için showMainMenu'de kullanılacak)
    if (!selectedIndexHistory.isEmpty()) {
        // Pop etmiyoruz çünkü showMainMenu'de kullanılacak
        qDebug() << "[Debug] Seçim hafızasında değer var: " << selectedIndexHistory.top();
    }

    // Mevcut yolu güncelle
    currentPath = previousPath;

    // Eski veriyi temizle
    commandData = QJsonObject();

    // Önceki JSON dosyasını yükle
    loadCommandData(currentPath);

    if (commandData.isEmpty()) {
        qDebug() << "[Debug] Önceki JSON yüklenemedi!";
        return;
    }

    // Ana dizine dönüyorsak Personal Notlar'ı tekrar ekle
    if (currentPath == ":/json/index.json") {
        qDebug() << "[Debug] Ana dizine dönüldü, Personal Notlar ekleniyor";
        mergePersonalNotesIntoMenu();
    }

    // Menüyü göster (seçim hafızası burada geri yüklenecek)
    showMainMenu();

    // Seçim hafızasını temizle (kullanıldı)
    if (!selectedIndexHistory.isEmpty()) {
        selectedIndexHistory.pop();
    }

    // LineEdit'i temizle ve infoBrowser'ı gizle
    ui->lineEdit->clear();
    ui->infoBrowser->hide();

    qDebug() << "[Debug] Bir önceki dizine geri dönüldü: " << currentPath;
}

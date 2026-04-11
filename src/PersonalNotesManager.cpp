#include "mainwindow.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QUuid>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include "DatabaseManager.h"
#include "qaesencryption.h"

// Personal Notes sistemini başlat
void MainWindow::initPersonalNotes()
{
    // Cihaz veri yolunu DBManager içinde kullanıyoruz ancak eski dosya migration'ı için
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);
    if (!appDir.exists()) appDir.mkpath(".");
    personalNotesPath = appDir.filePath("personal_notes.json");
    
    // Uygulama başlatıldığında Notlar şifrelemesi için Master Key sor
    bool ok;
    QString pwd = QInputDialog::getText(this, "Güvenlik Duvarı - Cyn0", 
      "Kişisel Notlar SQLite Veritabanı için AES-256 Parolanızı girin:\n(İlk defa açıyorsanız bu parola Master Key'iniz olur!)", 
      QLineEdit::Password, "", &ok);
      
    if (ok && !pwd.isEmpty()) {
        m_masterPassword = pwd;
    } else {
        m_masterPassword = "";
        QMessageBox::warning(this, "Güvenlik Uyarısı", "Master Parola girilmedi. Kişisel Notlar modülü devredışı bırakıldı!");
        return; // Devam etme
    }

    // Notları yükle
    loadPersonalNotes();
}

// Personal Notes'u DB'den veya Eski JSON dosyasından AES ile oku
void MainWindow::loadPersonalNotes()
{
    if (m_masterPassword.isEmpty()) return;
    
    QByteArray jsonData;
    QString encryptedNotes = DatabaseManager::instance().getPersonalNote("ALL_NOTES_JSON");
    
    if (encryptedNotes.isEmpty()) {
        // MIGRATION: Eğer SQLite veritabanında "ALL_NOTES_JSON" blob'u yoksa eski dosyaya bak
        QFile file(personalNotesPath);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            qDebug() << "[PersonalNotes] Eski JSON Notlar bulundu, SQLite+AES moduna geçirilecek!";
            jsonData = file.readAll();
            file.close();
            file.remove(); // Güvenlik sebebiyle plaintext eski dosyayı diskten sil !!
        } else {
            qDebug() << "[PersonalNotes] DB boş, yeni yapı oluşturuluyor.";
            personalNotesData = QJsonObject();
            QJsonObject personalNotes;
            personalNotes["_meta"] = QJsonObject{
                {"desc", "Kişisel notlarınız ve hızlı erişim"},
                {"total_notes", 0},
                {"created", QDateTime::currentDateTime().toString(Qt::ISODate)}
            };
            personalNotes["notes"] = QJsonArray();
            personalNotesData["Personal Notlar"] = personalNotes;
            savePersonalNotes();
            return;
        }
    } else {
        // AES ŞİFRE ÇÖZME İŞLEMİ (DECRYPTION)
        qDebug() << "[PersonalNotes] SQLite'dan AES şifreli veri çekildi. Çözülüyor...";
        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC, QAESEncryption::PKCS7);
        QByteArray hashKey = QCryptographicHash::hash(m_masterPassword.toUtf8(), QCryptographicHash::Sha256);
        QByteArray hashIV = QCryptographicHash::hash(m_masterPassword.toUtf8(), QCryptographicHash::Md5);
        
        QByteArray decodedText = encryption.decode(QByteArray::fromBase64(encryptedNotes.toUtf8()), hashKey, hashIV);
        jsonData = encryption.removePadding(decodedText);
    }

    // JSON parse et
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[PersonalNotes] AES Çözme Veya JSON Hatası:" << parseError.errorString();
        QMessageBox::critical(this, "Şifre Hatası?", "Notlar çözülemedi. Parolanızı yanlış girmiş olabilirsiniz!");
        return;
    }

    if (doc.isObject()) {
        personalNotesData = doc.object();

        // Eski format kontrolü var ise
        QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
        if (personalNotes["notes"].isObject() && !personalNotes["notes"].toObject().isEmpty()) {
            QJsonObject oldNotes = personalNotes["notes"].toObject();
            QJsonArray newNotes;
            for (auto it = oldNotes.constBegin(); it != oldNotes.constEnd(); ++it) {
                QJsonObject note = it.value().toObject();
                note["id"] = it.key();
                newNotes.append(note);
            }
            personalNotes["notes"] = newNotes;
            personalNotesData["Personal Notlar"] = personalNotes;
            savePersonalNotes(); 
        }

        int noteCount = personalNotesData["Personal Notlar"].toObject()["notes"].toArray().size();
        qDebug() << "[PersonalNotes] Notlar başarıyla çözüldü ve yüklendi. Toplam not:" << noteCount;
    }
}

// Personal Notes'u şifreleyerek SQLite Database'e (Diske değil!) kaydet
void MainWindow::savePersonalNotes()
{
    if (m_masterPassword.isEmpty()) return;

    // Önce JSON'a dönüştür
    QJsonDocument doc(personalNotesData);
    QByteArray rawData = doc.toJson(QJsonDocument::Compact); // Boşluksuz daha güvenli/küçük

    // AES ŞİFRELEME (ENCRYPTION)
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC, QAESEncryption::PKCS7);
    QByteArray hashKey = QCryptographicHash::hash(m_masterPassword.toUtf8(), QCryptographicHash::Sha256);
    QByteArray hashIV = QCryptographicHash::hash(m_masterPassword.toUtf8(), QCryptographicHash::Md5);
    
    QByteArray encodedText = encryption.encode(rawData, hashKey, hashIV);
    QString base64Data = QString::fromUtf8(encodedText.toBase64());

    // Veritabanına Yaz
    if (DatabaseManager::instance().savePersonalNote("ALL_NOTES_JSON", base64Data)) {
        qDebug() << "[PersonalNotes] AES şifreli Note Verisi başarıyla SQLite'a kaydedildi.";
    } else {
        QMessageBox::critical(this, "Şifreleme Hatası", "Notlar veritabanına kayıt edilemedi!");
    }
}

// Yeni not ekle
void MainWindow::addPersonalNote(const QString &title, const QString &content, const QString &tags, bool isFavorite, const QString &existingNoteId)
{
    // Benzersiz ID oluştur (veya mevcut ID'yi kullan)
    QString noteId = existingNoteId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : existingNoteId;

    // Timestamp
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Not objesi oluştur - ÇOK OKUNAKLI FORMAT
    QJsonObject note;
    note["id"] = noteId;
    note["title"] = title;
    note["content"] = content;
    note["tags"] = tags;
    note["favorite"] = isFavorite;
    note["created"] = timestamp;
    note["modified"] = timestamp;

    // Personal Notlar objesini al
    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
    QJsonArray notes = personalNotes["notes"].toArray();

    // Yeni notu array'in BAŞINA ekle (en yeni notlar üstte)
    notes.prepend(note);

    // Meta bilgiyi güncelle
    QJsonObject meta = personalNotes["_meta"].toObject();
    meta["total_notes"] = notes.size();
    meta["last_modified"] = timestamp;

    personalNotes["notes"] = notes;
    personalNotes["_meta"] = meta;
    personalNotesData["Personal Notlar"] = personalNotes;

    // Diske kaydet
    savePersonalNotes();

    qDebug() << "[PersonalNotes] Yeni not eklendi:" << title << "Toplam not:" << notes.size();

    // Bildirim göster
    if (trayIcon && trayIcon->isVisible()) {
        QString favoriteIcon = isFavorite ? "⭐ " : "";
        trayIcon->showMessage("✅ Not Kaydedildi",
                              favoriteIcon + "\"" + title + "\"",
                              QSystemTrayIcon::Information, 2000);
    }
}

// Notu güncelle
void MainWindow::updatePersonalNote(const QString &noteId, const QString &title, const QString &content, const QString &tags, bool isFavorite)
{
    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
    QJsonArray notes = personalNotes["notes"].toArray();

    // Notu bul ve güncelle
    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject note = notes[i].toObject();
        if (note["id"].toString() == noteId) {
            note["title"] = title;
            note["content"] = content;
            note["tags"] = tags;
            note["favorite"] = isFavorite;
            note["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            notes[i] = note;
            break;
        }
    }

    // Meta bilgiyi güncelle
    QJsonObject meta = personalNotes["_meta"].toObject();
    meta["last_modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    personalNotes["notes"] = notes;
    personalNotes["_meta"] = meta;
    personalNotesData["Personal Notlar"] = personalNotes;

    // Diske kaydet
    savePersonalNotes();

    qDebug() << "[PersonalNotes] Not güncellendi:" << title;

    // Bildirim göster
    if (trayIcon && trayIcon->isVisible()) {
        QString favoriteIcon = isFavorite ? "⭐ " : "";
        trayIcon->showMessage("✏️ Not Güncellendi",
                              favoriteIcon + "\"" + title + "\"",
                              QSystemTrayIcon::Information, 2000);
    }
}

// Notu sil
void MainWindow::deletePersonalNote(const QString &noteId)
{
    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
    QJsonArray notes = personalNotes["notes"].toArray();

    QString deletedTitle;

    // Notu bul ve sil
    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject note = notes[i].toObject();
        if (note["id"].toString() == noteId) {
            deletedTitle = note["title"].toString();
            notes.removeAt(i);
            break;
        }
    }

    // Meta bilgiyi güncelle
    QJsonObject meta = personalNotes["_meta"].toObject();
    meta["total_notes"] = notes.size();
    meta["last_modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    personalNotes["notes"] = notes;
    personalNotes["_meta"] = meta;
    personalNotesData["Personal Notlar"] = personalNotes;

    // Diske kaydet
    savePersonalNotes();

    qDebug() << "[PersonalNotes] Not silindi:" << deletedTitle;

    // Bildirim göster
    if (trayIcon && trayIcon->isVisible()) {
        trayIcon->showMessage("🗑️ Not Silindi",
                              "\"" + deletedTitle + "\"",
                              QSystemTrayIcon::Information, 2000);
    }
}

// Hızlı not dialog'unu göster
void MainWindow::showQuickNoteDialog()
{
    QuickNoteDialog *dialog = new QuickNoteDialog(this);

    connect(dialog, &QuickNoteDialog::quickNoteSaved, this, [this](const QString &content) {
        // Başlık olarak ilk iki kelimeyi al
        QStringList words = content.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QString title;
        if (words.size() >= 2) {
            title = words[0] + " " + words[1];
        } else if (words.size() == 1) {
            title = words[0];
        } else {
            title = "Yeni Not";
        }

        // Notu ekle (tags ve favorite boş)
        addPersonalNote(title, content, "", false);

        // Eğer ana pencere açıksa ve personal notes içindeysek, listeyi güncelle
        if (isVisibleFlag && currentPath == "personal_notes") {
            // Personal Notlar listesini yenile
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
                QString noteKey = noteTitle;

                QJsonObject noteItem;
                noteItem["desc"] = "📅 " + note["created"].toString().left(10);
                noteItem["content"] = note["content"].toString();
                noteItem["created"] = note["created"].toString();
                noteItem["modified"] = note["modified"].toString();
                noteItem["tags"] = note["tags"].toString();
                noteItem["favorite"] = note["favorite"].toBool();
                noteItem["note_id"] = note["id"].toString();
                noteItem["is_personal_note"] = true;

                commandData[noteKey] = noteItem;
            }

            showMainMenu();
        }
    });

    dialog->exec();
    dialog->deleteLater();
}

// Not düzenleme dialog'unu göster
void MainWindow::showEditNoteDialog(const QString &noteId)
{
    // Notu bul
    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
    QJsonArray notes = personalNotes["notes"].toArray();

    QJsonObject foundNote;
    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject note = notes[i].toObject();
        if (note["id"].toString() == noteId) {
            foundNote = note;
            break;
        }
    }

    if (foundNote.isEmpty()) {
        qDebug() << "[PersonalNotes] Not bulunamadı:" << noteId;
        return;
    }

    // Düzenleme dialog'unu aç
    PersonalNotesDialog *dialog = new PersonalNotesDialog(this, noteId, true);
    dialog->setNoteTitle(foundNote["title"].toString());
    dialog->setNoteContent(foundNote["content"].toString());
    dialog->setNoteTags(foundNote["tags"].toString());
    dialog->setIsFavorite(foundNote["favorite"].toBool());

    connect(dialog, &PersonalNotesDialog::noteSaved, this,
            [this](const QString &title, const QString &content, const QString &tags, bool isFavorite, const QString &id) {
        // Notu güncelle
        updatePersonalNote(id, title, content, tags, isFavorite);

        // Eğer personal notes içindeysek, listeyi güncelle
        if (currentPath == "personal_notes") {
            // Listeyi yenile
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

                QJsonObject noteItem;
                noteItem["desc"] = "📅 " + note["created"].toString().left(10);
                noteItem["content"] = note["content"].toString();
                noteItem["created"] = note["created"].toString();
                noteItem["modified"] = note["modified"].toString();
                noteItem["tags"] = note["tags"].toString();
                noteItem["favorite"] = note["favorite"].toBool();
                noteItem["note_id"] = note["id"].toString();
                noteItem["is_personal_note"] = true;

                commandData[noteTitle] = noteItem;
            }

            showMainMenu();
        }
    });

    dialog->exec();
    dialog->deleteLater();
}

// Notlar için yedekleme (program kapatılmadan önce)
void MainWindow::backupPersonalNotes()
{
    if (m_masterPassword.isEmpty()) return;

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir backupDir(QDir(appDataPath).filePath("backups"));
    if (!backupDir.exists()) backupDir.mkpath(".");

    // AES'li Ham Blobu veritabanından çek (Şifreli kalsın backup'ta da, bu harika)
    QString currentData = DatabaseManager::instance().getPersonalNote("ALL_NOTES_JSON");
    if(currentData.isEmpty()) return;
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(currentData.toUtf8());
    QString currentHash = hash.result().toHex();

    QString hashFilePath = backupDir.filePath("last_backup_hash.txt");
    QFile hashFile(hashFilePath);
    QString lastHash;
    if (hashFile.open(QIODevice::ReadOnly)) {
        lastHash = hashFile.readAll().trimmed();
        hashFile.close();
    }

    // Değişiklik varsa Encrypted dosyayı JSON olarak değil SQL-Backup (veya Base64 TXT) olarak yedekle
    if (currentHash != lastHash) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupFilePath = backupDir.filePath("personal_notes_AES_" + timestamp + ".txt");

        QFile backupFile(backupFilePath);
        if (backupFile.open(QIODevice::WriteOnly)) {
            backupFile.write(currentData.toUtf8());
            backupFile.close();

            if (hashFile.open(QIODevice::WriteOnly)) {
                hashFile.write(currentHash.toUtf8());
                hashFile.close();
            }
            qDebug() << "[Backup] AES Yedeklemesi oluşturuldu:" << backupFilePath;
        }
    }

    QStringList backupFiles = backupDir.entryList(QStringList() << "personal_notes_AES_*.txt", QDir::Files, QDir::Time);
    while (backupFiles.size() > 10) {
        QString oldestBackup = backupFiles.takeLast();
        backupDir.remove(oldestBackup);
    }
}

// Personal Notlar'ı ana menüye ekle - KLASÖR YAPISI
void MainWindow::mergePersonalNotesIntoMenu()
{
    QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
    QJsonArray notesArray = personalNotes["notes"].toArray();
    int noteCount = notesArray.size();

    // Personal Notlar klasörü (file key'i ile)
    QJsonObject personalNotesFolder;
    personalNotesFolder["desc"] = "Kişisel notlarınız (" + QString::number(noteCount) + " not)";
    personalNotesFolder["file"] = "personal_notes_virtual.json";  // Sanal dosya
    personalNotesFolder["is_personal_notes"] = true;

    // Ana menüye ekle
    commandData["Personal Notlar"] = personalNotesFolder;

    qDebug() << "[PersonalNotes] Ana menüye klasör olarak eklendi, toplam not:" << noteCount;
}

// Backup Yöneticisi - Backup'ları listeleme, geri yükleme, silme
void MainWindow::showBackupManager()
{
    // Backup klasörünü kontrol et
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir backupDir(QDir(appDataPath).filePath("backups"));

    if (!backupDir.exists()) {
        QMessageBox::information(this, "Backup Yöneticisi", "Henüz hiç backup alınmamış.");
        return;
    }

    // Dialog oluştur
    QDialog *backupDialog = new QDialog(this);
    backupDialog->setWindowTitle("🗂️ Backup Yöneticisi");
    backupDialog->setMinimumSize(600, 400);

    // Dark tema stili
    backupDialog->setStyleSheet(
        "QDialog { background-color: #1C1C1E; }"
        "QLabel { color: #FFFFFF; font-size: 14px; }"
        "QListWidget { background-color: #2A2A2C; color: #FFFFFF; border: 1px solid #3A3A3C; border-radius: 8px; padding: 8px; }"
        "QListWidget::item { padding: 10px; border-radius: 6px; margin: 2px; }"
        "QListWidget::item:selected { background-color: #7C3AED; }"
        "QListWidget::item:hover { background-color: #2C2C2E; }"
        "QPushButton { background-color: #7C3AED; color: #FFFFFF; border-radius: 8px; padding: 10px 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #8B5CF6; }"
        "QPushButton:pressed { background-color: #6D28D9; }"
        "QPushButton[class=\"danger\"] { background-color: #FF3B30; }"
        "QPushButton[class=\"danger\"]:hover { background-color: #D70015; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(backupDialog);

    // Başlık
    QLabel *titleLabel = new QLabel("📦 Mevcut Backup'lar", backupDialog);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
    mainLayout->addWidget(titleLabel);

    // Backup listesi
    QListWidget *backupList = new QListWidget(backupDialog);
    mainLayout->addWidget(backupList);

    // Backup dosyalarını yükle
    QStringList backupFiles = backupDir.entryList(QStringList() << "personal_notes_*.json", QDir::Files, QDir::Time);

    if (backupFiles.isEmpty()) {
        QListWidgetItem *emptyItem = new QListWidgetItem("Hiç backup bulunamadı");
        emptyItem->setFlags(Qt::NoItemFlags);
        backupList->addItem(emptyItem);
    } else {
        for (const QString &fileName : backupFiles) {
            QFileInfo fileInfo(backupDir.filePath(fileName));
            QString displayText = QString("📅 %1 - %2 (%3 KB)")
                .arg(fileInfo.lastModified().toString("dd/MM/yyyy HH:mm:ss"))
                .arg(fileName)
                .arg(fileInfo.size() / 1024);

            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, backupDir.filePath(fileName));  // Tam dosya yolunu sakla
            backupList->addItem(item);
        }
    }

    // Bilgi etiketi
    QLabel *infoLabel = new QLabel(QString("Toplam %1 backup bulundu").arg(backupFiles.size()), backupDialog);
    infoLabel->setStyleSheet("color: #8E8E93; font-size: 12px;");
    mainLayout->addWidget(infoLabel);

    // Butonlar
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *restoreButton = new QPushButton("↩️ Geri Yükle", backupDialog);
    QPushButton *deleteButton = new QPushButton("🗑️ Sil", backupDialog);
    deleteButton->setProperty("class", "danger");
    QPushButton *closeButton = new QPushButton("❌ Kapat", backupDialog);

    buttonLayout->addWidget(restoreButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Geri yükleme butonu
    connect(restoreButton, &QPushButton::clicked, [=]() {
        QListWidgetItem *selectedItem = backupList->currentItem();
        if (!selectedItem || !selectedItem->flags().testFlag(Qt::ItemIsEnabled)) {
            QMessageBox::warning(backupDialog, "Uyarı", "Lütfen geri yüklenecek bir backup seçin!");
            return;
        }

        QString backupFilePath = selectedItem->data(Qt::UserRole).toString();

        QMessageBox::StandardButton reply = QMessageBox::question(
            backupDialog,
            "Backup Geri Yükleme",
            "Bu backup'ı geri yüklemek istediğinizden emin misiniz?\n\nMevcut notlarınızın üzerine yazılacak!",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            // Backup dosyasını oku
            QFile backupFile(backupFilePath);
            if (backupFile.open(QIODevice::ReadOnly)) {
                QByteArray backupData = backupFile.readAll();
                backupFile.close();

                // SQLite'a geri aktar AES blob'u olarsk
                DatabaseManager::instance().savePersonalNote("ALL_NOTES_JSON", QString(backupData));
                
                // Notları yeniden yükle
                loadPersonalNotes();
                mergePersonalNotesIntoMenu();

                QMessageBox::information(backupDialog, "Başarılı", "AES Backup başarıyla geri yüklendi!");
                backupDialog->accept();
            } else {
                QMessageBox::critical(backupDialog, "Hata", "Backup dosyası okunamadı!");
            }
        }
    });

    // Silme butonu
    connect(deleteButton, &QPushButton::clicked, [=]() {
        QListWidgetItem *selectedItem = backupList->currentItem();
        if (!selectedItem || !selectedItem->flags().testFlag(Qt::ItemIsEnabled)) {
            QMessageBox::warning(backupDialog, "Uyarı", "Lütfen silinecek bir backup seçin!");
            return;
        }

        QString backupFilePath = selectedItem->data(Qt::UserRole).toString();

        QMessageBox::StandardButton reply = QMessageBox::question(
            backupDialog,
            "Backup Silme",
            "Bu backup'ı silmek istediğinizden emin misiniz?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            if (QFile::remove(backupFilePath)) {
                delete selectedItem;
                infoLabel->setText(QString("Toplam %1 backup bulundu").arg(backupList->count()));
                QMessageBox::information(backupDialog, "Başarılı", "Backup silindi!");
            } else {
                QMessageBox::critical(backupDialog, "Hata", "Backup silinemedi!");
            }
        }
    });

    // Kapat butonu
    connect(closeButton, &QPushButton::clicked, backupDialog, &QDialog::accept);

    backupDialog->exec();
    backupDialog->deleteLater();
}

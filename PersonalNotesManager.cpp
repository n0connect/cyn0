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

// Personal Notes sistemini başlat
void MainWindow::initPersonalNotes()
{
    // Uygulama veri klasörünü al
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);

    // Klasör yoksa oluştur
    if (!appDir.exists()) {
        appDir.mkpath(".");
        qDebug() << "[PersonalNotes] Veri klasörü oluşturuldu:" << appDataPath;
    }

    // Personal notes dosya yolu
    personalNotesPath = appDir.filePath("personal_notes.json");
    qDebug() << "[PersonalNotes] Not dosya yolu:" << personalNotesPath;

    // Notları yükle
    loadPersonalNotes();
}

// Personal Notes'u diskten yükle
void MainWindow::loadPersonalNotes()
{
    QFile file(personalNotesPath);

    // Dosya yoksa boş yapı oluştur
    if (!file.exists()) {
        qDebug() << "[PersonalNotes] Dosya bulunamadı, yeni oluşturuluyor";
        personalNotesData = QJsonObject();

        // Basit ve temiz yapı
        QJsonObject personalNotes;
        personalNotes["_meta"] = QJsonObject{
            {"desc", "Kişisel notlarınız ve hızlı erişim"},
            {"total_notes", 0},
            {"created", QDateTime::currentDateTime().toString(Qt::ISODate)}
        };
        personalNotes["notes"] = QJsonArray();  // Array olarak, daha düzenli

        personalNotesData["Personal Notlar"] = personalNotes;
        savePersonalNotes();
        return;
    }

    // Dosyayı oku
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[PersonalNotes] Dosya açılamadı:" << file.errorString();
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // JSON parse et
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[PersonalNotes] JSON parse hatası:" << parseError.errorString();
        return;
    }

    if (doc.isObject()) {
        personalNotesData = doc.object();

        // Eski format kontrolü - object ise array'e çevir
        QJsonObject personalNotes = personalNotesData["Personal Notlar"].toObject();
        if (personalNotes["notes"].isObject() && !personalNotes["notes"].toObject().isEmpty()) {
            qDebug() << "[PersonalNotes] Eski format tespit edildi, yeni formata çeviriliyor...";

            QJsonObject oldNotes = personalNotes["notes"].toObject();
            QJsonArray newNotes;

            // Object'ten Array'e çevir
            for (auto it = oldNotes.constBegin(); it != oldNotes.constEnd(); ++it) {
                QJsonObject note = it.value().toObject();
                note["id"] = it.key();  // ID'yi note içine ekle
                newNotes.append(note);
            }

            personalNotes["notes"] = newNotes;
            personalNotes["_meta"] = QJsonObject{
                {"desc", "Kişisel notlarınız ve hızlı erişim"},
                {"total_notes", newNotes.size()},
                {"created", QDateTime::currentDateTime().toString(Qt::ISODate)}
            };

            personalNotesData["Personal Notlar"] = personalNotes;
            savePersonalNotes();  // Yeni formatı kaydet
        }

        int noteCount = personalNotesData["Personal Notlar"].toObject()["notes"].toArray().size();
        qDebug() << "[PersonalNotes] Notlar başarıyla yüklendi. Toplam not:" << noteCount;
    }
}

// Personal Notes'u diske kaydet
void MainWindow::savePersonalNotes()
{
    QFile file(personalNotesPath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[PersonalNotes] Dosya yazılamadı:" << file.errorString();
        QMessageBox::warning(this, "Kayıt Hatası",
                             "Personal notlar kaydedilemedi:\n" + file.errorString());
        return;
    }

    // JSON'a dönüştür ve yaz
    QJsonDocument doc(personalNotesData);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[PersonalNotes] Notlar başarıyla kaydedildi";
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
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);

    // Backup klasörü oluştur
    QDir backupDir(appDir.filePath("backups"));
    if (!backupDir.exists()) {
        backupDir.mkpath(".");
    }

    // Mevcut notlar dosyasını oku
    QFile sourceFile(personalNotesPath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qDebug() << "[Backup] Kaynak dosya okunamadı";
        return;
    }

    QByteArray currentData = sourceFile.readAll();
    sourceFile.close();

    // Dosya boyutu kontrolü - En az bir not olmalı
    QJsonDocument doc = QJsonDocument::fromJson(currentData);
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonObject personalNotes = root["Personal Notlar"].toObject();
        QJsonArray notesArray = personalNotes["notes"].toArray();

        if (notesArray.isEmpty()) {
            qDebug() << "[Backup] Hiç not yok, backup atlandı";
            return;
        }
    }

    // SHA256 hash hesapla (güvenli hash algoritması)
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(currentData);
    QString currentHash = hash.result().toHex();

    // Son backup'ın hash'ini kontrol et
    QString hashFilePath = backupDir.filePath("last_backup_hash.txt");
    QFile hashFile(hashFilePath);

    QString lastHash;
    if (hashFile.open(QIODevice::ReadOnly)) {
        lastHash = hashFile.readAll().trimmed();
        hashFile.close();
    }

    // Hash değişmişse yeni backup al
    if (currentHash != lastHash) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupFilePath = backupDir.filePath("personal_notes_" + timestamp + ".json");

        QFile backupFile(backupFilePath);
        if (backupFile.open(QIODevice::WriteOnly)) {
            backupFile.write(currentData);
            backupFile.close();

            // Hash'i kaydet
            if (hashFile.open(QIODevice::WriteOnly)) {
                hashFile.write(currentHash.toUtf8());
                hashFile.close();
            }

            qDebug() << "[Backup] Yedekleme oluşturuldu:" << backupFilePath;
        }
    } else {
        qDebug() << "[Backup] Değişiklik yok, yedekleme atlandı";
    }

    // Eski yedekleri temizle (son 10'u tut)
    QStringList backupFiles = backupDir.entryList(QStringList() << "personal_notes_*.json", QDir::Files, QDir::Time);
    while (backupFiles.size() > 10) {
        QString oldestBackup = backupFiles.takeLast();
        backupDir.remove(oldestBackup);
        qDebug() << "[Backup] Eski yedek silindi:" << oldestBackup;
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

                // Aktif dosyaya yaz
                QFile activeFile(personalNotesPath);
                if (activeFile.open(QIODevice::WriteOnly)) {
                    activeFile.write(backupData);
                    activeFile.close();

                    // Notları yeniden yükle
                    loadPersonalNotes();
                    mergePersonalNotesIntoMenu();

                    QMessageBox::information(backupDialog, "Başarılı", "Backup başarıyla geri yüklendi!");
                    backupDialog->accept();
                } else {
                    QMessageBox::critical(backupDialog, "Hata", "Notlar dosyası yazılamadı!");
                }
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

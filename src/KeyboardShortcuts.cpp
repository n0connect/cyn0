#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>
#include <QCryptographicHash>
#include <QMenu>
#include <QMessageBox>

// Klavye kısayollarını ayarla
void MainWindow::setupKeyboardShortcuts()
{
    // Ctrl+N - Not menüsü (Yeni not veya Backup yönetimi)
    QShortcut *newNoteShortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(newNoteShortcut, &QShortcut::activated, this, [this]() {
        qDebug() << "[Shortcut] Ctrl+N - Not menüsü açılıyor";

        // Menü oluştur
        QMenu *noteMenu = new QMenu(this);
        noteMenu->setStyleSheet(
            "QMenu { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; padding: 4px; color: #FFFFFF; }"
            "QMenu::item { padding: 8px 16px; border-radius: 6px; margin: 2px; }"
            "QMenu::item:selected { background-color: #7C3AED; color: #FFFFFF; }"
        );

        QAction *newNoteAction = noteMenu->addAction("📝 Yeni Not Oluştur");
        QAction *backupAction = noteMenu->addAction("🗂️ Backup Yöneticisi");

        QAction *selectedAction = noteMenu->exec(QCursor::pos());

        if (selectedAction == newNoteAction) {
            // Yeni not oluştur
            qDebug() << "[Shortcut] Yeni not oluşturma seçildi";
            PersonalNotesDialog *dialog = new PersonalNotesDialog(this);

        connect(dialog, &PersonalNotesDialog::noteSaved, this,
                [this](const QString &title, const QString &content, const QString &tags, bool isFavorite, const QString &) {
            // Notu ekle
            addPersonalNote(title, content, tags, isFavorite);

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
            }
        });

            dialog->exec();
            dialog->deleteLater();
        } else if (selectedAction == backupAction) {
            // Backup yöneticisi aç
            qDebug() << "[Shortcut] Backup yöneticisi seçildi";
            showBackupManager();
        }

        noteMenu->deleteLater();
    });

    // Ctrl+Shift+N - Hızlı not oluştur
    QShortcut *quickNoteShortcut = new QShortcut(QKeySequence("Ctrl+Shift+N"), this);
    connect(quickNoteShortcut, &QShortcut::activated, this, &MainWindow::showQuickNoteDialog);

    // ESC ile Not oluşturma penceresini gizle gizle
    QShortcut *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (isVisibleFlag && ui->lineEdit->text().isEmpty() && !m_isShowingContent) {
            toggleVisibility();
        }
    });

    qDebug() << "[Shortcuts] Klavye kısayolları ayarlandı: Ctrl+N, Ctrl+Shift+N, ESC";
}

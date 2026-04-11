#include "PersonalNotes.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QShortcut>
#include <QMouseEvent>
#include <QJsonObject>
#include <QJsonArray>

// ============= PersonalNotesDialog (Detaylı Not) =============

PersonalNotesDialog::PersonalNotesDialog(QWidget *parent, const QString &id, bool isEdit)
    : QDialog(parent), noteId(id), editMode(isEdit)
{
    setWindowTitle(editMode ? "Notu Düzenle" : "Yeni Not Ekle");
    setMinimumSize(520, 480);

    // Frame'li pencere - daha temiz görünüm
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    // setAttribute(Qt::WA_TranslucentBackground); // KALDIRILDI - transparan sorunu çözüldü

    // Ana layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Dialog için stil - Arka plan dahil
    setStyleSheet(
        "PersonalNotesDialog {"
        "   background-color: #2b2b2b;"
        "}"
        "QLabel {"
        "   color: #e0e0e0;"
        "   font-size: 12px;"
        "}"
        "QLineEdit {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 5px;"
        "}"
        "QTextEdit {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 5px;"
        "}"
        "QCheckBox {"
        "   color: #e0e0e0;"
        "   spacing: 5px;"
        "}"
        "QPushButton {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #4a4a4a;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #2a2a2a;"
        "}"
    );

    // Başlık label ve edit
    QLabel *titleLabel = new QLabel("Not Başlığı (Otomatik oluşturulur, değiştirilebilir):", this);
    titleEdit = new QLineEdit(this);
    titleEdit->setPlaceholderText("Otomatik oluşturulur, dilersen değiştirebilirsin");

    // İçerik label ve edit
    QLabel *contentLabel = new QLabel("Not İçeriği:", this);
    contentEdit = new QTextEdit(this);
    contentEdit->setPlaceholderText("Not içeriğini buraya yazın...\n\nİpucu: Kod parçacıkları, komutlar veya önemli bilgileri buraya kaydedebilirsiniz.");

    // Etiketler (Tags)
    QLabel *tagsLabel = new QLabel("Etiketler (virgülle ayırın):", this);
    tagsEdit = new QLineEdit(this);
    tagsEdit->setPlaceholderText("örnek: güvenlik, linux, exploit");

    // Favori checkbox
    favoriteCheckbox = new QCheckBox("⭐ Favorilere Ekle", this);
    favoriteCheckbox->setStyleSheet("QCheckBox { color: #e0e0e0; font-size: 12px; }");

    // Mevcut Notlar Listesi (sadece yeni not modunda göster)
    noteListWidget = nullptr;
    QLabel *notesListLabel = nullptr;
    if (!editMode) {
        notesListLabel = new QLabel("Mevcut Notlar (Düzenlemek/Silmek için sağ tıkla):", this);
        noteListWidget = new QListWidget(this);
        noteListWidget->setMaximumHeight(150);
        noteListWidget->setStyleSheet(
            "QListWidget {"
            "   background-color: #3a3a3a;"
            "   color: #e0e0e0;"
            "   border: 1px solid #555;"
            "   border-radius: 4px;"
            "}"
            "QListWidget::item {"
            "   padding: 5px;"
            "}"
            "QListWidget::item:selected {"
            "   background-color: #4a4a4a;"
            "}"
            "QListWidget::item:hover {"
            "   background-color: #454545;"
            "}"
        );
        noteListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(noteListWidget, &QListWidget::itemDoubleClicked, this, &PersonalNotesDialog::onNoteListItemClicked);
        connect(noteListWidget, &QListWidget::customContextMenuRequested, this, &PersonalNotesDialog::showNoteContextMenu);
    }

    // Butonlar
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton(editMode ? "Güncelle" : "Kaydet", this);
    cancelButton = new QPushButton("İptal", this);

    saveButton->setDefault(true);
    saveButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border-radius: 4px; }");
    cancelButton->setStyleSheet("QPushButton { padding: 8px 16px; border-radius: 4px; }");

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(saveButton);

    // Layout'lara ekle
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(titleEdit);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(contentLabel);
    mainLayout->addWidget(contentEdit);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(tagsLabel);
    mainLayout->addWidget(tagsEdit);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(favoriteCheckbox);
    mainLayout->addSpacing(10);

    // Mevcut notlar listesini ekle (sadece yeni not modunda)
    if (!editMode && notesListLabel && noteListWidget) {
        mainLayout->addWidget(notesListLabel);
        mainLayout->addWidget(noteListWidget);
        mainLayout->addSpacing(10);
        // Mevcut notları yükle
        loadExistingNotes();
    }

    mainLayout->addLayout(buttonLayout);

    // Bağlantılar
    connect(saveButton, &QPushButton::clicked, this, &PersonalNotesDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &PersonalNotesDialog::onCancelClicked);
    
    // İçerik değiştiğinde başlığı otomatik güncelle
    connect(contentEdit, &QTextEdit::textChanged, this, &PersonalNotesDialog::updateTitleFromContent);

    // Focus'u içerik alanına ver
    contentEdit->setFocus();
}

PersonalNotesDialog::~PersonalNotesDialog()
{
}

QString PersonalNotesDialog::getNoteTitle() const
{
    return titleEdit->text().trimmed();
}

QString PersonalNotesDialog::getNoteContent() const
{
    return contentEdit->toPlainText().trimmed();
}

QString PersonalNotesDialog::getNoteTags() const
{
    return tagsEdit->text().trimmed();
}

bool PersonalNotesDialog::getIsFavorite() const
{
    return favoriteCheckbox->isChecked();
}

QString PersonalNotesDialog::getNoteId() const
{
    return noteId;
}

void PersonalNotesDialog::setNoteTitle(const QString &title)
{
    titleEdit->setText(title);
}

void PersonalNotesDialog::setNoteContent(const QString &content)
{
    contentEdit->setPlainText(content);
}

void PersonalNotesDialog::setNoteTags(const QString &tags)
{
    tagsEdit->setText(tags);
}

void PersonalNotesDialog::setIsFavorite(bool favorite)
{
    favoriteCheckbox->setChecked(favorite);
}

void PersonalNotesDialog::onSaveClicked()
{
    QString content = getNoteContent();

    // Boş veya sadece boşluk karakterlerinden oluşan notları reddet
    if (content.isEmpty() || content.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Uyarı", "Lütfen not içeriği girin!\n\nBoş notlar kaydedilmez.");
        contentEdit->setFocus();
        return;
    }

    QString title = getNoteTitle();

    // Başlık boş bırakılırsa ilk iki kelimeden güvenli bir varsayılan üret.
    QStringList words = content.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (title.isEmpty() && words.size() >= 2) {
        title = words[0] + " " + words[1];
    } else if (title.isEmpty() && words.size() == 1) {
        title = words[0];
    } else if (title.isEmpty()) {
        title = "Yeni Not";
    }

    QString tags = getNoteTags();
    bool isFavorite = getIsFavorite();

    emit noteSaved(title, content, tags, isFavorite, noteId);
    accept();
}

void PersonalNotesDialog::onCancelClicked()
{
    reject();
}

void PersonalNotesDialog::updateTitleFromContent()
{
    if (editMode) {
        return;
    }

    QString content = contentEdit->toPlainText().trimmed();

    if (!content.isEmpty()) {
        // Başlık olarak ilk iki kelimeyi al
        QStringList words = content.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QString title;
        if (words.size() >= 2) {
            title = words[0] + " " + words[1];
        } else if (words.size() == 1) {
            title = words[0];
        } else {
            title = "Yeni Not";
        }

        const QString currentTitle = titleEdit->text().trimmed();
        if (currentTitle.isEmpty() || currentTitle == lastAutoTitle) {
            titleEdit->setText(title);
            lastAutoTitle = title;
        }
    } else if (titleEdit->text().trimmed() == lastAutoTitle) {
        titleEdit->setText("");
        lastAutoTitle.clear();
    }
}

void PersonalNotesDialog::onNoteListItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    // Liste öğesine tıklandığında yapılacak işlemler
    // Not ID'sini UserRole'dan al
    QString clickedNoteId = item->data(Qt::UserRole).toString();

    // Parent MainWindow'a erişim
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent());
    if (!mainWindow) {
        qDebug() << "[PersonalNotesDialog] Parent MainWindow'a erişilemedi";
        return;
    }

    // Düzenleme dialog'unu aç
    accept(); // Mevcut dialogu kapat
    mainWindow->showEditNoteDialog(clickedNoteId);
}

void PersonalNotesDialog::showNoteContextMenu(const QPoint &pos)
{
    if (!noteListWidget) return;

    QListWidgetItem *item = noteListWidget->itemAt(pos);
    if (!item) return;

    // Not bilgilerini al
    QString noteIdToManage = item->data(Qt::UserRole).toString();
    bool isFavorite = item->data(Qt::UserRole + 1).toBool();

    // Parent MainWindow'a erişim
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent());
    if (!mainWindow) {
        qDebug() << "[PersonalNotesDialog] Parent MainWindow'a erişilemedi";
        return;
    }

    QMenu contextMenu(this);
    contextMenu.setStyleSheet(
        "QMenu {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #4a4a4a;"
        "}"
    );

    QAction *editAction = contextMenu.addAction("✏️ Düzenle");
    QAction *favoriteAction = contextMenu.addAction(isFavorite ? "⭐ Favorilerden Çıkar" : "⭐ Favorilere Ekle");
    contextMenu.addSeparator();
    QAction *deleteAction = contextMenu.addAction("🗑️ Sil");

    QAction *selectedAction = contextMenu.exec(noteListWidget->mapToGlobal(pos));

    if (selectedAction == editAction) {
        onNoteListItemClicked(item);
    }
    else if (selectedAction == favoriteAction) {
        // Favorileme/çıkarma işlemi
        // Notu bul ve favorite durumunu değiştir
        QJsonObject personalNotes = mainWindow->personalNotesData["Personal Notlar"].toObject();
        QJsonArray notes = personalNotes["notes"].toArray();

        for (int i = 0; i < notes.size(); ++i) {
            QJsonObject note = notes[i].toObject();
            if (note["id"].toString() == noteIdToManage) {
                QString title = note["title"].toString();
                QString content = note["content"].toString();
                QString tags = note["tags"].toString();
                bool newFavoriteStatus = !isFavorite;

                // MainWindow fonksiyonunu çağır
                mainWindow->updatePersonalNote(noteIdToManage, title, content, tags, newFavoriteStatus);

                // Listeyi yenile
                loadExistingNotes();
                break;
            }
        }
    }
    else if (selectedAction == deleteAction) {
        // Silme onayı
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Notu Sil",
                                     "Bu notu silmek istediğinizden emin misiniz?",
                                     QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            // MainWindow fonksiyonunu çağır
            mainWindow->deletePersonalNote(noteIdToManage);
            // Listeyi yenile
            loadExistingNotes();
        }
    }
}

void PersonalNotesDialog::loadExistingNotes()
{
    if (!noteListWidget) return;

    // Parent MainWindow'a erişim
    MainWindow *mainWindow = qobject_cast<MainWindow*>(parent());
    if (!mainWindow) {
        qDebug() << "[PersonalNotesDialog] Parent MainWindow'a erişilemedi";
        return;
    }

    // Mevcut notları yükle
    noteListWidget->clear();

    QJsonObject personalNotes = mainWindow->personalNotesData["Personal Notlar"].toObject();
    QJsonArray notes = personalNotes["notes"].toArray();

    qDebug() << "[PersonalNotesDialog] Toplam not sayısı:" << notes.size();

    // Notları listele
    for (int i = 0; i < notes.size(); ++i) {
        QJsonObject note = notes[i].toObject();
        QString noteTitle = note["title"].toString();
        QString noteId = note["id"].toString();
        bool isFavorite = note["favorite"].toBool();
        QString created = note["created"].toString().left(10);

        // Liste item oluştur
        QString displayText = (isFavorite ? "⭐ " : "📝 ") + noteTitle + " (" + created + ")";
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, noteId);  // Not ID'sini sakla
        item->setData(Qt::UserRole + 1, isFavorite);  // Favorite durumunu sakla

        noteListWidget->addItem(item);
    }

    qDebug() << "[PersonalNotesDialog] Notlar yüklendi";
}

// ============= QuickNoteDialog (Hızlı Not) =============

QuickNoteDialog::QuickNoteDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Hızlı Not Ekle");
    setMinimumSize(400, 200);

    // Frame'li pencere
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);

    // Ana layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Dialog için stil
    setStyleSheet(
        "QuickNoteDialog {"
        "   background-color: #2b2b2b;"
        "}"
        "QTextEdit {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "   font-size: 13px;"
        "}"
        "QPushButton {"
        "   background-color: #3a3a3a;"
        "   color: #e0e0e0;"
        "   border: 1px solid #555;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #4a4a4a;"
        "}"
    );

    // İçerik edit
    contentEdit = new QTextEdit(this);
    contentEdit->setPlaceholderText("Hızlı notunuzu buraya yazın...\n\nCtrl+Enter ile kaydedin!");

    // Clipboard'dan yapıştır önerisi
    QClipboard *clipboard = QApplication::clipboard();
    QString clipboardText = clipboard->text().trimmed();
    if (!clipboardText.isEmpty() && clipboardText.length() < 500) {
        contentEdit->setPlainText(clipboardText);
        contentEdit->selectAll();
    }

    // Butonlar
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Kaydet (Ctrl+Enter)", this);
    cancelButton = new QPushButton("İptal (Esc)", this);

    saveButton->setDefault(true);
    saveButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 6px 12px; border-radius: 4px; }");
    cancelButton->setStyleSheet("QPushButton { padding: 6px 12px; border-radius: 4px; }");

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(saveButton);

    // Layout'lara ekle
    mainLayout->addWidget(contentEdit);
    mainLayout->addLayout(buttonLayout);

    // Bağlantılar
    connect(saveButton, &QPushButton::clicked, this, &QuickNoteDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Ctrl+Enter ile kaydet
    QShortcut *saveShortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(saveShortcut, &QShortcut::activated, this, &QuickNoteDialog::onSaveClicked);

    contentEdit->setFocus();
}

QuickNoteDialog::~QuickNoteDialog()
{
}

QString QuickNoteDialog::getNoteContent() const
{
    return contentEdit->toPlainText().trimmed();
}

void QuickNoteDialog::onSaveClicked()
{
    QString content = getNoteContent();

    // Boş veya sadece boşluk karakterlerinden oluşan notları reddet
    if (content.isEmpty() || content.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Uyarı", "Lütfen not içeriği girin!\n\nBoş notlar kaydedilmez.");
        contentEdit->setFocus();
        return;
    }

    emit quickNoteSaved(content);
    accept();
}

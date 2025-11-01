#ifndef PERSONALNOTES_H
#define PERSONALNOTES_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>

class PersonalNotesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PersonalNotesDialog(QWidget *parent = nullptr, const QString &noteId = "", bool isEditMode = false);
    ~PersonalNotesDialog();

    // Not bilgilerini al/set et
    QString getNoteTitle() const;
    QString getNoteContent() const;
    QString getNoteTags() const;
    bool getIsFavorite() const;
    QString getNoteId() const;

    void setNoteTitle(const QString &title);
    void setNoteContent(const QString &content);
    void setNoteTags(const QString &tags);
    void setIsFavorite(bool favorite);

signals:
    void noteSaved(const QString &title, const QString &content, const QString &tags, bool isFavorite, const QString &noteId);

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void updateTitleFromContent();
    void onNoteListItemClicked(QListWidgetItem *item);
    void showNoteContextMenu(const QPoint &pos);
    void loadExistingNotes();

private:
    QLineEdit *titleEdit;
    QTextEdit *contentEdit;
    QLineEdit *tagsEdit;
    QCheckBox *favoriteCheckbox;
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QListWidget *noteListWidget;
    QString noteId;
    bool editMode;
};

// Hızlı not dialog'u (Tray'den açılır)
class QuickNoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuickNoteDialog(QWidget *parent = nullptr);
    ~QuickNoteDialog();

    QString getNoteContent() const;

signals:
    void quickNoteSaved(const QString &content);

private slots:
    void onSaveClicked();

private:
    QTextEdit *contentEdit;
    QPushButton *saveButton;
    QPushButton *cancelButton;
};

#endif // PERSONALNOTES_H

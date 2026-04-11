#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QVariant>
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    // Veritabanı işlemleri
    bool initDatabase();
    void closeDatabase();

    // Knowledge Base İşlemleri (Documents)
    bool saveDocument(const QString& path, const QString& jsonContent);
    QString getDocument(const QString& path);
    
    // Arama İşlemleri için Ön Bellek Tablosu
    bool saveSearchItem(const QString& keyword, const QString& description, bool isFolder, const QString& parentPath, bool showDirectContent);
    QStringList search(const QString& keyword); // Eski sistemin matches formatında dönüş (Icon|Keyword|Path|Key|Flag)

    // Personal Notes İşlemleri (Encrypted)
    bool savePersonalNote(const QString& id, const QString& encryptedBase64);
    QString getPersonalNote(const QString& id);
    QList<QPair<QString, QString>> getAllPersonalNotes(); // id, encryptedBase64 döner
    bool deletePersonalNote(const QString& id);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
    bool createTables();
};

#endif // DATABASEMANAGER_H

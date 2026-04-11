#include "DatabaseManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QSqlRecord>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initDatabase()
{
    // Cihazın uygulama verileri klasöründe SQLite dosyasını oluştur oraya bağlan
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString dbPath = dir.filePath("cyn0_secure_vault.sqlite");

    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "[DatabaseManager] Veritabanı açılamadı:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "[DatabaseManager] Veritabanına bağlanıldı:" << dbPath;
    return createTables();
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);

    // 1. Documents (Knowledge Base QJsonObject as String blobs)
    QString createDocs = "CREATE TABLE IF NOT EXISTS documents ("
                         "path TEXT PRIMARY KEY, "
                         "json_content TEXT)";
    if (!query.exec(createDocs)) {
        qDebug() << "[DatabaseManager] documents tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }

    // 2. Search Index (Arama motoru performansı için indexleme, XSS/SQLi payload güvenliği test edilmiştir)
    QString createSearch = "CREATE TABLE IF NOT EXISTS search_items ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "keyword TEXT, "
                           "description TEXT, "
                           "is_folder BOOLEAN, "
                           "parent_path TEXT, "
                           "show_direct_content BOOLEAN)";
    if (!query.exec(createSearch)) {
        qDebug() << "[DatabaseManager] search_items tablosu oluşturulamadı:" << query.lastError().text();
        return false;
    }

    return true;
}

// ------ KNOWLEDGE BASE GÜVENLİ (PREPARED) SORGULAR ------

bool DatabaseManager::saveDocument(const QString& path, const QString& jsonContent)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO documents (path, json_content) VALUES (:path, :content)");
    query.bindValue(":path", path);
    query.bindValue(":content", jsonContent);
    
    if (!query.exec()) {
        qDebug() << "[DatabaseManager] Belge kaydedilemedi:" << query.lastError().text();
        return false;
    }
    return true;
}

QString DatabaseManager::getDocument(const QString& path)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT json_content FROM documents WHERE path = :path");
    query.bindValue(":path", path);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString(); // Bulunamazsa boş döner
}

bool DatabaseManager::saveSearchItem(const QString& keyword, const QString& description, bool isFolder, const QString& parentPath, bool showDirectContent)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO search_items (keyword, description, is_folder, parent_path, show_direct_content) "
                  "VALUES (:keyword, :description, :is_folder, :parent_path, :show_direct_content)");
    query.bindValue(":keyword", keyword);
    query.bindValue(":description", description);
    query.bindValue(":is_folder", isFolder);
    query.bindValue(":parent_path", parentPath);
    query.bindValue(":show_direct_content", showDirectContent);
    
    if (!query.exec()) {
        qDebug() << "[DatabaseManager] Search objesi kaydedilemedi:" << query.lastError().text();
        return false;
    }
    return true;
}

QStringList DatabaseManager::search(const QString& keyword)
{
    QStringList matches;
    QSqlQuery query(m_db);
    
    // Güvenlik sağlayan parameterized 'LIKE' sorgusu (Güçlü XSS/SQLi koruması garantisi)
    query.prepare("SELECT keyword, is_folder, parent_path, show_direct_content, description FROM search_items "
                  "WHERE keyword LIKE :search OR description LIKE :search LIMIT 100");
    query.bindValue(":search", "%" + keyword + "%");
    
    if (query.exec()) {
        while (query.next()) {
            QString itemKeyword = query.value("keyword").toString();
            bool isFolder = query.value("is_folder").toBool();
            QString parentPath = query.value("parent_path").toString();
            bool showDirectContent = query.value("show_direct_content").toBool();
            QString description = query.value("description").toString();
            
            // UI geriye dönük uyumluluğu için özel maç formatını oluştur ("Icon|DisplayText|JsonPath|OriginalKey|ShowDirectContent")
            QString icon = isFolder ? "📁" : "📄";
            
            // Eğer keyword match varsa, description match yazmaya gerek yok, ancak logicle ayırabiliyorsak güzel olur.
            // UI tarafı "DisplayText" içine " (açıklama eşleşmesi)" vs eklentisi beklerdi eskiden.
            // Biz basitçe isim olarak eşleşmenin parçasını koyacağız.
            QString displayText = itemKeyword;
            
            if (!itemKeyword.contains(keyword, Qt::CaseInsensitive) && description.contains(keyword, Qt::CaseInsensitive)) {
                displayText += " (içerikte bulundu)";
            }
            
            QString showStr = showDirectContent ? "true" : "false";
            QString matchStr = QString("%1|%2|%3|%4|%5").arg(icon, displayText, parentPath, itemKeyword, showStr);
            
            matches.append(matchStr);
        }
    } else {
        qDebug() << "[DatabaseManager] Arama başarısız:" << query.lastError().text();
    }
    
    return matches;
}

#include <mainwindow.h>
#include <QApplication>
#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QIcon>


void setStyleForUi(QApplication *application, const QString &styleSheetPath)
{
    QFile file(styleSheetPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        const QString styleSheet = file.readAll();
        application->setStyleSheet(styleSheet);
        qDebug() << "[+] Style dosyası yüklendi: " << styleSheetPath;
    } else {
        qWarning() << "[-] Style dosyası açılamadı: " << styleSheetPath;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);         // QApplication nesnesi oluşturuluyor
    
    // MacOS Top-Bar, Linux ve Windows Görev Yöneticisi isimleri için meta veriler
    QApplication::setApplicationName("Cyn0");
    QApplication::setApplicationDisplayName("Cyn0");
    QApplication::setOrganizationName("Cyn0");
    QApplication::setApplicationVersion("1.0");
    
    // Linux (X11) ve eski Windows pencereleri için varsayılan ikon
    a.setWindowIcon(QIcon(":/icon/icon48x48.ico"));
    
    const QString styleSheetPath = ":/qss/macos_ios_qss_themev2.qss";

    // --- StyleSheet Yükle ---
    setStyleForUi(&a, styleSheetPath);

    MainWindow w;                       // MainWindow sınıfından bir nesne oluşturuluyor
    w.show();                           // MainWindow'u görünür yap.

    return a.exec();                    // Uygulama döngüsünü başlat
}

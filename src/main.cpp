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
    const QString styleSheetPath = ":/qss/macos_ios_qss_themev2.qss";

    // --- StyleSheet Yükle ---
    setStyleForUi(&a, styleSheetPath);

    MainWindow w;                       // MainWindow sınıfından bir nesne oluşturuluyor
    w.show();                           // MainWindow'u görünür yap.

    return a.exec();                    // Uygulama döngüsünü başlat
}

#include <mainwindow.h>
#include <Animations.h>
#include <QObject>
#include <QPropertyAnimation>
#include <QDebug>

//Program baslangicinda calisacak olan UI efekti
void fadeInOnStart(QMainWindow* window) {
    QPropertyAnimation* anim = new QPropertyAnimation(window, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Fade Animasyon ile UI kontrolu
void toggleVisibilityAnimation(QMainWindow* window, bool isVisible) {
    QPropertyAnimation* animation = new QPropertyAnimation(window, "windowOpacity");
    animation->setDuration(300);
    qDebug() << "Window Opacity(Now Active): " << window->windowOpacity();
    if (isVisible) { // True ise hide()
        animation->setStartValue(window->windowOpacity());
        animation->setEndValue(0.0);
        QObject::connect(animation, &QPropertyAnimation::finished, [window]() {
            window->hide();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        qDebug() << "Window Opacity(Now Hidden): " << window->windowOpacity();

    } else {        // False ise show()
        qDebug() << "Window Opacity(Now Hidden): " << window->windowOpacity();

        window->showNormal();           // Görünür yap ve eğer minimize olduysa normale getir
        window->activateWindow();       // Uygulamayı ön plana al
        window->raise();                // Diğer pencerelerin üstüne getir

        //window->ui->lineEdit->setFocus();

        animation->setStartValue(window->windowOpacity());
        animation->setEndValue(1.0);
        QObject::connect(animation, &QPropertyAnimation::finished, [window]() {
            window->show();
            window->activateWindow();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        qDebug() << "Window Opacity(Now Active): " << window->windowOpacity();
    }
}

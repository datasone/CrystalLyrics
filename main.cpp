#include "mainwindow.h"
#include "window.h"

#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();

    QFont font("Times");

    Window window;
    window.show();

    return QApplication::exec();
}

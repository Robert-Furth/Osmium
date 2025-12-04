#include "mainwindow.h"

#include <iostream>

#include <QApplication>
#include <QStyle>
#include <QStyleFactory>

#include <osmium.h>

int main(int argc, char *argv[]) {
    if (!osmium::init()) {
        std::cout << "Could not start osmium library\n";
        return 1;
    }

    QApplication a(argc, argv);
    a.setApplicationName("Osmium");

    // The windows11 style is hideous; override it here
    a.setStyle("Fusion");

    a.connect(&a, &QApplication::aboutToQuit, osmium::uninit);

    MainWindow w;
    w.show();
    return a.exec();
}

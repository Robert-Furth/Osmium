#include "mainwindow.h"

#include <iostream>

#include <QApplication>
#include <QStyle>
#include <QStyleFactory>

#include "osmium/osmium.h"

int main(int argc, char *argv[])
{
    if (!osmium::init()) {
        std::cout << "Could not start osmium\n";
        return 1;
    }

    QApplication a(argc, argv);

    // The windows11 style is hideous; override it here
    a.setStyle("Fusion");

    MainWindow w;
    w.show();
    int status_code = a.exec();

    osmium::uninit();
    return status_code;
}

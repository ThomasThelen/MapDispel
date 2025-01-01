#include "mapdispel.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MapDispel w;
    w.show();
    return a.exec();
}



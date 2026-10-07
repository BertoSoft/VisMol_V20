#include "moleculavista.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MoleculaVista w;
    w.showMaximized();
    return QApplication::exec();
}

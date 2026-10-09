#include "moleculavista.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MoleculaVista* vista    = new MoleculaVista();
    MoleculaView* view      = new MoleculaView();

    vista->setMoleculaView(view);
    vista->showMaximized();

    return QApplication::exec();
}

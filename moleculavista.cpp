#include "moleculavista.h"

#include <QMenu>
#include <QMenuBar>

MoleculaVista::MoleculaVista(QWidget *parent)
    : QMainWindow(parent){

    initUi();

}

MoleculaVista::~MoleculaVista(){

}

void MoleculaVista::initUi(){
    setWindowTitle(NOMBRE_APP);
    initMenu();
}

void MoleculaVista::initMenu(){
    QMenuBar *menuBarSuperior = menuBar();

    // Menu Archivo
    QMenu *menuArchivo = menuBarSuperior->addMenu("&Archivo");
    menuArchivo->addAction("Nuevo Proyecto");
    menuArchivo->addAction("Abrir Proyecto (.json)");
    menuArchivo->addAction("Guardar Proyecto (.json)");
    menuArchivo->addSeparator();
    menuArchivo->addAction("Salir");

    // Menú Importar / Exportar (Aislado para escalabilidad)
    QMenu* menuFormatos = menuBarSuperior->addMenu("&Importar/Exportar");
    menuFormatos->addAction("Importar Coordenadas XYZ (.xyz)");
    menuFormatos->addAction("Exportar Entrada MOPAC (.mop)");

    // Menú Edición
    QMenu* menuEdicion = menuBarSuperior->addMenu("&Edición");
    menuEdicion->addAction("Deshacer (Ctrl+Z)");
    menuEdicion->addAction("Rehacer (Ctrl+Y)");
    menuEdicion->addSeparator();
    menuEdicion->addAction("Limpiar Lienzo");

    // Menú Cálculo
    QMenu* menuCalculo = menuBarSuperior->addMenu("&Cálculo");
    menuCalculo->addAction("Configurar Simulación MOPAC...");

    // Menú Análisis
    QMenu* menuAnalisis = menuBarSuperior->addMenu("&Análisis");
    menuAnalisis->addAction("Mostrar Analizador Vibracional");
    menuAnalisis->addAction("Ver Log de Salida MOPAC (.out)");
}
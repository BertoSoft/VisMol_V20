#include "moleculalienzo.h"

#include <QObject>

MoleculaLienzo::MoleculaLienzo(QWidget* parent)
    : QGraphicsView(parent){

    // 1. Instanciamos el espacio de dibujo infinito en memoria
    m_escena = new QGraphicsScene(this);

    // 2. Asociamos de forma invisible la escena a este visor gráfico
    this->setScene(m_escena);

    // 3. Opciones estéticas profesionales para un renderizado de átomos nítido
    this->setRenderHint(QPainter::Antialiasing); // Suavizado de bordes para los círculos
}

MoleculaLienzo::~MoleculaLienzo(){

}

void MoleculaLienzo::setMoleculaView(MoleculaView* view){
    if(!view) return;

    m_view = view;
}

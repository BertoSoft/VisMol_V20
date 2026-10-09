#include "moleculalienzo.h"
#include "moleculaview.h"

#include <QObject>
#include <QMouseEvent>

MoleculaLienzo::MoleculaLienzo(QWidget* parent)
    : QGraphicsView(parent){

    m_escena = new QGraphicsScene(this);

    // =========================================================================
    // CONFIGURACIÓN DE ESCALA CIENTÍFICA (1 Unidad = 1 Ángstrom)
    // Definimos un lienzo de 1000x1000 Å con el origen (0,0) en el centro
    // =========================================================================
    m_escena->setSceneRect(-500, -500, 1000, 1000);

    this->setScene(m_escena);

    // Opciones estéticas profesionales para un renderizado de átomos nítido
    this->setRenderHint(QPainter::Antialiasing);

    // Evita que aparezcan barras de scroll molestas si nos acercamos a los bordes
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

MoleculaLienzo::~MoleculaLienzo(){

}

//###########################################################################################
// Funciones Publicas
//#########################################################################################

void MoleculaLienzo::setMoleculaView(MoleculaView* view){
    if(!view) return;

    m_view = view;
}

void MoleculaLienzo::actualizarLienzo(){
    if(!m_view) return;

    QVector<Atomo> listaAtomos      = m_view->getListaAtomos();
    QVector<Enlace> listaEnlaces    = m_view->getListaEnlaces();

    m_escena->clear();

    // Renderizamos los enlaces



    // Renderizamos los atomos



}

//##############################################################################################
// Funciones Protegidas SObreescritas
//#############################################################################################

void MoleculaLienzo::mousePressEvent(QMouseEvent* mouseEv){
    if(!m_view){
        return;
    }

    if(mouseEv->button() == Qt::LeftButton){
        QPointF     posClick = this->mapToScene(mouseEv->pos());
        QVector3D   vector3D = QVector3D(posClick.x(), posClick.y(), 0.0f);

        m_view->setClick(vector3D);
    }

    QGraphicsView::mousePressEvent(mouseEv);
}

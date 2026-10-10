#include "moleculalienzo.h"
#include "moleculaview.h"

#include <QObject>
#include <QMouseEvent>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QColor>
#include <QGraphicsSimpleTextItem>

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

    // =========================================================================
    // 1. RENDERIZAMOS LOS ENLACES (LÍNEAS)
    // ==========================================================================
    QPen penEnlaces(QColor("#aaaaaa"));
    penEnlaces.setWidthF(0.15f);
    penEnlaces.setCapStyle(Qt::RoundCap);

    for(const Enlace &enlace: listaEnlaces){
        QVector3D   posOrigen;
        QVector3D   posDestino;
        bool        origenEncontrado    = false;
        bool        destinoEncontrado   = false;

        for(const Atomo &atomo: listaAtomos){
            if(atomo.id == enlace.id_atomo1){
                posOrigen           = atomo.posicion;
                origenEncontrado    = true;
            }
            if(atomo.id == enlace.id_atomo2){
                posDestino          = atomo.posicion;
                destinoEncontrado   = true;
            }
            if(origenEncontrado && destinoEncontrado){
                break;
            }
        }
        if(origenEncontrado && destinoEncontrado){
            QGraphicsLineItem* linea = m_escena->addLine(
                posOrigen.x(),
                posOrigen.y(),
                posDestino.x(),
                posDestino.y(),
                penEnlaces
                );
            linea->setZValue(0);
        }
    }

    // =========================================================================
    // 1. RENDERIZAMOS LOS ATOMOS (ELIPSES)
    // ==========================================================================

    for(const Atomo &atomo: listaAtomos){
        qreal   radio   = getRadioFromAtomo(atomo);
        QColor  color   = getColorFromAtomo(atomo);

        // QGraphicsEllipseItem se dibuja desde la esquina superior izquierda de su contenedor rectangular.
        // Restamos el radio a la posición (x, y) para centrar el átomo en su coordenada exacta.
        qreal x         = atomo.posicion.x() - radio;
        qreal y         = atomo.posicion.y() - radio;
        qreal diametro  = radio * 2.0f;

        QGraphicsEllipseItem* elipse = m_escena->addEllipse(
            x,
            y,
            diametro,
            diametro
            );

        elipse->setBrush(QColor(color));
        elipse->setPen(QPen(Qt::black, 0.05f));
        elipse->setZValue(1);

        QGraphicsSimpleTextItem* texto = m_escena->addSimpleText(atomo.simbolo);
        QRectF contornoTexto = texto->boundingRect();
        texto->setPos(
            atomo.posicion.x() - (contornoTexto.width() / 2),
            atomo.posicion.y() - (contornoTexto.height() / 2)
            );
        texto->setZValue(2);
    }

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

//##############################################################################################
// Funciones Privadas
//#############################################################################################

qreal MoleculaLienzo::getRadioFromAtomo(const Atomo& atomo){
    // Proporciones basadas en radios atómicos relativos (escalados para pantalla)
    if (atomo.simbolo == "C") return 36; // Carbono estándar
    if (atomo.simbolo == "H") return 24; // Hidrógeno (más pequeño)
    if (atomo.simbolo == "O") return 32; // Oxígeno
    if (atomo.simbolo == "N") return 34; // Nitrógeno
    return 15;
}

QColor MoleculaLienzo::getColorFromAtomo(const Atomo& atomo){
    if (atomo.simbolo == "H")  return Qt::white;
    if (atomo.simbolo == "C")  return Qt::darkGray;
    if (atomo.simbolo == "O")  return Qt::red;
    if (atomo.simbolo == "N")  return Qt::blue;
    if (atomo.simbolo == "S")  return Qt::yellow;
    if (atomo.simbolo == "P")  return QColor(255, 165, 0); // Naranja
    if (atomo.simbolo == "F" || atomo.simbolo == "Cl") return Qt::green;
    return Qt::magenta; // Color por defecto para elementos no registrados
}
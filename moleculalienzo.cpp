#include "moleculalienzo.h"
#include "moleculaview.h"

#include <QObject>
#include <QMouseEvent>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QColor>
#include <QGraphicsSimpleTextItem>
#include <QtMath> // Requerido para qSqrt()

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
    // Un gris claro suave (#D1D5DB) es mucho menos agresivo que el blanco puro sobre fondo oscuro
    QPen penEnlaces(QColor("#D1D5DB"));
    penEnlaces.setWidthF(3.0f);
    penEnlaces.setCapStyle(Qt::RoundCap);

    // Distancia de separación física en píxeles/unidades entre las líneas paralelas
    const qreal offsetDist = 8.0;

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
            // Coordenadas base
            qreal x1 = posOrigen.x();
            qreal y1 = posOrigen.y();
            qreal x2 = posDestino.x();
            qreal y2 = posDestino.y();

            // Calculamos el vector de dirección del enlace (Dx, Dy)
            qreal dx = x2 - x1;
            qreal dy = y2 - y1;
            qreal longitud = qSqrt(dx * dx + dy * dy);

            // Evitamos divisiones por cero si los átomos están superpuestos
            if (longitud < 0.001) continue;

            // Vector unitario perpendicular (Normalizado)
            qreal px = -dy / longitud;
            qreal py = dx / longitud;

            // Dibujamos según el orden del enlace químico
            if (enlace.orden == 1) {
                // --- Enlace Simple: Una única línea central ---
                QGraphicsLineItem* linea = m_escena->addLine(x1, y1, x2, y2, penEnlaces);
                linea->setZValue(0);
            }
            else if (enlace.orden == 2) {
                // --- Enlace Doble: Dos líneas desplazadas simétricamente a los lados ---
                qreal ox = px * (offsetDist / 2.0);
                qreal oy = py * (offsetDist / 2.0);

                QGraphicsLineItem* l1 = m_escena->addLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy, penEnlaces);
                QGraphicsLineItem* l2 = m_escena->addLine(x1 - ox, y1 - oy, x2 - ox, y2 - oy, penEnlaces);
                l1->setZValue(0);
                l2->setZValue(0);
            }
            else if (enlace.orden >= 3) {
                // --- Enlace Triple: Una línea central y dos líneas a los extremos exteriores ---
                qreal ox = px * offsetDist;
                qreal oy = py * offsetDist;

                QGraphicsLineItem* l_centro = m_escena->addLine(x1, y1, x2, y2, penEnlaces);
                QGraphicsLineItem* l_izq    = m_escena->addLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy, penEnlaces);
                QGraphicsLineItem* l_der    = m_escena->addLine(x1 - ox, y1 - oy, x2 - ox, y2 - oy, penEnlaces);
                l_centro->setZValue(0);
                l_izq->setZValue(0);
                l_der->setZValue(0);
            }
        }
    }

    // =========================================================================
    // 2. RENDERIZAMOS LOS ATOMOS (ELIPSES Y CONTRASTE DE TEXTO)
    // ==========================================================================

    for(const Atomo &atomo: listaAtomos){
        qreal   radio   = getRadioFromAtomo(atomo);
        QColor  color   = getColorFromAtomo(atomo);

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

        // Bordes oscuros muy sutiles pero definidos para separar el átomo de los enlaces traseros
        elipse->setPen(QPen(QColor("#1F2937"), 1.5f));
        elipse->setZValue(1);

        // =========================================================================
        // ¡NUEVO!: FEEDBACK VISUAL DE ÁTOMO SELECCIONADO (ANILLO CELESTE)
        // =========================================================================
        if (m_view && m_view->getIdAtomoSeleccionado() == atomo.id) {
            qreal radioAnillo = radio + 6.0; // 6 unidades más grande para que rodee el átomo
            qreal xAnillo = atomo.posicion.x() - radioAnillo;
            qreal yAnillo = atomo.posicion.y() - radioAnillo;
            qreal diametroAnillo = radioAnillo * 2.0;

            QGraphicsEllipseItem* anilloSel = m_escena->addEllipse(xAnillo, yAnillo, diametroAnillo, diametroAnillo);

            // Configuración estética: Azul celeste vibrante (#38BDF8) y estilo punteado
            QPen penSel(QColor("#38BDF8"));
            penSel.setWidthF(2.5f);
            penSel.setStyle(Qt::DashLine); // Línea discontinua/punteada muy tecnológica

            anilloSel->setPen(penSel);
            anilloSel->setBrush(Qt::NoBrush); // Interior transparente para que se vea el átomo
            anilloSel->setZValue(3); // Capa superior para que brille por encima de todo
        }

        // Renderizado del Símbolo Químico con tipografía unificada y limpia
        QGraphicsSimpleTextItem* texto = m_escena->addSimpleText(atomo.simbolo);

        QFont fuenteApp("Segoe UI", 12, QFont::Bold);
        fuenteApp.setStyleHint(QFont::SansSerif);
        texto->setFont(fuenteApp);

        // Control dinámico de legibilidad: texto negro para fondos muy claros (H, S, P, Halógenos), blanco para el resto
        if (atomo.simbolo == "H" || atomo.simbolo == "S" || atomo.simbolo == "P" || atomo.simbolo == "F" || atomo.simbolo == "Cl") {
            texto->setBrush(QColor("#111827")); // Gris casi negro
        } else {
            texto->setBrush(QColor("#FFFFFF")); // Blanco puro
        }

        QRectF contornoTexto = texto->boundingRect();
        texto->setPos(
            atomo.posicion.x() - (contornoTexto.width() / 2),
            atomo.posicion.y() - (contornoTexto.height() / 2)
            );
        texto->setZValue(2);
    }

}

//##############################################################################################
// Funciones Protegidas Sobreescritas
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

    if(mouseEv->button() == Qt::RightButton){
        QPointF     posClickDerecho = this->mapToScene(mouseEv->pos());
        QVector3D   vector3D        = QVector3D(posClickDerecho.x(), posClickDerecho.y(), 0.0f);

        m_view->setClickDerecho(vector3D);
    }

    QGraphicsView::mousePressEvent(mouseEv);
}

//##############################################################################################
// Funciones Privadas
//#############################################################################################

qreal MoleculaLienzo::getRadioFromAtomo(const Atomo& atomo){
    if (atomo.simbolo == "C") return 36;
    if (atomo.simbolo == "H") return 24;
    if (atomo.simbolo == "O") return 32;
    if (atomo.simbolo == "N") return 34;
    return 15;
}

QColor MoleculaLienzo::getColorFromAtomo(const Atomo& atomo){
    // Paleta de colores optimizada (Flat / Material Design) adaptada al estándar CPK químico
    if (atomo.simbolo == "H")  return QColor("#F3F4F6"); // Blanco grisáceo limpio
    if (atomo.simbolo == "C")  return QColor("#4B5563"); // Gris grafito profesional (no se confunde con el fondo negro)
    if (atomo.simbolo == "O")  return QColor("#EF4444"); // Rojo coral moderno
    if (atomo.simbolo == "N")  return QColor("#3B82F6"); // Azul cobalto eléctrico
    if (atomo.simbolo == "S")  return QColor("#FBBF24"); // Amarillo azufre pastel
    if (atomo.simbolo == "P")  return QColor("#F97316"); // Naranja brillante
    if (atomo.simbolo == "F" || atomo.simbolo == "Cl") return QColor("#10B981"); // Verde esmeralda claro
    return QColor("#EC4899"); // Magenta/Rosa chicle para elementos no identificados
}
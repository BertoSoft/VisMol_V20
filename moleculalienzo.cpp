#include "moleculalienzo.h"
#include "moleculaview.h"
#include "atomos.h"
#include "moleculamat.h"


#include <QObject>
#include <QMouseEvent>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QColor>
#include <QGraphicsSimpleTextItem>
#include <QtMath> // Requerido para qSqrt()
#include <QScrollArea>
#include <QScrollBar>

MoleculaLienzo::MoleculaLienzo(QWidget* parent)
    : QGraphicsView(parent){

    m_escena = new QGraphicsScene(this);

    // ─── FIJAMOS EL ESPACIO DE TRABAJO CON ORIGEN CENTRAL INAMOVIBLE ───
    m_escena->setSceneRect(-5000, -5000, 10000, 10000);

    this->setScene(m_escena);

    // Opciones estéticas profesionales para un renderizado de átomos nítido
    this->setRenderHint(QPainter::Antialiasing);

    // Evita que aparezcan barras de scroll molestas
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Inicialización explícita de seguridad
    m_anguloX = 0.0f;
    m_anguloY = 0.0f;

    this->setMouseTracking(true);
    this->setDragMode(QGraphicsView::NoDrag);
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

void MoleculaLienzo::resetearVistaFrontal() {
    m_anguloX = 0.0f;
    m_anguloY = 0.0f;
    actualizarLienzo(); // Pinta la escena de frente con los datos reales en RAM de m_view
    // =========================================================================
    // ─── ¡SOLUCIÓN!: CALIBRACIÓN E INYECCIÓN DE FOCO INSTANTÁNEA PARA EL RATÓN ───
    // =========================================================================
    this->updateSceneRect(m_escena->itemsBoundingRect()); // Fuerza a la escena a recalcular sus fronteras reales ya aplanadas
    this->setFocus(); // Obliga al widget a tomar el foco activo del sistema operativo, reseteando la caché del cursor
}

void MoleculaLienzo::actualizarLienzo(){
    if(!m_view) return;

    QVector<Atomo> listaAtomos      = m_view->getListaAtomos();
    QVector<Enlace> listaEnlaces    = m_view->getListaEnlaces();

    m_escena->clear();

    // =========================================================================
    // 1. RENDERIZAMOS LOS ENLACES (LÍNEAS CON ROTACIÓN SELECCIONADA)
    // ==========================================================================
    QPen penEnlaces(QColor("#D1D5DB"));
    penEnlaces.setWidthF(3.0f);
    penEnlaces.setCapStyle(Qt::RoundCap);

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
            // ¡MÁGICO!: Calculamos los extremos del enlace usando tu clase matemática
            QPointF p1 = MoleculaMat::rotarPunto(posOrigen, m_anguloX, m_anguloY);
            QPointF p2 = MoleculaMat::rotarPunto(posDestino, m_anguloX, m_anguloY);

            // Calculamos el vector de dirección basándonos en los nuevos puntos rotados
            qreal dx = p2.x() - p1.x();
            qreal dy = p2.y() - p1.y();
            qreal longitud = qSqrt(dx * dx + dy * dy);

            if (longitud < 0.001) continue;

            // Vector unitario perpendicular adaptado al giro de la pantalla
            qreal px = -dy / longitud;
            qreal py = dx / longitud;

            // Dibujamos utilizando p1 y p2 corregidos por el hardware/QSS matemático
            if (enlace.orden == 1) {
                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
            }
            else if (enlace.orden == 2) {
                qreal ox = px * (offsetDist / 2.0);
                qreal oy = py * (offsetDist / 2.0);

                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
            else if (enlace.orden >= 3) {
                qreal ox = px * offsetDist;
                qreal oy = py * offsetDist;

                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
        }
    }

    // =========================================================================
    // 2. RENDERIZAMOS LOS ATOMOS (ELIPSES ROTADAS Y CONTRASTE DE TEXTO)
    // ==========================================================================
    for(const Atomo &atomo: listaAtomos){
        qreal   radio   = Atomos::getRadio(atomo.simbolo);
        QColor  color   = Atomos::getColor(atomo.simbolo);

        // ¡MÁGICO!: Proyectamos el centro del átomo aplicando los ángulos de la vista
        QPointF posRotada = MoleculaMat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);

        qreal x         = posRotada.x() - radio;
        qreal y         = posRotada.y() - radio;
        qreal diametro  = radio * 2.0f;

        QGraphicsEllipseItem* elipse = m_escena->addEllipse(x, y, diametro, diametro);
        elipse->setBrush(QColor(color));
        elipse->setPen(QPen(QColor("#1F2937"), 1.5f));
        elipse->setZValue(1);

        // Feedback visual adaptado a la coordenada rotada del átomo
        if (m_view && m_view->getIdAtomoSeleccionado() == atomo.id) {
            qreal radioAnillo = radio + 6.0;
            qreal xAnillo = posRotada.x() - radioAnillo;
            qreal yAnillo = posRotada.y() - radioAnillo;
            qreal diametroAnillo = radioAnillo * 2.0;

            QGraphicsEllipseItem* anilloSel = m_escena->addEllipse(xAnillo, yAnillo, diametroAnillo, diametroAnillo);

            QPen penSel(QColor("#38BDF8"));
            penSel.setWidthF(2.5f);
            penSel.setStyle(Qt::DashLine);

            anilloSel->setPen(penSel);
            anilloSel->setBrush(Qt::NoBrush);
            anilloSel->setZValue(3);
        }

        // Renderizado del Símbolo Químico en concordancia con el centro proyectado
        QGraphicsSimpleTextItem* texto = m_escena->addSimpleText(atomo.simbolo);

        QFont fuenteApp("Segoe UI", 12, QFont::Bold);
        fuenteApp.setStyleHint(QFont::SansSerif);
        texto->setFont(fuenteApp);

        if (Atomos::isTextoOscuro(atomo.simbolo)) {
            texto->setBrush(QColor("#111827"));
        } else {
            texto->setBrush(QColor("#FFFFFF"));
        }

        QRectF contornoTexto = texto->boundingRect();
        texto->setPos(
            posRotada.x() - (contornoTexto.width() / 2),
            posRotada.y() - (contornoTexto.height() / 2)
            );
        texto->setZValue(2);
    }

    // ─── ¡NUEVO!: Fuerza el refresco continuo del área gráfica al final del repintado ───
    this->viewport()->update();
}

//##############################################################################################
// Funciones Protegidas Sobreescritas
//#############################################################################################

void MoleculaLienzo::mousePressEvent(QMouseEvent* mouseEv){
    if(!m_view) return;

    m_ultimaPosRat = mouseEv->pos();

    if(m_view->getModoEditor() != MoleculaView::ModoRotacion3D) {
        // Capturamos el punto exacto en píxeles puros de la escena
        QPointF posClick = this->mapToScene(mouseEv->pos());

        // ─── ELIMINADA LA DIVISIÓN: Mandamos píxeles puros directamente al Cerebro ───
        if(mouseEv->button() == Qt::LeftButton){
            m_view->setClick(QVector3D(posClick.x(), posClick.y(), 0.0f));
        }
        if(mouseEv->button() == Qt::RightButton){
            m_view->setClickDerecho(QVector3D(posClick.x(), posClick.y(), 0.0f));
        }
    }
    QGraphicsView::mousePressEvent(mouseEv);
}

void MoleculaLienzo::mouseMoveEvent(QMouseEvent* mouseEv) {
    if(m_view && m_view->getModoEditor() == MoleculaView::ModoRotacion3D && (mouseEv->buttons() & Qt::LeftButton)) {
        int deltaX = mouseEv->pos().x() - m_ultimaPosRat.x();
        int deltaY = mouseEv->pos().y() - m_ultimaPosRat.y();

        m_anguloY += deltaX * 0.5f;
        m_anguloX += deltaY * 0.5f;

        m_anguloX = qMax(-85.0f, qMin(85.0f, m_anguloX));
        m_ultimaPosRat = mouseEv->pos();

        actualizarLienzo();
    } else {
        QGraphicsView::mouseMoveEvent(mouseEv);
    }
}

void MoleculaLienzo::resizeEvent(QResizeEvent* event){
    QGraphicsView::resizeEvent(event);
    this->resetTransform();
    m_escena->update();

    // ─── ANCLAMOS EL FOCO DE LA PANTALLA EN EL CENTRO REAL (0,0) ───
    centerOn(0, 0);
}
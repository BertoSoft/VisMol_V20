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
    // 1. RENDERIZAMOS LOS ENLACES (LÍNEAS CON TRASLACIÓN O ROTACIÓN EXCLUSIVA)
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
            QPointF p1;
            QPointF p2;

            // Maquinaria de decisión exclusiva y limpia: O una o la otra
            if (m_view->getModoEditor() == MoleculaView::ModoSeleccion) {
                // Si desplazamos, la vista está de frente (ángulos = 0). Mover punto directo.
                p1 = MoleculaMat::moverPunto(posOrigen, m_deltaX, m_deltaY);
                p2 = MoleculaMat::moverPunto(posDestino, m_deltaX, m_deltaY);
            } else {
                // Si rotamos, aplicamos tu proyección cuántica 3D estándar de la casa
                p1 = MoleculaMat::rotarPunto(posOrigen, m_anguloX, m_anguloY);
                p2 = MoleculaMat::rotarPunto(posDestino, m_anguloX, m_anguloY);
            }

            // Calculamos el vector de dirección basándonos en los nuevos puntos proyectados
            qreal dx = p2.x() - p1.x();
            qreal dy = p2.y() - p1.y();
            qreal longitud = qSqrt(dx * dx + dy * dy);

            if (longitud < 0.001) continue;

            // Vector unitario perpendicular adaptado al giro de la pantalla
            qreal px = -dy / longitud;
            qreal py = dx / longitud;

            // Dibujamos utilizando p1 y p2 corregidos de forma exclusiva
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
    // 2. RENDERIZAMOS LOS ATOMOS (ELIPSES TRASLADADAS O ROTADAS EXCLUSIVAS)
    // ==========================================================================
    for(const Atomo &atomo: listaAtomos){
        qreal   radio   = Atomos::getRadio(atomo.simbolo);
        QColor  color   = Atomos::getColor(atomo.simbolo);

        QPointF posProyectada;

        // Misma máquina de estados pura para el centro de los átomos
        if (m_view->getModoEditor() == MoleculaView::ModoSeleccion) {
            posProyectada = MoleculaMat::moverPunto(atomo.posicion, m_deltaX, m_deltaY);
        } else {
            posProyectada = MoleculaMat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);
        }

        qreal x         = posProyectada.x() - radio;
        qreal y         = posProyectada.y() - radio;
        qreal diametro  = radio * 2.0f;

        QGraphicsEllipseItem* elipse = m_escena->addEllipse(x, y, diametro, diametro);
        elipse->setBrush(QColor(color));
        elipse->setPen(QPen(QColor("#1F2937"), 1.5f));
        elipse->setZValue(1);

        // Feedback visual adaptado a la coordenada del átomo
        if (m_view && m_view->getIdAtomoSeleccionado() == atomo.id) {
            qreal radioAnillo = radio + 6.0;
            qreal xAnillo = posProyectada.x() - radioAnillo;
            qreal yAnillo = posProyectada.y() - radioAnillo;
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
            posProyectada.x() - (contornoTexto.width() / 2),
            posProyectada.y() - (contornoTexto.height() / 2)
            );
        texto->setZValue(2);
    }

    this->viewport()->update();
}

//##############################################################################################
// Funciones Protegidas Sobreescritas
//#############################################################################################

void MoleculaLienzo::mousePressEvent(QMouseEvent* mouseEv){
    if(!m_view) return;

    m_ultimaPosRat = mouseEv->pos();

    // Si no
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
    if (!m_view) {
        QGraphicsView::mouseMoveEvent(mouseEv);
        return;
    }

    // Calculamos el micro-delta instantáneo entre la posición intermedia anterior y la actual
    int deltaX = mouseEv->pos().x() - m_ultimaPosRat.x();
    int deltaY = mouseEv->pos().y() - m_ultimaPosRat.y();

    // ─── POLÍTICA A: MODO ROTACIÓN 3D (MUTAR ÁNGULOS LOCALES) ───
    if(m_view->getModoEditor() == MoleculaView::ModoRotacion3D && (mouseEv->buttons() & Qt::LeftButton)) {
        m_anguloY += deltaX * 0.5f;
        m_anguloX += deltaY * 0.5f;
        m_anguloX = qMax(-85.0f, qMin(85.0f, m_anguloX));

        m_ultimaPosRat = mouseEv->pos(); // Sincronizamos el ancla intermedia
        actualizarLienzo();
    }
    // ─── POLÍTICA B: MODO SELECCIÓN (MUTAR DELTAS LOCALES, EL CEREBRO NO TRABAJA) ───
    else if(m_view->getModoEditor() == MoleculaView::ModoSeleccion && (mouseEv->buttons() & Qt::LeftButton)) {
        if (deltaX != 0 || deltaY != 0) {
            // Refrescamos y acumulamos los deltas locales en el lienzo
            m_deltaX += static_cast<qreal>(deltaX);
            m_deltaY += static_cast<qreal>(deltaY);

            m_ultimaPosRat = mouseEv->pos(); // Sincronizamos el ancla intermedia exactamente igual que arriba
            actualizarLienzo();             // Forzamos el repintado óptico fluido en tiempo real
        }
    }
    else {
        QGraphicsView::mouseMoveEvent(mouseEv);
    }
}

void MoleculaLienzo::mouseReleaseEvent(QMouseEvent* mouseEv){
    if(!m_view) {
        QGraphicsView::mouseReleaseEvent(mouseEv);
        return;
    }

    // ─── TRANSACCIÓN DE CIERRE AL LEVANTAR EL BOTÓN IZQUIERDO EN SELECCIÓN ───
    if(m_view->getModoEditor() == MoleculaView::ModoSeleccion && mouseEv->button() == Qt::LeftButton) {
        // 1. Convertimos los píxeles de pantalla a coordenadas finales reales de la escena
        QPointF posClickEscenaFinal = this->mapToScene(mouseEv->pos());

        // 2. Respaldamos los deltas acumulados locales del arrastre
        qreal deltaFinalX = m_deltaX;
        qreal deltaFinalY = m_deltaY;

        // 3. LIMPIEZA PROTOCOLARIA: Reseteamos los acumuladores visuales a cero de golpe
        m_deltaX = 0.0;
        m_deltaY = 0.0;
        m_ultimaPosRat = QPoint(0, 0); // Limpiamos la caché del ancla intermedia de Qt

        // 4. ENVIAR AL CEREBRO: Mandamos la coordenada final pura de la escena
        // El cerebro computará la diferencia real y el lienzo queda limpio para la próxima acción
        m_view->setClick(QVector3D(posClickEscenaFinal.x(), posClickEscenaFinal.y(), 0.0f));

        mouseEv->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(mouseEv);
}

void MoleculaLienzo::resizeEvent(QResizeEvent* event){
    QGraphicsView::resizeEvent(event);
    this->resetTransform();
    m_escena->update();

    // ─── ANCLAMOS EL FOCO DE LA PANTALLA EN EL CENTRO REAL (0,0) ───
    centerOn(0, 0);
}
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
#include <QTimer>

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

    m_timerVibracion = new QTimer(this);
    connect(m_timerVibracion, &QTimer::timeout, this, &MoleculaLienzo::avanzarOscilacionVibratoria);

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
    QVector<Frecuencias> espectro   = m_view->getListaFrecuencias();

    int idxFreq = m_view->getFrecuenciaActiva();

    // ─── ¡NUEVO!: FACTOR DE OSCILACIÓN CENTRALIZADO PARA EVITAR CONFLICTOS ───
    float factorSeno = 0.0f;
    if (idxFreq >= 0 && idxFreq < espectro.size()) {
        const float AMPLITUD_GRAFICA = 30.0f;
        factorSeno = qSin(m_faseVibracion) * AMPLITUD_GRAFICA;
    }

    m_escena->clear();

    // =========================================================================
    // 1. RENDERIZAMOS LOS ENLACES - CON PROYECCIÓN TRIDIMENSIONAL SINCRONIZADA
    // ==========================================================================
    QPen penEnlaces(QColor("#D1D5DB"));
    penEnlaces.setWidthF(3.0f);
    penEnlaces.setCapStyle(Qt::RoundCap);
    const qreal offsetDist = 8.0;

    for(const Enlace &enlace: listaEnlaces){
        QVector3D posOrigen, posDestino;
        QVector3D vibOrigen(0,0,0), vibDestino(0,0,0);
        bool oEncontrado = false, dEncontrado = false;

        for(const Atomo &atomo: listaAtomos){
            if(atomo.id == enlace.id_atomo1) {
                posOrigen = atomo.posicion;
                oEncontrado = true;
                // Buscamos su vector de oscilación cuántica si hay frecuencia activa
                if (idxFreq >= 0 && idxFreq < espectro.size()) {
                    for (const Vibraciones& v : espectro[idxFreq].vibraciones) {
                        if (v.id_atomo == atomo.id) {
                            vibOrigen = QVector3D(v.dx * factorSeno, v.dy * factorSeno, v.dz * factorSeno);
                            break;
                        }
                    }
                }
            }
            if(atomo.id == enlace.id_atomo2) {
                posDestino = atomo.posicion;
                dEncontrado = true;
                // Buscamos su vector de oscilación cuántica si hay frecuencia activa
                if (idxFreq >= 0 && idxFreq < espectro.size()) {
                    for (const Vibraciones& v : espectro[idxFreq].vibraciones) {
                        if (v.id_atomo == atomo.id) {
                            vibDestino = QVector3D(v.dx * factorSeno, v.dy * factorSeno, v.dz * factorSeno);
                            break;
                        }
                    }
                }
            }
            if(oEncontrado && dEncontrado) break;
        }

        if(oEncontrado && dEncontrado){
            QPointF p1, p2;
            if (m_view->getModoEditor() == MoleculaView::ModoSeleccion) {
                p1 = MoleculaMat::moverPunto(posOrigen, m_deltaX, m_deltaY);
                p2 = MoleculaMat::moverPunto(posDestino, m_deltaX, m_deltaY);
            } else {
                // Rotamos la posición base y le sumamos la vibración rotada bajo los mismos ángulos de cámara
                p1 = MoleculaMat::rotarPunto(posOrigen, m_anguloX, m_anguloY) + MoleculaMat::rotarPunto(vibOrigen, m_anguloX, m_anguloY);
                p2 = MoleculaMat::rotarPunto(posDestino, m_anguloX, m_anguloY) + MoleculaMat::rotarPunto(vibDestino, m_anguloX, m_anguloY);
            }

            qreal dx = p2.x() - p1.x();
            qreal dy = p2.y() - p1.y();
            qreal longitud = qSqrt(dx * dx + dy * dy);
            if (longitud < 0.001) continue;
            qreal px = -dy / longitud;
            qreal py = dx / longitud;

            if (enlace.orden == 1) {
                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
            }
            else if (enlace.orden == 2) {
                qreal ox = px * (offsetDist / 2.0); qreal oy = py * (offsetDist / 2.0);
                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
            else if (enlace.orden >= 3) {
                qreal ox = px * offsetDist; qreal oy = py * offsetDist;
                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
        }
    }

    // =========================================================================
    // 2. RENDERIZAMOS LOS ATOMOS - CON PROYECCIÓN TRIDIMENSIONAL INDEPENDIENTE
    // ==========================================================================
    for(const Atomo &atomo : listaAtomos){
        qreal radio = Atomos::getRadio(atomo.simbolo);
        QColor color = Atomos::getColor(atomo.simbolo);
        QPointF posProyectada;

        if (m_view->getModoEditor() == MoleculaView::ModoSeleccion) {
            posProyectada = MoleculaMat::moverPunto(atomo.posicion, m_deltaX, m_deltaY);
        } else {
            // Rotamos la posición estática en reposo
            posProyectada = MoleculaMat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);

            // Si el modo analítico está corriendo, rotamos su empuje dinámico
            if (idxFreq >= 0 && idxFreq < espectro.size()) {
                for (const Vibraciones& v : espectro[idxFreq].vibraciones) {
                    if (v.id_atomo == atomo.id) {
                        QVector3D vectorVibracionCrudo(v.dx * factorSeno, v.dy * factorSeno, v.dz * factorSeno);
                        QPointF vibracionProyectada = MoleculaMat::rotarPunto(vectorVibracionCrudo, m_anguloX, m_anguloY);

                        // Acumulamos directamente sobre el plano de proyección bidimensional
                        posProyectada.setX(posProyectada.x() + vibracionProyectada.x());
                        posProyectada.setY(posProyectada.y() + vibracionProyectada.y());
                        break;
                    }
                }
            }
        }

        qreal x = posProyectada.x() - radio;
        qreal y = posProyectada.y() - radio;
        qreal diametro = radio * 2.0f;

        QGraphicsEllipseItem* elipse = m_escena->addEllipse(x, y, diametro, diametro);
        elipse->setBrush(QColor(color));
        elipse->setPen(QPen(QColor("#1F2937"), 1.5f));
        elipse->setZValue(1);

        if (m_view && m_view->getIdAtomoSeleccionado() == atomo.id) {
            qreal radioAnillo = radio + 6.0;
            qreal xAnillo = posProyectada.x() - radioAnillo;
            qreal yAnillo = posProyectada.y() - radioAnillo;
            QGraphicsEllipseItem* anilloSel = m_escena->addEllipse(xAnillo, yAnillo, radioAnillo * 2.0, radioAnillo * 2.0);
            QPen penSel(QColor("#38BDF8")); penSel.setWidthF(2.5f); penSel.setStyle(Qt::DashLine);
            anilloSel->setPen(penSel); anilloSel->setZValue(3);
        }

        QGraphicsSimpleTextItem* texto = m_escena->addSimpleText(atomo.simbolo);
        QFont fuenteApp("Segoe UI", 12, QFont::Bold); texto->setFont(fuenteApp);
        texto->setBrush(QColor(Atomos::isTextoOscuro(atomo.simbolo) ? "#111827" : "#FFFFFF"));
        QRectF contornoTexto = texto->boundingRect();
        texto->setPos(posProyectada.x() - (contornoTexto.width() / 2), posProyectada.y() - (contornoTexto.height() / 2));
        texto->setZValue(2);
    }

    this->viewport()->update();
}

void MoleculaLienzo::encenderVibracion(int ms) {
    if (m_timerVibracion && !m_timerVibracion->isActive()) {
        m_timerVibracion->start(ms);
    }
}

void MoleculaLienzo::apagarVibracion() {
    if (m_timerVibracion) {
        m_timerVibracion->stop();
    }
    m_faseVibracion = 0.0f; // Retorno inmediato al estado de equilibrio estable
}

//###############################################################################################
// Private slots
//##############################################################################################

void MoleculaLienzo::avanzarOscilacionVibratoria() {
    // Incrementamos el ángulo angular de la fase (aproximadamente 30 FPS fluidos)
    m_faseVibracion += 0.2f;
    if (m_faseVibracion > 6.2831f) { // 2 * PI: reseteo de ciclo limpio para evitar desbordamientos
        m_faseVibracion = 0.0f;
    }

    // Forzamos el repintado de la escena con la nueva deformación calculada
    actualizarLienzo();
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
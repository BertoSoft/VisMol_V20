#ifndef MOLECULALIENZO_H
#define MOLECULALIENZO_H

#include "moleculaview.h"

#include <QGraphicsScene>
#include <QGraphicsView>

class MoleculaLienzo : public QGraphicsView{

    Q_OBJECT

public:

    MoleculaLienzo(QWidget* parent = nullptr);
    ~MoleculaLienzo() override;

    void setMoleculaView(MoleculaView* view);
    void actualizarLienzo();
    void resetearVistaFrontal();

protected:

    void mousePressEvent(QMouseEvent* mouseEv) override;
    void mouseMoveEvent(QMouseEvent* mouseEv) override; // <-- Captura el arrastre dinámico
    void mouseReleaseEvent(QMouseEvent* mouseEv) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_escena = nullptr; // El espacio infinito donde se colocarán los gráficos
    MoleculaView*   m_view   = nullptr; // Nuestro puente con el negocio de la aplicación

    // =========================================================================
    // CONTROL DE ÁNGULOS Y MEMORIA DEL RATÓN PARA ROTACIÓN 3D
    // =========================================================================
    float   m_anguloX      = 0.0f;  // Rotación vertical (Pitch)
    float   m_anguloY      = 0.0f;  // Rotación horizontal (Yaw)
    float   m_deltaX       = 0.0f;
    float   m_deltaY       = 0.0f;
    QPoint  m_ultimaPosRat;         // Guarda el último píxel del cursor para calcular el movimiento (Delta)
};



#endif // MOLECULALIENZO_H

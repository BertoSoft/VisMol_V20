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

protected:

    void mousePressEvent(QMouseEvent* mouseEv) override;

private:
    QGraphicsScene* m_escena = nullptr; // El espacio infinito donde se colocarán los gráficos
    MoleculaView*   m_view   = nullptr; // Nuestro puente con el negocio de la aplicación

};



#endif // MOLECULALIENZO_H

#ifndef MOLECULAMAT_H
#define MOLECULAMAT_H

#include <QPointF>
#include <QVector3D>

class MoleculaMat
{
public:
    MoleculaMat() = delete;
    ~MoleculaMat() = delete;

    /**
     * @brief Calcula la rotación espacial de un punto 3D y lo proyecta a un plano 2D.
     * @param posReal Posición tridimensional original del átomo.
     * @param anguloX Ángulo de rotación vertical (Pitch) en grados.
     * @param anguloY Ángulo de rotación horizontal (Yaw) en grados.
     * @return QPointF Coordenada (X, Y) lista para ser dibujada en el lienzo.
     */
    static QPointF rotarPunto(const QVector3D& posReal, float anguloX, float anguloY);
};

#endif // MOLECULAMAT_H
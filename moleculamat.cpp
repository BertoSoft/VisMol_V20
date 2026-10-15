#include "moleculamat.h"
#include <QtMath>

QPointF MoleculaMat::rotarPunto(const QVector3D& posReal, float anguloX, float anguloY) {
    // 1. Convertimos los ángulos de grados a radianes
    float radX = qDegreesToRadians(anguloX);
    float radY = qDegreesToRadians(anguloY);

    float cosX = qCos(radX);
    float sinX = qSin(radX);
    float cosY = qCos(radY);
    float sinY = qSin(radY);

    // 2. Aplicamos la rotación alrededor del eje Y (Giro horizontal / Yaw)
    float xRotY = posReal.x() * cosY + posReal.z() * sinY;
    float zRotY = -posReal.x() * sinY + posReal.z() * cosY;

    // 3. Aplicamos la rotación alrededor del eje X (Giro vertical / Pitch)
    float yRotX = posReal.y() * cosX - zRotY * sinX;

    // Devolvemos el punto transformado en 2D de forma directa
    return QPointF(xRotY, yRotX);
}
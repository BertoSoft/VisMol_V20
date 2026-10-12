#ifndef DATOSIO_H
#define DATOSIO_H

#include "moleculaview.h"

#include <QString>

class DatosIO
{
public:
    DatosIO()   = delete;
    ~DatosIO()    = delete;

    static QString loadVisMol(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces);
    static QString saveVisMol(const QString& ruta, const QVector<Atomo>& m_listaAtomos, const QVector<Enlace>& m_listaEnlaces);

};

#endif // DATOSIO_H

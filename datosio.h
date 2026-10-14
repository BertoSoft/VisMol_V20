#ifndef DATOSIO_H
#define DATOSIO_H

#include "moleculaview.h"

#include <QString>

class DatosIO
{
public:
    DatosIO()   = delete;
    ~DatosIO()  = delete;

    static QString loadVisMol(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario);
    static QString saveVisMol(const QString& ruta, const QVector<Atomo>& m_listaAtomos, const QVector<Enlace>& m_listaEnlaces, const QString& comentario);
    static QString saveMOPAC(const QString& ruta, const QVector<Atomo>& m_listaAtomos,  const QString& comentario, const QString& argumentos);
    static QString loadMOPAC(const QString& ruta, QVector<Atomo>& m_listaAtomos, QString& comentario, QString& argumentos);

private:
    // FACTOR DE ESCALA CRÍTICO: 100 píxeles de pantalla equivalen a 1.0 Ångstrom real
    static constexpr float FACTOR_ANGSTROM = 100.0f;
};

#endif // DATOSIO_H

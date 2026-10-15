#ifndef DATOSIO_H
#define DATOSIO_H

#include "moleculaview.h"
#include <QString>
#include <QVector>

class DatosIO
{
public:

    DatosIO()   = delete;
    ~DatosIO()  = delete;

    static QString loadVisMol(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario);
    static QString loadMOPAC(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario, QString& argumentos, TipoFormato formato, QString& texto);

    static QString saveVisMol(const QString& ruta, const QVector<Atomo>& m_listaAtomos, const QVector<Enlace>& m_listaEnlaces, const QString& comentario);
    static QString saveMOPAC(const QString& ruta, const QVector<Atomo>& m_listaAtomos,  const QString& comentario, const QString& argumentos);

    // Firma corregida: pasamos referencias directas a objetos reales y añadimos el flag de formato

private:
    static inline const float FACTOR_ANGSTROM = 100.0f;
    // Parsers modulares independientes especializados por extensión de archivo
    static QString parseVisMol(QTextStream& entrada, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario);
    static QString parseDAT(QTextStream& entrada, QVector<Atomo>& m_listaAtomos, QString& comentario, QString& argumentos);
    static QString parseOUT(QTextStream& entrada, QVector<Atomo>& m_listaAtomos);
    static QString parseARC(QTextStream& entrada, QVector<Atomo>& m_listaAtomos);
};

#endif // DATOSIO_H
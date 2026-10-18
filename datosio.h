#ifndef DATOSIO_H
#define DATOSIO_H

#include "moleculaview.h"

#include <QString>
#include <QVector>
#include <QTextStream>

class DatosIO
{
public:
    DatosIO()   = delete;
    ~DatosIO()  = delete;

    static QString  loadFrecuencias(const QString& ruta, QVector<Frecuencias>& m_listaFrecuencias);
    static QString  loadVisMol(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario);
    static QString  loadMOPAC(const QString& ruta, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario, QString& argumentos, QVector<Frecuencias>& m_listaFrecuencias, TipoFormato formato, QString& texto);
    static QString  loadMopacAux(const QString& ruta, QVector<Trayectoria>& m_trayectoria, QString& comentario, QString& argumentos, QString& texto);
    static QString  saveVisMol(const QString& ruta, const QVector<Atomo>& m_listaAtomos, const QVector<Enlace>& m_listaEnlaces, const QString& comentario);
    static QString  saveMOPAC(const QString& ruta, const QVector<Atomo>& m_listaAtomos, const QString& comentario, const QString& argumentos);
    static QString  leerLogTexto(const QString& ruta);
    static QString  loadAuxForce(const QString& ruta, QVector<Atomo>& m_listaAtomos);

    static inline const float FACTOR_ANGSTROM = 100.0f;


private:

    // Parsers modulares independientes especializados por extensión de archivo
    static QString              parseVisMol(QTextStream& entrada, QVector<Atomo>& m_listaAtomos, QVector<Enlace>& m_listaEnlaces, QString& comentario);
    static QString              parseDAT(QTextStream& entrada, QVector<Atomo>& m_listaAtomos, QString& comentario, QString& argumentos);
    static QString              parseOUT(QTextStream& entrada, QVector<Atomo>& m_listaAtomos);
    static QString              parseARC(QTextStream& entrada, QVector<Atomo>& m_listaAtomos);
    static QString              parseMopacAux(QTextStream& entrada, QVector<Trayectoria>& m_trayectoria, QString& comentario, QString& argumentos);
    static QVector<Frecuencias> parserFrecuencias(const QString& ruta);
    static QString              parseAuxForce(QTextStream& entrada, QVector<Atomo>& m_listaAtomos);

    // Utilitario privado de refactorización para evitar código duplicado en Trayectorias
    static void procesarYCentrarFotograma(const QStringList& plantillaSimbolos, const QVector<float>& coordsEtapa, QVector<Trayectoria>& m_trayectoria);
};

#endif // DATOSIO_H
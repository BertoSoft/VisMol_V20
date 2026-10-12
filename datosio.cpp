#include "datosio.h"
#include "moleculaview.h"

#include <QFile>
#include <QTextStream>

QString DatosIO::loadVisMol(
    const QString& ruta,
    QVector<Atomo>& m_listaAtomos,
    QVector<Enlace>& m_listaEnlaces
    ){

    QFile archivo(ruta);
    // Abrimos el archivo en modo lectura y forzamos el tratamiento de texto plano
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error al abrir el archivo %1").arg(ruta);
    }

    QTextStream entrada(&archivo);



























    return "";
}

QString DatosIO::saveVisMol(
    const QString& ruta,
    const QVector<Atomo>& m_listaAtomos,
    const QVector<Enlace>& m_listaEnlaces
    ){

    return "";
}
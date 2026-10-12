#include "datosio.h"
#include "moleculaview.h"

#include <QFile>
#include <QTextStream>

QString DatosIO::loadVisMol(
    const QString& ruta,
    QVector<Atomo>& m_listaAtomos,
    QVector<Enlace>& m_listaEnlaces,
    QString& comentario
    ){

    QFile archivo(ruta);
    // Abrimos el archivo en modo lectura y forzamos el tratamiento de texto plano
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error al abrir el archivo %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // 1. Limpieza absoluta preventiva de la memoria local
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    comentario = "";

    // =========================================================================
    // BLOQUE DE CABECERA: LECTURA SIMÉTRICA DE LAS 3 LÍNEAS OBLIGATORIAS
    // =========================================================================
    for(int i=0; i<3; i++){
        QString linea = entrada.readLine();

        if(i==1){
            comentario = linea.trimmed();
        }
    }

    // =========================================================================
    // SECCIONES DE DATOS (ÁTOMOS Y ENLACES)
    // =========================================================================
    QString seccionActual = "";

    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();

        // Ignorar líneas vacías o comentarios adicionales con '#'
        if (linea.isEmpty() || linea.startsWith("#")) {
            continue;
        }

        // Detectar los delimitadores oficiales de bloques
        if (linea == "[ATOMOS]" || linea == "[ENLACES]") {
            seccionActual = linea;
            continue;
        }

        // --- SECCIÓN DE ÁTOMOS (id, simbolo, x, y, z) ---
        if (seccionActual == "[ATOMOS]") {
            QStringList tokens = linea.split(',');
            if (tokens.size() < 5) continue; // Si la línea está incompleta, saltar

            Atomo atomo;
            atomo.id       = tokens[0].trimmed().toInt();
            atomo.simbolo  = tokens[1].trimmed();

            // Reconstruimos la posición manteniendo toda la precisión de los flotantes
            float x = tokens[2].trimmed().toFloat();
            float y = tokens[3].trimmed().toFloat();
            float z = tokens[4].trimmed().toFloat();
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }

        // --- SECCIÓN DE ENLACES (id_atomo1, id_atomo2, orden_enlace) ---
        else if (seccionActual == "[ENLACES]") {
            QStringList tokens = linea.split(',');
            if (tokens.size() < 3) continue;

            Enlace enlace;
            enlace.id_atomo1 = tokens[0].trimmed().toInt();
            enlace.id_atomo2 = tokens[1].trimmed().toInt();
            enlace.orden     = tokens[2].trimmed().toInt();

            m_listaEnlaces.append(enlace);
        }
    }

    archivo.close();
    return ""; // Retornar cadena vacía indica éxito absoluto
}

QString DatosIO::saveVisMol(
    const QString& ruta,
    const QVector<Atomo>& m_listaAtomos,
    const QVector<Enlace>& m_listaEnlaces,
    const QString& comentario
    ){

    QFile archivo(ruta);
    // Abrimos el archivo en modo lectura y forzamos el tratamiento de texto plano
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString("Error al abrir el archivo %1").arg(ruta);
    }

    QTextStream salida(&archivo);

    // =========================================================================
    // BLOQUE DE CABECERA CON EL NUEVO ORDEN REQUERIDO
    // =========================================================================
    // Línea 1: Reservada para futuros argumentos de MOPAC (Ej: método y optimización)
    salida << "KEYWORDS MOPAC: AM1 PRECISE GEOM-OPT\n";

    // Línea 2: Comentario personal o título del proyecto proporcionado por el usuario
    if (comentario.isEmpty()) {
        salida << "Molecula generada con VisorMol_V20\n";
    } else {
        salida << comentario << "\n";
    }

    // Línea 3: Línea en blanco obligatoria de separación
    salida << "\n";

    //=========================================================================
    // SECCIONES DE DATOS (ÁTOMOS Y ENLACES)
    // =========================================================================
    salida << "[ATOMOS]\n";
    for (const Atomo& atomo : m_listaAtomos) {
        // qSetRealNumberPrecision(6) garantiza la máxima exactitud compatible con cálculos cuánticos
        salida << atomo.id << ", "
               << atomo.simbolo << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << atomo.posicion.x() << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << atomo.posicion.y() << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << atomo.posicion.z() << "\n";
    }

    salida << "\n";

    // =========================================================================
    // SECCIÓN DE ENLACES
    // =========================================================================
    salida << "[ENLACES]\n";
    for (const Enlace& enlace : m_listaEnlaces) {
        salida << enlace.id_atomo1 << ", "
               << enlace.id_atomo2 << ", "
               << enlace.orden << "\n";
    }

    archivo.close();
    return "";
}
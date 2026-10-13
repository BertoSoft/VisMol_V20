#include "datosio.h"
#include "moleculaview.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

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

QString DatosIO::saveMOPAC(const QString& ruta, const QVector<Atomo>& m_listaAtomos,  const QString& comentario, const QString& argumentos){
    QFile archivo(ruta);
    // Abrimos el archivo en modo escritura y forzamos el tratamiento de texto plano
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString("Error: No se tienen permisos de escritura para crear el archivo %1").arg(ruta);
    }

    QTextStream salida(&archivo);

    // =========================================================================
    // 1. ESTRUCTURA DE CABECERA EXIGIDA POR MOPAC
    // =========================================================================
    // Línea 1: Las instrucciones cuánticas (Keywords) limpias y en mayúsculas
    salida << argumentos.trimmed().toUpper() << "\n";

    // Línea 2: El título o comentario identificador de la molécula
    if (comentario.isEmpty()) {
        salida << "Molecula generada con VisorMol_V20\n";
    } else {
        salida << comentario << "\n";
    }

    // Línea 3: Línea en blanco OBLIGATORIA (Delimitador físico para el código de MOPAC)
    salida << "\n";

    // =========================================================================
    // 2. BLOQUE GEOMÉTRICO (COORDENADAS CARTESIANAS + FLAGS '1')
    // =========================================================================
    for (const Atomo& atomo : m_listaAtomos) {
        // Formato estándar MOPAC: Símbolo \t X 1 \t Y 1 \t Z 1
        // qSetRealNumberPrecision(6) garantiza la máxima exactitud para matrices de energía cuántica
        salida << atomo.simbolo << "\t"
               << qSetRealNumberPrecision(6) << Qt::fixed
               << atomo.posicion.x() << " 1\t"
               << atomo.posicion.y() << " 1\t"
               << atomo.posicion.z() << " 1\n";
    }

    archivo.close();
    return ""; // Retornar una cadena vacía indica éxito absoluto en la escritura física
}

QString DatosIO::loadMOPAC(const QString& ruta, QVector<Atomo>& m_listaAtomos, QString& comentario, QString& argumentos){
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo de salida MOPAC %1").arg(ruta);
    }

    QTextStream entrada(&archivo);
    QStringList todasLasLineas;

    // 1. Volcamos todo el mar de texto en memoria para poder viajar en el tiempo hacia atrás
    while (!entrada.atEnd()) {
        todasLasLineas.append(entrada.readLine());
    }
    archivo.close();

    // Limpieza preventiva de las estructuras del lienzo
    m_listaAtomos.clear();
    comentario = "Geometría Final Optimizada (MOPAC)";
    argumentos = "";

    // 2. RASTREO FASE A: Buscamos el cartel de fin de trabajo desde el fondo del archivo
    int indiceJobEnded = -1;
    for (int i = todasLasLineas.size() - 1; i >= 0; --i) {
        if (todasLasLineas[i].contains("JOB ENDED NORMALLY")) {
            indiceJobEnded = i;
            break;
        }
    }

    if (indiceJobEnded == -1) {
        return "Error: El cálculo de MOPAC no terminó correctamente (No se encontró 'JOB ENDED NORMALLY').";
    }

    // 3. RASTREO FASE B: Desde ese cartel, subimos hacia atrás buscando las coordenadas finales
    int indiceCartesian = -1;
    for (int i = indiceJobEnded; i >= 0; --i) {
        if (todasLasLineas[i].contains("CARTESIAN COORDINATES")) {
            indiceCartesian = i;
            break;
        }
    }

    if (indiceCartesian == -1) {
        return "Error: No se encontró ningún bloque de 'CARTESIAN COORDINATES' previo a la finalización del trabajo.";
    }

    // 4. EXTRACCIÓN DE DATOS: Volvemos a leer hacia adelante a partir de la cabecera encontrada
    int idContador = 1;
    QRegularExpression separadorEspacios("\\s+");

    // Saltamos la línea del título "CARTESIAN COORDINATES" y las cabeceras vacías/decorativas
    int inicioBloqueAtomos = indiceCartesian + 1;
    while (inicioBloqueAtomos < todasLasLineas.size()) {
        QString lineaLimpia = todasLasLineas[inicioBloqueAtomos].trimmed();
        // Si la línea contiene números de columna o está en blanco, seguimos bajando hasta los átomos reales
        if (lineaLimpia.isEmpty() || lineaLimpia.contains("NO.") || lineaLimpia.contains("ATOM")) {
            inicioBloqueAtomos++;
        } else {
            break; // Hemos llegado al primer átomo real
        }
    }

    // Procesamos la tabla línea a línea en orden normal hasta encontrar el cierre (línea vacía)
    for (int i = inicioBloqueAtomos; i < todasLasLineas.size(); ++i) {
        QString lineaAtomo = todasLasLineas[i].trimmed();

        // MOPAC cierra la tabla de coordenadas con una línea en blanco
        if (lineaAtomo.isEmpty()) {
            break;
        }

        QStringList columnas = lineaAtomo.split(separadorEspacios);

        // Formato esperado de la fila: "1   C   -500.0000   -54.0000   0.0000"
        if (columnas.size() >= 5) {
            Atomo atomo;
            atomo.id      = idContador++;
            atomo.simbolo = columnas[1].trimmed(); // Columna 2: Símbolo químico

            // Extraemos las nuevas posiciones tridimensionales moleculares
            float x = columnas[2].toFloat();
            float y = columnas[3].toFloat();
            float z = columnas[4].toFloat();
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }
    }

    if (m_listaAtomos.isEmpty()) {
        return "Error: El bloque 'CARTESIAN COORDINATES' estaba vacío o corrupto.";
    }

    return ""; // ¡Éxito absoluto!
}

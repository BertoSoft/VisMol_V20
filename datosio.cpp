#include "datosio.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>

QString DatosIO::leerLogTexto(const QString& ruta) {
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ""; // Fallback defensivo: si MOPAC aún no ha escrito nada, devuelve vacío
    }
    QTextStream entrada(&archivo);
    QString contenido = entrada.readAll();
    archivo.close();
    return contenido;
}

QString DatosIO::loadFrecuencias(const QString& ruta, QVector<Frecuencias>& m_listaFrecuencias) {
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo de frecuencias %1").arg(ruta);
    }
    archivo.close(); // El parserFrecuencias interno se encargará de reabrirlo de forma controlada

    // Limpieza preventiva transaccional antes de poblar
    m_listaFrecuencias.clear();

    // Delegamos de forma segura en la maquinaria del parser privado de la casa
    m_listaFrecuencias = parserFrecuencias(ruta);

    return ""; // Éxito transaccional sin incidentes
}

QString DatosIO::loadVisMol(
    const QString& ruta,
    QVector<Atomo>& m_listaAtomos,
    QVector<Enlace>& m_listaEnlaces,
    QString& comentario
    ) {
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo VisMol %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // Limpieza transaccional de los contenedores
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    comentario = "";

    QString resultado = parseVisMol(entrada, m_listaAtomos, m_listaEnlaces, comentario);

    archivo.close();
    return resultado;
}

QString DatosIO::loadMOPAC(
    const QString& ruta,
    QVector<Atomo>& m_listaAtomos,
    QVector<Enlace>& m_listaEnlaces,
    QString& comentario,
    QString& argumentos,
    QVector<Frecuencias>& m_listaFrecuencias,
    TipoFormato formato,
    QString& texto
    ) {

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo MOPAC %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // Inicialización y limpieza transaccional de todos los contenedores
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    m_listaFrecuencias.clear(); // ─── ¡LIMPIEZA AQUÍ!
    comentario = "";
    argumentos = "";

    if (formato == TipoFormato::OUT || formato == TipoFormato::ARC) {
        qint64 posicionOriginal = archivo.pos();
        archivo.seek(0);
        texto = entrada.readAll();
        archivo.seek(posicionOriginal);
    }

    // ─── [NUEVO] EXTRACCIÓN AUTOMÁTICA DEL ESPECTRO INFRACOJO (IR) ───
    if (formato == TipoFormato::OUT) {
        m_listaFrecuencias = parserFrecuencias(ruta); // Invoca internamente a tu parser
    }

    QString resultado = "";
    if(formato == TipoFormato::DAT) resultado = parseDAT(entrada, m_listaAtomos, comentario, argumentos);
    if(formato == TipoFormato::OUT) resultado = parseOUT(entrada, m_listaAtomos);
    if(formato == TipoFormato::ARC) resultado = parseARC(entrada, m_listaAtomos);

    archivo.close();
    return resultado;
}

QString DatosIO::loadMopacAux(
    const QString& ruta,
    QVector<Trayectoria>& m_trayectoria,
    QString& comentario,
    QString& argumentos,
    QString& texto // <-- NUEVO PARÁMETRO
    ) {
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo MOPAC AUX %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // Limpieza transaccional controlada antes de procesar
    m_trayectoria.clear();
    comentario = "";
    argumentos = "";
    texto = ""; // Inicializamos el texto vacío

    // ─── [NUEVO] LECTURA ESPEJO DEL ARCHIVO .OUT ASOCIADO ───
    QFileInfo info(ruta);
    QString rutaEspejoOut = info.absolutePath() + "/" + info.baseName() + ".out";
    if (QFile::exists(rutaEspejoOut)) {
        texto = leerLogTexto(rutaEspejoOut); // Reutiliza tu lector nativo de texto
    }

    // Delegación directa en el parser interno de la casa
    QString resultado = parseMopacAux(entrada, m_trayectoria, comentario, argumentos);

    archivo.close();
    return resultado;
}

QString DatosIO::loadAuxForce(const QString& ruta, QVector<Atomo>& m_listaAtomos) {
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo auxiliar de fuerzas %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // Limpieza preventiva transaccional en la memoria antes de poblar
    m_listaAtomos.clear();

    // Delegamos de forma segura en la maquinaria del parser privado
    QString resultado = parseAuxForce(entrada, m_listaAtomos);

    archivo.close();
    return resultado; // Retorna el éxito o el código de error de sintaxis del parser
}

QString DatosIO::saveVisMol(
    const QString& ruta,
    const QVector<Atomo>& m_listaAtomos,
    const QVector<Enlace>& m_listaEnlaces,
    const QString& comentario
    ){

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString("Error al abrir el archivo %1").arg(ruta);
    }

    QTextStream salida(&archivo);

    // =========================================================================
    // BLOQUE DE CABECERA CON EL ORDEN REQUERIDO
    // =========================================================================
    salida << "KEYWORDS MOPAC: AM1 PRECISE GEOM-OPT\n";

    if (comentario.isEmpty()) {
        salida << "Molecula generada con VisorMol_V20\n";
    } else {
        salida << comentario.trimmed() << "\n";
    }

    salida << "\n";

    // =========================================================================
    // SECCIÓN DE ÁTOMOS (Coordenadas nativas del lienzo/píxeles)
    // =========================================================================
    salida << "[ATOMOS]\n";
    for (const Atomo& atomo : m_listaAtomos) {

        // Reducimos las coordenadas del lienzo a unidades físicas reales antes de escribir
        float x_angstrom = atomo.posicion.x() / FACTOR_ANGSTROM;
        float y_angstrom = atomo.posicion.y() / FACTOR_ANGSTROM;
        float z_angstrom = atomo.posicion.z() / FACTOR_ANGSTROM;

        // CORREGIDO: Escribimos las variables convertidas en Ångstroms
        salida << atomo.id << ", "
               << atomo.simbolo << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << x_angstrom << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << y_angstrom << ", "
               << qSetRealNumberPrecision(6) << Qt::fixed << z_angstrom << "\n";
    }

    salida << "\n";

    // =========================================================================
    // SECCIÓN DE ENLACES (Campos corregidos según parseVisMol)
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

QString DatosIO::saveMOPAC(
    const QString& ruta,
    const QVector<Atomo>& m_listaAtomos,
    const QString& comentario,
    const QString& argumentos
    ){
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString("Error: No se tienen permisos de escritura para crear el archivo %1").arg(ruta);
    }

    QTextStream salida(&archivo);

    // =========================================================================
    // 1. ESTRUCTURA DE CABECERA EXIGIDA POR MOPAC
    // =========================================================================
    salida << (argumentos.isEmpty() ? "AM1 PRECISE GEOM-OPT" : argumentos.trimmed().toUpper()) << "\n";

    if (comentario.isEmpty()) {
        salida << "Molecula generada con VisorMol_V20\n";
    } else {
        salida << comentario.trimmed() << "\n";
    }

    salida << "\n";

    // =========================================================================
    // [QUIRÚRGICO] DETECCIÓN DE PLANARIDAD EN EL EJE Z
    // =========================================================================
    bool esTotalmentePlanaEnZ = true;
    for (const Atomo& atomo : m_listaAtomos) {
        if (qAbs(atomo.posicion.z()) > 0.0001f) {
            esTotalmentePlanaEnZ = false;
            break;
        }
    }

    // =========================================================================
    // 2. BLOQUE GEOMÉTRICO (ESCALADO INVERSO CRÍTICO: Píxeles -> Ångstroms)
    // =========================================================================
    for (int i = 0; i < m_listaAtomos.size(); ++i) {
        const Atomo& atomo = m_listaAtomos[i];

        // Conversión matemática inversa obligatoria a Ångstroms
        float x_angstrom = atomo.posicion.x() / FACTOR_ANGSTROM;
        float y_angstrom = atomo.posicion.y() / FACTOR_ANGSTROM;
        float z_angstrom = atomo.posicion.z() / FACTOR_ANGSTROM;

        // Si la molécula es 100% plana en Z, inyectamos una levísima perturbación
        // asimétrica (entre -0.05 y +0.05 Å) usando el índice para romper la simetría C_s
        if (esTotalmentePlanaEnZ) {
            z_angstrom = 0.05f * qSin(i * 1.57f); // Altera de forma pseudoaleatoria las Z de los átomos
        }

        // Formato estándar MOPAC utilizando tabuladores y flags de optimización espacial '1'
        salida << atomo.simbolo << "\t"
               << qSetRealNumberPrecision(6) << Qt::fixed
               << x_angstrom << " 1\t"
               << y_angstrom << " 1\t"
               << z_angstrom << " 1\n";
    }

    archivo.close();
    return "";
}

// PARSER .VISMOL: Archivo de Entrada estructurado (enlaces + Comentario + Geometría)
QString DatosIO::parseVisMol(
    QTextStream& entrada,
    QVector<Atomo>& m_listaAtomos,
    QVector<Enlace>& m_listaEnlaces,
    QString& comentario
    ){
    if (entrada.atEnd()) return "Archivo VisMol vacío.";

    // Las primeras líneas antes de [ATOMOS] se consideran el comentario/título
    QStringList lineasComentario;
    QString seccionActual = "";

    // Expresiones regulares precisas para evitar falsos positivos por espacios
    QRegularExpression regexAtomo(R"(^\s*(\d+)\s*,\s*([A-Za-z]{1,2})\s*,\s*(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*))");
    QRegularExpression regexEnlace(R"(^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+))");

    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();
        if (linea.isEmpty()) continue;

        // Detección de cambios de sección
        if (linea.startsWith("[ATOMOS]", Qt::CaseInsensitive)) {
            seccionActual = "ATOMOS";
            continue;
        } else if (linea.startsWith("[ENLACES]", Qt::CaseInsensitive)) {
            seccionActual = "ENLACES";
            continue;
        }

        // Procesado según la sección activa
        if (seccionActual.isEmpty()) {
            lineasComentario.append(linea);
        }
        else if (seccionActual == "ATOMOS") {
            QRegularExpressionMatch match = regexAtomo.match(linea);
            if (match.hasMatch()) {
                Atomo atomo;
                atomo.id = match.captured(1).toInt();
                atomo.simbolo = match.captured(2);

                // FACTORIZADO: Escalamos los Ångstroms entrantes a píxeles del lienzo
                float x = match.captured(3).toFloat() * FACTOR_ANGSTROM;
                float y = match.captured(4).toFloat() * FACTOR_ANGSTROM;
                float z = match.captured(5).toFloat() * FACTOR_ANGSTROM;

                atomo.posicion = QVector3D(x, y, z);

                m_listaAtomos.append(atomo);
            }
        }
        else if (seccionActual == "ENLACES") {
            QRegularExpressionMatch match = regexEnlace.match(linea);
            if (match.hasMatch()) {
                Enlace enlace;
                enlace.id_atomo1 = match.captured(1).toInt();
                enlace.id_atomo2 = match.captured(2).toInt();
                enlace.orden     = match.captured(3).toInt();

                m_listaEnlaces.append(enlace);
            }
        }
    }

    comentario = lineasComentario.join("\n");

    if (m_listaAtomos.isEmpty()) {
        return "Advertencia: El archivo se leyó pero no se encontraron átomos válidos.";
    }

    return ""; // Éxito
}

// PARSER .DAT: Archivo de Entrada estructurado (Keywords + Comentario + Geometría)
QString DatosIO::parseDAT(
    QTextStream& entrada,
    QVector<Atomo>& m_listaAtomos,
    QString& comentario,
    QString& argumentos
    ) {
    if (entrada.atEnd()) return "Archivo .dat vacío.";
    argumentos = entrada.readLine().trimmed();

    if (entrada.atEnd()) return "Archivo .dat incompleto (Falta título/comentario).";
    comentario = entrada.readLine().trimmed();

    if (entrada.atEnd()) return "Archivo .dat estructuralmente inválido (Falta separación).";
    entrada.readLine(); // Consumir línea en blanco obligatoria del estándar MOPAC

    int contadorId = 1;
    // Captura: 1: Símbolo químico, 2: X, 3: Y, 4: Z
    QRegularExpression regex(R"(^([A-Za-z]{1,2})\s+(-?\d+\.?\d*)\s+\d\s+(-?\d+\.?\d*)\s+\d\s+(-?\d+\.?\d*))");

    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();
        if (linea.isEmpty()) break; // Bloque geométrico cerrado por espacio

        QRegularExpressionMatch match = regex.match(linea);
        if (match.hasMatch()) {
            Atomo atomo;
            atomo.id = contadorId++;
            atomo.simbolo = match.captured(1);

            // Multiplica por 100.0f restaurando la reducción hecha al guardar
            float x = match.captured(2).toFloat() * FACTOR_ANGSTROM;
            float y = match.captured(3).toFloat() * FACTOR_ANGSTROM;
            float z = match.captured(4).toFloat() * FACTOR_ANGSTROM;
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }
    }
    return "";
}

// PARSER .OUT: Localiza la geometría final calculada tras optimizar el sistema cuántico
QString DatosIO::parseOUT(
    QTextStream& entrada,
    QVector<Atomo>& m_listaAtomos
    ) {

    QString todoElTexto = entrada.readAll();
    m_listaAtomos.clear();

    // 1. ANCLAJE GEOMÉTRICO: Buscamos el bloque definitivo de coordenadas cartesianas
    int indexGeometriaFinal = todoElTexto.lastIndexOf("CARTESIAN COORDINATES");
    if (indexGeometriaFinal == -1) {
        return "Error: No se encontró el bloque 'CARTESIAN COORDINATES' en el archivo de salida MOPAC.";
    }

    QString seccionFinal = todoElTexto.mid(indexGeometriaFinal);
    QTextStream lector(&seccionFinal);

    // Saltamos las líneas de cabecera del bloque ("CARTESIAN COORDINATES", vacía, títulos)
    lector.readLine();
    lector.readLine();

    // Expresión regular de precisión para absorber las columnas de texto de MOPAC v23
    QRegularExpression regexAtomo(R"(^\s*(\d+)\s+([A-Za-z]{1,2})\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+))");
    int contadorIdFallback = 1;
    bool encontradoBloqueDatos = false;

    while (!lector.atEnd()) {
        QString linea = lector.readLine();
        QString lineaTrimmed = linea.trimmed();

        if (encontradoBloqueDatos && lineaTrimmed.isEmpty()) {
            break; // MOPAC cierra el bloque de coordenadas con una línea en blanco
        }

        if (lineaTrimmed.isEmpty() || lineaTrimmed.startsWith("NO.") || lineaTrimmed.startsWith("ATOM")) {
            continue;
        }

        QRegularExpressionMatch match = regexAtomo.match(linea);
        if (match.hasMatch()) {
            encontradoBloqueDatos = true;
            Atomo atomo;

            // Extraemos el ID real del átomo asignado por el motor cuántico
            QString idCapturado = match.captured(1);
            atomo.id = idCapturado.isEmpty() ? contadorIdFallback++ : idCapturado.toInt();
            atomo.simbolo = match.captured(2);

            // ─── CRUCIAL: MULTIPLICACIÓN POR EL FACTOR DE ESCALA GRÁFICA ───
            float x = match.captured(3).toFloat() * FACTOR_ANGSTROM;
            float y = match.captured(4).toFloat() * FACTOR_ANGSTROM;
            float z = match.captured(5).toFloat() * FACTOR_ANGSTROM;
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }
    }

    if (m_listaAtomos.isEmpty()) {
        return "Error: No se pudieron extraer los átomos del bloque cartesiano.";
    }

    // ─── CENTRALIZACIÓN GEOMÉTRICA CONTRA EL ORIGEN (0,0) ───
    QVector3D acumulador(0.0f, 0.0f, 0.0f);
    for (const Atomo& a : m_listaAtomos) {
        acumulador += a.posicion;
    }
    QVector3D centroDeMasasMopac = acumulador / m_listaAtomos.size();

    // Restamos el centro de masas para clavar la estructura en el centro exacto del lienzo
    for (Atomo& a : m_listaAtomos) {
        a.posicion -= centroDeMasasMopac;
    }

    return ""; // Éxito total
}

// PARSER .ARC: Archivo compacto de resumen definitivo de simulación
QString DatosIO::parseARC(
    QTextStream& entrada,
    QVector<Atomo>& m_listaAtomos
    ) {
    // 1. Avanzar el puntero del archivo hasta encontrar la sección optimizada
    while (!entrada.atEnd()) {
        if (entrada.readLine().contains("FINAL GEOMETRY OBTAINED")) {
            break;
        }
    }

    // 2. Consumir las 3 líneas fijas de metadatos que van justo después
    if (!entrada.atEnd()) entrada.readLine(); // Keywords (AM1 PRECISE)
    if (!entrada.atEnd()) entrada.readLine(); // Comentario
    if (!entrada.atEnd()) entrada.readLine(); // Línea en blanco obligatoria

    int contadorId = 1;
    // Expresión regular: Captura Símbolo(1), X(2), Y(3), Z(4). Soporta flags como "+1" o "1"
    QRegularExpression regex(R"(^\s*([A-Za-z]{1,2})\s+(-?\d+\.?\d*)\s+(?:\+?\d)\s+(-?\d+\.?\d*)\s+(?:\+?\d)\s+(-?\d+\.?\d*))");

    // 3. Parsear el bloque geométrico línea a línea hasta que termine
    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();
        if (linea.isEmpty()) break; // El bloque de átomos termina en una línea vacía

        QRegularExpressionMatch match = regex.match(linea);
        if (match.hasMatch()) {
            Atomo atomo;
            atomo.id      = contadorId++;
            atomo.simbolo = match.captured(1);

            // Escalamos de Ångstroms a píxeles del lienzo gráfico
            float x = match.captured(2).toFloat() * FACTOR_ANGSTROM;
            float y = match.captured(3).toFloat() * FACTOR_ANGSTROM;
            float z = match.captured(4).toFloat() * FACTOR_ANGSTROM;
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }
    }

    if (m_listaAtomos.isEmpty()) {
        return "Error: No se pudieron parsear los átomos de la geometría final.";
    }

    // ─── INYECCIÓN DE SEGURIDAD GEOMÉTRICA PARA EL ARCHIVO .ARC ───
    QVector3D acumulador(0.0f, 0.0f, 0.0f);
    for (const Atomo& a : m_listaAtomos) {
        acumulador += a.posicion;
    }
    QVector3D centroDeMasasArc = acumulador / m_listaAtomos.size();

    for (Atomo& a : m_listaAtomos) {
        a.posicion -= centroDeMasasArc;
    }

    return ""; // Éxito
}

// PARSER .AUX: Archivo extenso con las coordenadas de la trayectoria
QString DatosIO::parseMopacAux(QTextStream& entrada, QVector<Trayectoria>& m_trayectoria, QString& comentario, QString& argumentos) {
    if (entrada.atEnd()) return "Archivo .aux vacío.";

    QStringList plantillaSimbolos;
    bool procesandoAtomos = false;
    bool procesandoCoordenadas = false;
    QVector<float> coordsEtapa;

    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();
        if (linea.isEmpty()) continue;

        if (linea.startsWith("TITLE=")) {
            comentario = linea.section('=', 1).trimmed().remove('"');
            procesandoAtomos = false;
            procesandoCoordenadas = false;
            continue;
        }
        else if (linea.startsWith("KEYWORDS=")) {
            argumentos = linea.section('=', 1).trimmed().remove('"');
            procesandoAtomos = false;
            procesandoCoordenadas = false;
            continue;
        }
        else if (linea.startsWith("ATOM_EL[")) {
            procesandoAtomos = true;
            procesandoCoordenadas = false;
            continue;
        }
        else if (linea.startsWith("ATOM_X_UPDATED:ANGSTROMS[")) {
            // FACTORIZADO: Procesamos el fotograma acumulado mediante la función utilitaria limpia
            procesarYCentrarFotograma(plantillaSimbolos, coordsEtapa, m_trayectoria);
            coordsEtapa.clear();
            procesandoCoordenadas = true;
            procesandoAtomos = false;
            continue;
        }
        else if (linea.contains('=')) {
            procesandoAtomos = false;
            procesandoCoordenadas = false;
        }

        if (procesandoAtomos) {
            plantillaSimbolos.append(linea.simplified().split(' ', Qt::SkipEmptyParts));
        }
        else if (procesandoCoordenadas) {
            QStringList tokens = linea.simplified().split(' ', Qt::SkipEmptyParts);
            for (const QString& token : tokens) {
                coordsEtapa.append(token.toFloat());
            }
        }
    }

    // FACTORIZADO: Procesamiento del último fotograma residual con la misma función común
    procesarYCentrarFotograma(plantillaSimbolos, coordsEtapa, m_trayectoria);

    if (plantillaSimbolos.isEmpty()) return "Error: Falta sección 'ATOM_EL:'.";
    if (m_trayectoria.isEmpty()) return "Error: Falta sección 'ATOM_X_UPDATED:'.";

    return "";
}

void DatosIO::procesarYCentrarFotograma(const QStringList& plantillaSimbolos, const QVector<float>& coordsEtapa, QVector<Trayectoria>& m_trayectoria) {
    if (coordsEtapa.isEmpty() || plantillaSimbolos.isEmpty()) return;

    Trayectoria fotograma;
    int ptr = 0;
    QVector3D acumulador(0.0f, 0.0f, 0.0f);

    // DETERMINACIÓN DE LA ESCALA: Si los valores ya vienen pre-escalados, el factor es 1.0f
    float factorEscalaReal = (coordsEtapa.size() > 0 && qAbs(coordsEtapa[0]) < 15.0f) ? 100.0f : 1.0f;

    for (int i = 0; i < plantillaSimbolos.size(); ++i) {
        if (ptr + 2 >= coordsEtapa.size()) break;
        Atomo atomo;
        atomo.id = i + 1;
        atomo.simbolo = plantillaSimbolos[i];

        // Proyección acotada a píxeles estables
        atomo.posicion = QVector3D(
            coordsEtapa[ptr]   * factorEscalaReal,
            coordsEtapa[ptr+1] * factorEscalaReal,
            coordsEtapa[ptr+2] * factorEscalaReal
            );
        ptr += 3;
        acumulador += atomo.posicion;
        fotograma.listaAtomos.append(atomo);
    }

    if (!fotograma.listaAtomos.isEmpty()) {
        QVector3D centroDeMasas = acumulador / fotograma.listaAtomos.size();
        for (Atomo& a : fotograma.listaAtomos) {
            a.posicion -= centroDeMasas; // Clavado en el centro del lienzo (0,0)
        }
        m_trayectoria.append(fotograma);
    }
}

QVector<Frecuencias> DatosIO::parserFrecuencias(const QString& ruta) {
    QVector<Frecuencias> resultadoEspectro;

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return resultadoEspectro;
    }

    QTextStream entrada(&archivo);
    QString todoElTexto = entrada.readAll();
    archivo.close();

    // 1. ANCLAJE: Localizamos el inicio real de la tabla analítica en el .out
    int indexAnalisis = todoElTexto.indexOf("NORMAL COORDINATE ANALYSIS");
    if (indexAnalisis == -1) {
        return resultadoEspectro;
    }

    QString seccionFreq = todoElTexto.mid(indexAnalisis);
    QTextStream lector(&seccionFreq);
    lector.readLine(); // Consumimos el título

    // Expresión regular para aislar números decimales individuales
    QRegularExpression regexDigitos(R"((-?\d+\.\d+))");

    // Expresión regular para cazar las líneas de coordenadas de los átomos (ID + 3 Columnas)
    QRegularExpression regexFilaDesplazamiento(R"(^\s*(\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+))");

    QString filaValores = "";
    QStringList lineasEstructura;

    // 2. ESCANEO DEL BLOQUE COMPLETO DE MODOS NORMALES
    while (!lector.atEnd()) {
        QString linea = lector.readLine();
        QString lineaTrimmed = linea.trimmed();

        // Paramos si empieza el siguiente bloque de MOPAC
        if (linea.contains("MASS-WEIGHTED COORDINATE ANALYSIS") || linea.contains("FORCE CONSTANT IN CARTESIAN")) {
            break;
        }

        if (lineaTrimmed.isEmpty()) continue;

        // Capturamos de forma dinámica la fila de valores de cm⁻¹ si no la teníamos ya
        if (filaValores.isEmpty() && !lineaTrimmed.startsWith("Root") &&
            !lineaTrimmed.contains("A'") && !lineaTrimmed.contains("A\"") &&
            !lineaTrimmed.contains("Ag") && !lineaTrimmed.contains("B1u") && !lineaTrimmed.contains("B2u") &&
            regexDigitos.match(lineaTrimmed).hasMatch()) {

            filaValores = lineaTrimmed;
            continue;
        }

        // Acumulamos las filas de datos numéricos de los átomos
        QRegularExpressionMatch matchAtomo = regexFilaDesplazamiento.match(linea);
        if (matchAtomo.hasMatch()) {
            lineasEstructura.append(linea);
        }
    }

    // 3. PROCESADO E INYECCIÓN DE METADATOS DE FRECUENCIAS (cm⁻¹)
    QRegularExpressionMatchIterator itFreq = regexDigitos.globalMatch(filaValores);
    QVector<int> indicesColumnasValidas; // Guarda la posición de las columnas que son > 10 cm⁻¹
    int columnaCandidata = 0;

    while (itFreq.hasNext()) {
        QRegularExpressionMatch match = itFreq.next();
        float valorCm1 = match.captured(1).toFloat();

        if (valorCm1 > 10.0f) {
            Frecuencias ficha;
            ficha.frecuencia = valorCm1;
            resultadoEspectro.append(ficha);
            indicesColumnasValidas.append(columnaCandidata);
        }
        columnaCandidata++;
    }

    if (resultadoEspectro.isEmpty() || lineasEstructura.isEmpty()) {
        return resultadoEspectro;
    }

    // 4. EXTRACCIÓN MODULAR EN MATRIZ DE LAS VIBRACIONES (dx, dy, dz)
    // Inicializamos la rejilla de vibraciones para cada frecuencia válida detectada
    for (int i = 0; i < resultadoEspectro.size(); ++i) {
        // Necesitamos tantas fichas de vibración como átomos se hayan leído (tamaño de líneas / 3 ejes)
        int totalAtomos = lineasEstructura.size() / 3;
        for (int a = 0; a < totalAtomos; ++a) {
            Vibraciones v;
            v.id_atomo = a + 1; // Asignamos el ID correlativo base 1
            v.dx = 0.0f; v.dy = 0.0f; v.dz = 0.0f;
            resultadoEspectro[i].vibraciones.append(v);
        }
    }

    // Procesamos el sándwich de 3 ejes (X, Y, Z) por cada átomo
    for (int i = 0; i < lineasEstructura.size(); ++i) {
        QRegularExpressionMatch match = regexFilaDesplazamiento.match(lineasEstructura[i]);
        if (match.hasMatch()) {
            int idFilaMatriz = match.captured(1).toInt();
            int idAtomoMapeado = ((idFilaMatriz - 1) / 3); // Agrupa las líneas de 3 en 3 por átomo
            int ejeActivo = (idFilaMatriz - 1) % 3;        // 0 = X, 1 = Y, 2 = Z

            // Extraemos los tres valores numéricos que MOPAC imprimió en esta fila
            float col1 = match.captured(2).toFloat();
            float col2 = match.captured(3).toFloat();
            float col3 = match.captured(4).toFloat();
            float columnasCrudas[3] = { col1, col2, col3 };

            // Inyectamos el desplazamiento en la frecuencia correspondiente mapeando su columna
            for (int f = 0; f < resultadoEspectro.size(); ++f) {
                int columnaMopacTarget = indicesColumnasValidas[f];

                // Red de seguridad por si MOPAC vuelca menos columnas de las esperadas
                if (columnaMopacTarget >= 3) continue;

                float valorDesplazamiento = columnasCrudas[columnaMopacTarget];

                // Asignamos el diferencial al eje tridimensional correspondiente
                if (ejeActivo == 0) resultadoEspectro[f].vibraciones[idAtomoMapeado].dx = valorDesplazamiento;
                if (ejeActivo == 1) resultadoEspectro[f].vibraciones[idAtomoMapeado].dy = valorDesplazamiento;
                if (ejeActivo == 2) resultadoEspectro[f].vibraciones[idAtomoMapeado].dz = valorDesplazamiento;
            }
        }
    }

    return resultadoEspectro;
}

QString DatosIO::parseAuxForce(QTextStream& entrada, QVector<Atomo>& m_listaAtomos) {
    if (entrada.atEnd()) return "Archivo .aux de fuerzas vacío.";

    m_listaAtomos.clear();
    QStringList plantillaSimbolos;
    QVector<float> coordsFuerza;

    bool procesandoAtomos = false;
    bool procesandoCoordenadas = false;

    while (!entrada.atEnd()) {
        QString linea = entrada.readLine().trimmed();
        if (linea.isEmpty()) continue;

        if (linea.startsWith("ATOM_EL[")) {
            procesandoAtomos = true;
            procesandoCoordenadas = false;
            continue;
        }
        else if (linea.startsWith("ATOM_X_FORCE:ANGSTROMS[")) {
            procesandoCoordenadas = true;
            procesandoAtomos = false;
            continue;
        }
        else if (linea.contains('=')) {
            procesandoAtomos = false;
            procesandoCoordenadas = false;
        }

        if (procesandoAtomos) {
            plantillaSimbolos.append(linea.simplified().split(' ', Qt::SkipEmptyParts));
        }
        else if (procesandoCoordenadas) {
            QStringList tokens = linea.simplified().split(' ', Qt::SkipEmptyParts);
            for (const QString& token : tokens) {
                coordsFuerza.append(token.toFloat());
            }
        }
    }

    if (plantillaSimbolos.isEmpty()) return "Error FORCE: Falta sección 'ATOM_EL:'.";
    if (coordsFuerza.isEmpty()) return "Error FORCE: Falta sección 'ATOM_X_FORCE:'.";

    // Reconstrucción e inyección tridimensional indexada
    int ptr = 0;
    QVector3D acumulador(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < plantillaSimbolos.size(); ++i) {
        if (ptr + 2 >= coordsFuerza.size()) break;

        Atomo atomo;
        atomo.id = i + 1;
        atomo.simbolo = plantillaSimbolos[i];

        // Escalamos de Ångstroms a píxeles estables de la vista
        float x = coordsFuerza[ptr]   * FACTOR_ANGSTROM;
        float y = coordsFuerza[ptr+1] * FACTOR_ANGSTROM;
        float z = coordsFuerza[ptr+2] * FACTOR_ANGSTROM;

        atomo.posicion = QVector3D(x, y, z);
        ptr += 3;

        acumulador += atomo.posicion;
        m_listaAtomos.append(atomo);
    }

    // Centralización geométrica obligatoria contra el origen del lienzo
    if (!m_listaAtomos.isEmpty()) {
        QVector3D centroDeMasas = acumulador / m_listaAtomos.size();
        for (Atomo& a : m_listaAtomos) {
            a.posicion -= centroDeMasas;
        }
    }

    return "";
}


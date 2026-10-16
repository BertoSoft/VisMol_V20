#include "datosio.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

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
    TipoFormato formato,
    QString& texto
    ) {

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: No se pudo abrir el archivo MOPAC %1").arg(ruta);
    }

    QTextStream entrada(&archivo);

    // Inicialización y limpieza transaccional de los contenedores
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    comentario = "";
    argumentos = "";

    // =========================================================================
    // EXTRACCIÓN MODULAR DEL CONTENIDO COMPLETO DEL LOG
    // El QTextStream mantiene su puntero libre porque leemos usando el QFile
    // =========================================================================
    if (formato == TipoFormato::OUT || formato == TipoFormato::ARC) {
        // Guardamos la posición actual del stream por si los parsers la necesitan
        qint64 posicionOriginal = archivo.pos();

        // Volcamos todo el texto plano para la consola de la UI
        archivo.seek(0);
        texto = entrada.readAll();

        // Restauramos el puntero para que los parsers lean desde el principio
        archivo.seek(posicionOriginal);
    }

    QString resultado = "";

    if(formato == TipoFormato::DAT){
        resultado = parseDAT(entrada, m_listaAtomos, comentario, argumentos);
    }
    if(formato == TipoFormato::OUT){
        resultado = parseOUT(entrada, m_listaAtomos);
    }
    if(formato == TipoFormato::ARC){
        resultado = parseARC(entrada, m_listaAtomos);
    }

    archivo.close();
    return resultado;
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

    // 1. Validar que el cálculo terminó con éxito buscando el marcador de MOPAC
    int indexFinTrabajo = todoElTexto.indexOf("* JOB ENDED NORMALLY *");
    if (indexFinTrabajo == -1) {
        return "El archivo .out no indica una finalización normal de MOPAC ('* JOB ENDED NORMALLY *').";
    }

    // 2. Extraer todo el texto desde el inicio hasta el marcador de fin
    QString textoHastaFin = todoElTexto.left(indexFinTrabajo);

    // 3. Buscar la PRIMERA aparición de "CARTESIAN COORDINATES" yendo hacia atrás
    int indexGeometriaFinal = textoHastaFin.lastIndexOf("CARTESIAN COORDINATES");
    if (indexGeometriaFinal == -1) {
        return "No se encontró el bloque 'CARTESIAN COORDINATES' previo a la finalización del trabajo.";
    }

    // 4. Aislar la sección que contiene la geometría final optimizada
    QString seccionFinal = textoHastaFin.mid(indexGeometriaFinal);
    QTextStream lector(&seccionFinal);

    lector.readLine(); // Consumir cabecera "CARTESIAN COORDINATES"

    bool encontradoBloqueDatos = false;
    // Expresión regular flexible para absorber la separación por columnas de MOPAC
    // Captura: 1: ID Atómico (opcional), 2: Símbolo químico, 3: X, 4: Y, 5: Z
    QRegularExpression regex(R"(^\s*(\d+)?\s+([A-Za-z]{1,2})\s+(-?\d+\.?\d*)\s+(-?\d+\.?\d*)\s+(-?\d+\.?\d*))");
    int contadorIdFallback = 1;

    while (!lector.atEnd()) {
        QString linea = lector.readLine();
        QString lineaTrimmed = linea.trimmed();

        // Si ya encontramos datos y viene una línea vacía, el bloque geométrico cerró
        if (encontradoBloqueDatos && lineaTrimmed.isEmpty()) {
            break;
        }

        // Saltamos cabeceras de texto o líneas vacías iniciales antes de los datos
        if (lineaTrimmed.isEmpty() || lineaTrimmed.startsWith("NO.") || lineaTrimmed.startsWith("ATOM")) {
            continue;
        }

        QRegularExpressionMatch match = regex.match(linea);
        if (match.hasMatch()) {
            encontradoBloqueDatos = true;
            Atomo atomo;

            // Si el archivo no provee un ID limpio por fila, usamos el autoincremental
            QString idCapturado = match.captured(1);
            if (idCapturado.isEmpty()) {
                atomo.id = contadorIdFallback++;
            } else {
                atomo.id = idCapturado.toInt();
            }

            atomo.simbolo = match.captured(2);

            // Restaura los Ångstroms optimizados de la salida cuántica a píxeles exactos del lienzo
            float x = match.captured(3).toFloat() * FACTOR_ANGSTROM;
            float y = match.captured(4).toFloat() * FACTOR_ANGSTROM;
            float z = match.captured(5).toFloat() * FACTOR_ANGSTROM;
            atomo.posicion = QVector3D(x, y, z);

            m_listaAtomos.append(atomo);
        }
    }

    if (m_listaAtomos.isEmpty()) {
        return "Error: Se localizó el bloque geométrico final optimizado pero no se pudieron parsear sus átomos.";
    }

    // =========================================================================
    // ─── ANCLAJE GEOMÉTRICO: RE-CENTRADO ABSOLUTO DEL MODELO EN EL VISOR ───
    // Como MOPAC altera el centro de masas, calculamos el desvío y devolvemos
    // la molécula al origen geométrico del lienzo (0,0,0) de píxeles puros.
    // =========================================================================
    QVector3D acumulador(0.0f, 0.0f, 0.0f);
    for (const Atomo& a : m_listaAtomos) {
        acumulador += a.posicion;
    }
    QVector3D centroDeMasasMopac = acumulador / m_listaAtomos.size();

    // Restamos el centro de masas a cada átomo para re-centrar el sistema de coordenadas
    for (Atomo& a : m_listaAtomos) {
        a.posicion -= centroDeMasasMopac;
    }

    return ""; // Éxito absoluto y corregido
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
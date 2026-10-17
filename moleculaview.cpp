#include "moleculaview.h"
#include "atomos.h"
#include "datosio.h"

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QTimer>


// =========================================================================
// 1. ZONA: CICLO DE VIDA DEL CEREBRO DE NEGOCIO
// =========================================================================

MoleculaView::MoleculaView(QObject *parent)
    : QObject(parent){

    // Iniciar todo de fábrica usando el nuevo método centralizado
    resetVariablesClase();

    // Establecemos el estado de salud inicial del lienzo
    m_modoLienzo.estaVacio      = true;
    m_modoLienzo.estaGuardado   = false;
    m_modoLienzo.estaIniciado   = false;
}

MoleculaView::~MoleculaView(){
    // ─── SEGURIDAD EN EL CIERRE DE LA APLICACIÓN ───
    // Si el usuario cierra el visor molecular por completo, nos aseguramos
    // de limpiar la RAM y fulminar el proceso de fondo para no dejar zombis en la CPU.
    if (m_procesoMopac) {
        if (m_procesoMopac->state() != QProcess::NotRunning) {
            m_procesoMopac->kill(); // Cierre fulminante e instantáneo sin esperas
        }
        // No hace falta delete porque al pasarle 'this' como padre, Qt lo destruye solo,
        // pero dejar el puntero a nulo es una buena práctica de higiene.
        m_procesoMopac = nullptr;
    }
}


// =========================================================================
// 2. ZONA: FUNCIONES PÚBLICAS (CONTROL DE ESTADOS Y HERRAMIENTAS)
// =========================================================================
void MoleculaView::setModoEditor(const ModoEditor& nuevoModo){

    if(getModoEditor() == nuevoModo) return;

    m_modoActual = nuevoModo;

    if(m_idAtomoSeleccionado != -1){
        m_idAtomoSeleccionado = -1;
    }

    emit actualizarLienzo();
    emit modoEditorCambiado(m_modoActual);
    setMensajeEstado();
}

void MoleculaView::setElementoActivo(const QString& simbolo){

    if(getSimboloAtomoActivo() == simbolo) return;

    if (m_idAtomoSeleccionado != -1) {
        m_idAtomoSeleccionado = -1;
        emit actualizarLienzo();
    }

    m_atomoActivo = simbolo;
    setMensajeEstado();
}

void MoleculaView::setComentario(const QString& comentario){
    m_comentario = comentario.trimmed();
}

void MoleculaView::setArgumentos(const QString& argumentos){
    m_argumentos = argumentos;
}

void MoleculaView::setNuevoProyecto(){
    limpiarLienzo();

    ModoLienzo modo;

    modo.estaGuardado   = false;
    modo.estaIniciado   = true;
    modo.estaVacio      = true;

    // Limpieza profunda del historial al cerrar, limpiar o cambiar de lienzo
    m_atras.clear();
    m_adelante.clear();

    setModoEditor(ModoSeleccion);
    setModoLienzo(modo);
}

void MoleculaView::deshacer() {
    if (m_atras.isEmpty()) return;

    pushAdelante();
    popAtras();

    // Forzamos un estado temporal que notifique que el archivo ha cambiado respecto al disco
    ModoLienzo modoTmp = m_modoLienzo;
    modoTmp.estaGuardado = false;
    setModoLienzo(modoTmp);
}

void MoleculaView::rehacer() {
    if (m_adelante.isEmpty()) return;

    FotoActual fotoPresente;
    fotoPresente.listaAtomos  = m_listaAtomos;
    fotoPresente.listaEnlaces = m_listaEnlaces;
    fotoPresente.contadorIds  = m_contadorIds;
    m_atras.append(fotoPresente);

    popAdelante();

    // Mismo principio: avanzar al futuro alternativo rompe la simetría del archivo en disco
    ModoLienzo modoTmp = m_modoLienzo;
    modoTmp.estaGuardado = false;
    setModoLienzo(modoTmp);
}

// =========================================================================
// 3. ZONA: FUNCIONES PÚBLICAS (PERSISTENCIA Y ENTRADA/SALIDA UNIFICADA)
// =========================================================================


QString MoleculaView::abrirArchivo(const QString& ruta){
    if (ruta.isEmpty()) {
        return tr("La ruta del archivo especificada está vacía.");
    }

    // 1. Clasificamos el fichero de forma autónoma inspeccionando sus marcas internas
    TipoFormato formatoDeteccion = getTipoArchivo(ruta);
    QString respuesta = "";

    // 2. ENRUTAMIENTO DIRECTO SEGURO SEGÚN EL CONTENIDO REAL DETECTADO
    if (formatoDeteccion == TipoFormato::VISMOL) {
        respuesta = loadFileVisMol(ruta); // Carga nativa pura (Geometría + Conectividad)
    }
    else if (formatoDeteccion == TipoFormato::OUT || formatoDeteccion == TipoFormato::ARC || formatoDeteccion == TipoFormato::DAT) {
        // Redirigimos de forma unificada hacia el cargador transaccional unificado para MOPAC
        respuesta = loadFileMOPAC(ruta, formatoDeteccion);
    }
    else if(formatoDeteccion == TipoFormato::AUX){
        // Redirigimos a archivos .AUX







    }
    else {
        // Si devuelve "*.*" o un formato no mapeado por las firmas, abortamos elegantemente
        return tr("El archivo seleccionado no contiene ninguna firma molecular o formato reconocido por el sistema.");
    }

    // 3. CONTROL DE ÉXITO DE LA TRANSACCIÓN:
    // Si la respuesta es vacía "" o es la advertencia controlada de topología, registramos la ruta activa
    if (respuesta.isEmpty() || respuesta == "AVISO_TOPOLOGIA") {
        m_rutaArchivoActual = ruta;
    }

    return respuesta;

}

QString MoleculaView::guardarArchivo(const QString& ruta){
    if (ruta.isEmpty()) {
        return tr("La ruta del archivo especificada está vacía.");
    }

    // 1. Clasificamos el fichero de forma autónoma inspeccionando sus marcas internas
    TipoFormato formatoDeteccion = getTipoArchivo(ruta);
    QString respuesta = "";

    // 2. ENRUTAMIENTO DIRECTO CORREGIDO
    if (formatoDeteccion == TipoFormato::VISMOL) {
        respuesta = saveFileVisMol(ruta); // <-- CORREGIDO: Llama a save, no a load
    }
    else if (formatoDeteccion == TipoFormato::OUT || formatoDeteccion == TipoFormato::ARC || formatoDeteccion == TipoFormato::DAT) {
        respuesta = saveFileMOPAC(ruta, m_argumentos);
    }
    else {
        // Fallback defensivo por extensión si el archivo es nuevo y no tiene marcas aún
        if (ruta.endsWith(".vismol", Qt::CaseInsensitive)) {
            respuesta = saveFileVisMol(ruta);
        } else {
            respuesta = saveFileMOPAC(ruta, m_argumentos);
        }
    }

    // 3. CONTROL DE ÉXITO DE LA TRANSACCIÓN:
    if (respuesta.isEmpty() || respuesta == "AVISO_TOPOLOGIA") {
        m_rutaArchivoActual = ruta;
    }

    return respuesta;
}

// =========================================================================
// 4. ZONA: FUNCIONES PÚBLICAS (INTERACCIÓN CON EL ESCENARIO / RATÓN)
// =========================================================================

void MoleculaView::setClick(const QVector3D& posClick){
    switch(m_modoActual) {
    // --- GRUPO 1: MODOS DE DIBUJO DE ÁTOMOS ---
    case ModoDibujo:
        // Delegamos en una función interna que ya dejamos programada
        procesarClickDibujoAtomo(posClick);
        break;

    // --- GRUPO 2: MODO SELECCIÓN Y CONSULTA ---
    case ModoSeleccion:
        // LO QUE VENDRÁ: Aquí buscaremos si en 'posClick' hay algún átomo
        // para seleccionarlo, medir distancias o activar el clic derecho.
        procesarClickSeleccion(posClick);
        break;

    // --- GRUPO 3: MODO CREAR ENLACES ---
    case ModoCrearEnlace:
        // LO QUE VENDRÁ: Aquí tiraremos líneas entre el átomo activo y el pulsado
        procesarClickCrearEnlace(posClick);
        break;

    // --- GRUPO 4: MODO ROTACIÓN / CÁMARA 3D ---
    case ModoRotacion3D:
        // LO QUE VENDRÁ: Manejo del pivot o cámara en el espacio 3D
        procesarClickRotacion(posClick);
        break;

    default:
        return;
    }
}

void MoleculaView::setClickDerecho(const QVector3D& posClickDerecho){

    // 1. Buscamos primero si hay un átomo bajo el cursor
    int idAtomo     = getIdAtomoFromPos(posClickDerecho);
    int idEnlace    = -1; // De momento mientras no medimos distancias

    // 2. Si NO hay ningún átomo, buscamos si el usuario ha pulsado sobre una línea (enlace)
    if (idAtomo < 0) {
        const float TOLERANCIA_CLIC_ENLACE = 12.0f; // Margen de error en píxeles/unidades para acertar a la línea
        float distanciaMinima = TOLERANCIA_CLIC_ENLACE;

        for (int i = 0; i < m_listaEnlaces.size(); ++i) {
            const Enlace& enlace = m_listaEnlaces[i];

            // Recuperamos las coordenadas de los dos átomos que forman el enlace
            QVector3D pos1, pos2;
            bool e1 = false, e2 = false;
            for (const Atomo& a : m_listaAtomos) {
                if (a.id == enlace.id_atomo1) { pos1 = a.posicion; e1 = true; }
                if (a.id == enlace.id_atomo2) { pos2 = a.posicion; e2 = true; }
                if (e1 && e2) break;
            }

            if (e1 && e2) {
                // Medimos la distancia geométrica desde el clic hasta este segmento de enlace
                float dist = getDistanciaPuntoSegmento(posClickDerecho, pos1, pos2);
                if (dist < distanciaMinima) {
                    distanciaMinima = dist;
                    idEnlace = i; // Guardamos el índice del enlace más cercano
                }
            }
        }
    }


    if(idAtomo>=0 || idEnlace>=0){
        emit setMenuEliminar(idAtomo, idEnlace);
    }
}

void MoleculaView::simularMopac(){
    if (m_listaAtomos.isEmpty()) return;

    QString folderTemporal = QDir::tempPath();
    QString baseName = "vismol_calc";
    QString rutaTemporalBase = folderTemporal + "/" + baseName;
    QString rutaDat = rutaTemporalBase + ".dat";
    QString rutaOut = rutaTemporalBase + ".out";

    // 1. PISTA LIMPIA: Borramos los archivos de la simulación anterior
    QFile::remove(rutaDat);
    QFile::remove(rutaOut);

    // 2. ESCRIBIMOS LA ENTRADA CUÁNTICA
    if (!saveFileMOPAC(rutaDat, m_argumentos).isEmpty()) return;

    // 3. ASIGNACIÓN SEGURA REUTILIZANDO LA VARIABLE DE CLASE
    m_procesoMopac = new QProcess(this);

    m_procesoMopac->setWorkingDirectory(folderTemporal);

    // 3.5 ─── CABLE DE ARRANQUE: Forzar el refresco de la UI al iniciar el proceso
    connect(m_procesoMopac, &QProcess::started, this, [this](){
        setModoLienzo(m_modoLienzo);
    });

    // 4. CONFIGURAMOS EL TEMPORIZADOR DE VOLCADO (1 SEGUNDO LECTURA PASIVA)
    QTimer* timerMonitor = new QTimer(this);
    connect(timerMonitor, &QTimer::timeout, this, [this, rutaOut]() {
        QString textoActual = DatosIO::leerLogTexto(rutaOut);
        if (!textoActual.isEmpty()) {
            emit notificaContenidoLog(textoActual);
        }
    });

    // 5. MÁQUINA DE EVENTOS AL TERMINAR (ESTANCA Y SIN RECURSIVIDAD)
    connect(m_procesoMopac, &QProcess::finished, this, [this, rutaTemporalBase, rutaOut, timerMonitor](int exitCode, QProcess::ExitStatus exitStatus) {

        // Frenamos y destruimos el reloj de lectura inmediatamente al finalizar
        timerMonitor->stop();
        timerMonitor->deleteLater();

        emit notificarMensajeEstado(tr(" Simulación cuántica finalizada."));

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Último volcado definitivo para el log de la UI
            emit notificaContenidoLog(DatosIO::leerLogTexto(rutaOut));

            // CARGAMOS LA GEOMETRÍA OPTIMIZADA RECENTRADA CON ENLACES EN LA RAM
            abrirArchivo(rutaOut);
        } else {
            emit notificaContenidoLog(tr(" > ❌ ERROR: MOPAC abortó o no generó el archivo .out."));
        }

        // LIMPIEZA DE RESIDUOS CRÍTICOS EN DISCO
        QFile::remove(rutaTemporalBase + ".dat");
        QFile::remove(rutaTemporalBase + ".out");
        m_rutaArchivoActual = "";
    });

    // 6. LANZAMIENTO CIEGO ASÍNCRONO INDESTRUCTIBLE
    // MOPAC exige estrictamente solo el nombre base, sin la extensión ".dat"
    m_procesoMopac->start("mopac", QStringList() << baseName);
    m_procesoMopac->closeWriteChannel(); // Bloquea esperas de teclado accidentales

    // Arrancamos el temporizador justo después de lanzar el proceso de fondo
    timerMonitor->start(100);

}

void MoleculaView::detenerSimularMopac() {
    // Verificamos que el puntero sea válido y que el proceso realmente se esté ejecutando
    if (m_procesoMopac && m_procesoMopac->state() != QProcess::NotRunning) {

        // 1. Solicitamos un cierre elegante al sistema operativo
        m_procesoMopac->terminate();

        // 2. Red de seguridad: si tarda más de 2 segundos en responder, lo forzamos (kill)
        if (!m_procesoMopac->waitForFinished(2000)) {
            m_procesoMopac->kill();
        }

        emit notificarMensajeEstado(tr(" Simulación cancelada por el usuario."));
        emit notificaContenidoLog(tr(" > ❌ SIMULACIÓN ABORTADA."));
    }
}

// =========================================================================
// 5. ZONA: FUNCIONES PÚBLICAS (GESTOR DE MEMORIA Y ELIMINACIÓN)
// =========================================================================

void MoleculaView::delAtomo(const int& idAtomo){

    // Guarda la memoria
    pushAtras();

    // 1.- INTEGRIDAD QUÍMICA CRÍTICA: Eliminamos los enlaces recorriendo el vector al revés
    for (int i = m_listaEnlaces.size() - 1; i >= 0; --i) {
        if (m_listaEnlaces[i].id_atomo1 == idAtomo || m_listaEnlaces[i].id_atomo2 == idAtomo) {
            m_listaEnlaces.removeAt(i);
        }
    }

    // 2.- Buscamos y eliminamos el átomo del vector en memoria
    for (int i = 0; i < m_listaAtomos.size(); i++) {
        if (m_listaAtomos[i].id == idAtomo) {
            m_listaAtomos.removeAt(i);
            break;
        }
    }

    // 3.- Si eliminamos un átomo preseleccionado, limpiamos la selección
    if (m_idAtomoSeleccionado == idAtomo) {
        m_idAtomoSeleccionado = -1;
    }

    // 4.- Notificamos centralizadamente para redibujar
    setModoLienzo(m_modoLienzo);
}

void MoleculaView::delEnlace(const int& idEnlace){

    // Guarda la memoria
    pushAtras();

    // Buscamos el enlace por las identificaciones de sus átomos (o si tuviera un id único)
    // En tu estructura actual, si pasamos el índice directo del vector de enlaces:
    if (idEnlace >= 0 && idEnlace < m_listaEnlaces.size()) {
        m_listaEnlaces.removeAt(idEnlace);

        // ¡NUEVO!: Si se elimina un enlace por el menú contextual, limpiamos la selección por seguridad
        m_idAtomoSeleccionado = -1;

        // Notificamos e invocamos al lienzo para que repinte las líneas inmediatamente
        setModoLienzo(m_modoLienzo);
    }
}

void MoleculaView::limpiarLienzo(){

    // Guarda la memoria
    pushAtras();

    // Reseteo absoluto de la memoria en silencio
    resetVariablesClase();

    // Configuración específica de la salud del documento para el lienzo limpio
    ModoLienzo modo;
    modo.estaVacio      = true;
    modo.estaGuardado   = false;
    modo.estaIniciado   = true; // El lienzo sigue activo y listo para dibujar

    setModoLienzo(modo);
}

void MoleculaView::cerrarProyecto(){
    // Reseteo absoluto de la memoria en silencio
    resetVariablesClase();

    // Limpieza profunda del historial al cerrar, limpiar o cambiar de lienzo
    m_atras.clear();
    m_adelante.clear();

    // Configuración específica para el cierre completo de la sesión
    ModoLienzo modo;
    modo.estaVacio      = true;
    modo.estaGuardado   = false;
    modo.estaIniciado   = false; // Apaga la barra de herramientas lateral

    setModoLienzo(modo);
}

// =========================================================================
// 6. ZONA: GETTERS DE ACCESO A DATOS (ESTRICTO SOLO LECTURA)
// =========================================================================

MoleculaView::ModoEditor MoleculaView::getModoEditor(){

    return m_modoActual;
}

QString MoleculaView::getSimboloAtomoActivo(){

    return m_atomoActivo;
}

QVector<Atomo> MoleculaView::getListaAtomos(){
    return m_listaAtomos;
}

QVector<Enlace> MoleculaView::getListaEnlaces(){
    return m_listaEnlaces;
}

int MoleculaView::getIdAtomoSeleccionado(){
    return m_idAtomoSeleccionado;
}

QString MoleculaView::getComentario(){
    return m_comentario;
}

MoleculaView::ModoLienzo MoleculaView::getModoLienzo(){
    return m_modoLienzo;
}

bool MoleculaView::hayPasado() const {
    return !m_atras.isEmpty();
}

bool MoleculaView::hayFuturo() const {
    return !m_adelante.isEmpty();
}

QString MoleculaView::getRutaArchivoActual(){
    return m_rutaArchivoActual;
}

// =========================================================================
// 8. ZONA: MÉTODOS INTERNOS PRIVADOS (PROCESAMIENTO DE CLICKS GRÁFICOS)
// =========================================================================

void MoleculaView::procesarClickDibujoAtomo(const QVector3D& posClick) {
    if(getIdAtomoFromPos(posClick) < 0){

        // Guarda la memoria
        pushAtras();

        Atomo nuevoAtomo;

        nuevoAtomo.id       = ++m_contadorIds;
        nuevoAtomo.simbolo  = m_atomoActivo;
        nuevoAtomo.posicion = posClick;

        // Introducido directamente aquí: Almacenamos en el vector del negocio
        m_listaAtomos.append(nuevoAtomo);

        // Actualizamos de forma centralizada la salud y estado de la UI
        setModoLienzo(m_modoLienzo);
    }

}

void MoleculaView::procesarClickSeleccion(const QVector3D& posClick) {
    // ─── PASO 1: SI EL ANCLA ESTÁ FUERA DE LA ESCENA (GESTO DE PULSAR) ───
    if (m_ultimaPosRat.x() < -5000.0f) {
        m_ultimaPosRat = posClick; // Registramos la coordenada de la escena inicial de partida
        return;                      // Salimos limpiamente, el arrastre óptico toma el control
    }

    // ─── PASO 2: SI EL ANCLA YA ERA VÁLIDA (GESTO DE SOLTAR) ───
    if (m_ultimaPosRat.x() >= -5000.0f) {
        if (!m_listaAtomos.isEmpty()) {

            // ─── TU DETECCIÓN QUIRÚRGICA: RESTA EXPLÍCITA COMPONENTE A COMPONENTE ───
            // Extraemos los diferenciales planos puros de la pantalla calculando las diferencias
            float deltaX = posClick.x() - m_ultimaPosRat.x();
            float deltaY = posClick.y() - m_ultimaPosRat.y();

            // Construimos el vector tridimensional real de traslación manteniendo Z neutro
            QVector3D vectorTraslacionFinal(deltaX, deltaY, 0.0f);

            // Filtro antipánico de ruido mecánico del hardware (Umbral de 3 píxeles/unidades)
            if (vectorTraslacionFinal.length() > 3.0f) {

                // B. Guardamos una única foto pura del pasado original en el historial (Ctrl+Z)
                pushAtras();

                // C. MUTACIÓN REAL DE COORDENADAS: Movemos todos los átomos de golpe en la RAM
                for (Atomo& atomo : m_listaAtomos) {
                    atomo.posicion += vectorTraslacionFinal;
                }

                // D. Activamos de forma oficial el asterisco de cambios sin guardar en la ventana
                ModoLienzo modoTmp = m_modoLienzo;
                modoTmp.estaGuardado = false;
                setModoLienzo(modoTmp);
            }
        }

        // ─── PASO 3: PROTOCOLO DE RESETEO DE TRANSACCIÓN ───
        // Restauramos la coordenada de control fuera de mapa para el próximo arrastre
        m_ultimaPosRat = QVector3D(-10000.0f, -10000.0f, -10000.0f);
    }
}

void MoleculaView::procesarClickCrearEnlace(const QVector3D& posClick) {
    int idAtomoActual = getIdAtomoFromPos(posClick);
    if(idAtomoActual < 0) return;

    // Guarda la memoria
    pushAtras();


    // =========================================================================
    // FASE 1: SELECCIÓN DEL PRIMER ÁTOMO (ÁTOMO 1)
    // =========================================================================
    if(m_idAtomoSeleccionado < 0){
        Atomo atomoPulsado;
        bool encontrado = false;
        for(const Atomo& a : m_listaAtomos) {
            if(a.id == idAtomoActual) {
                atomoPulsado = a;
                encontrado = true;
                break;
            }
        }

        if(encontrado) {
            int enlacesActuales = getEnlaces(atomoPulsado);
            int valenciaMaxima  = Atomos::getMaxValencia(atomoPulsado.simbolo);

            // CORREGIDO: Eliminamos "|| enlacesActuales == 3".
            // Ahora un Carbono con 3 enlaces (como el de tu carbonilo) SÍ se puede seleccionar
            // porque su valencia máxima es 4 y aún le queda un electrón libre.
            if(enlacesActuales >= valenciaMaxima) {
                return;
            }
        }

        m_idAtomoSeleccionado = idAtomoActual;
        setModoLienzo(m_modoLienzo);
        return;
    }

    // =========================================================================
    // FASE 2: SELECCIÓN DEL SEGUNDO ÁTOMO (ÁTOMO 2) Y CONECTIVIDAD
    // =========================================================================
    int idAtomo1 = m_idAtomoSeleccionado;
    int idAtomo2 = idAtomoActual;

    m_idAtomoSeleccionado = -1; // Limpiamos la selección inmediatamente

    Atomo atomo1, atomo2;
    bool encontrado1 = false, encontrado2 = false;
    for(const Atomo& a : m_listaAtomos) {
        if(a.id == idAtomo1) { atomo1 = a; encontrado1 = true; }
        if(a.id == idAtomo2) { atomo2 = a; encontrado2 = true; }
        if(encontrado1 && encontrado2) break;
    }

    // Evaluamos si ya existe una conexión previa entre estos dos átomos específicos
    for(Enlace &enlace : m_listaEnlaces){
        if((enlace.id_atomo1 == idAtomo1 && enlace.id_atomo2 == idAtomo2) ||
            (enlace.id_atomo1 == idAtomo2 && enlace.id_atomo2 == idAtomo1)) {

            // Aquí es donde SÍ prima el orden de enlace local:
            // Si ya es un enlace triple entre ellos dos, bloqueamos el incremento.
            if(enlace.orden >= 3) return;

            // Verificamos si ambos átomos tienen valencia libre para absorber el nuevo orden (1->2 o 2->3)
            if(isEnlacePosible(atomo1, atomo2)){
                enlace.orden++;
                setModoLienzo(m_modoLienzo);
            }
            return;
        }
    }

    // Si el enlace no existía entre ellos, intentamos crear un enlace simple nuevo (orden 1)
    if(isEnlacePosible(atomo1, atomo2)){

        Enlace nuevoEnlace;
        nuevoEnlace.id_atomo1 = idAtomo1;
        nuevoEnlace.id_atomo2 = idAtomo2;
        nuevoEnlace.orden     = 1;

        m_listaEnlaces.append(nuevoEnlace);
        setModoLienzo(m_modoLienzo);
    }
}

void MoleculaView::procesarClickRotacion(const QVector3D& posClick) {
}

// =========================================================================
// 9. ZONA: MÉTODOS INTERNOS PRIVADOS (GESTIÓN DE HISTORIAL / PILAS / RESET)
// =========================================================================
void MoleculaView::resetVariablesClase(){
    // 1. Vaciar contenedores físicos de memoria
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    m_comentario        = "";
    m_argumentos        = "";
    m_rutaArchivoActual = "";

    // 2. Valores numéricos y de control de fábrica
    m_contadorIds         = 0;
    m_idAtomoSeleccionado = -1;

    // 3. Herramientas y estados por defecto al iniciar
    m_atomoActivo         = "C";
    m_modoActual          = ModoSeleccion;

    //4. Iniciamos a nulo el m_ultimaPosRat
    m_ultimaPosRat = QVector3D(-10000.0f, -10000.0f, -10000.0f);

    m_procesoMopac = nullptr;
}

void MoleculaView::pushAtras() {
    FotoActual foto;

    // Forzamos la asignación explícita para duplicar los datos en un bloque de RAM nuevo
    foto.listaAtomos  = QVector<Atomo>(m_listaAtomos);
    foto.listaEnlaces = QVector<Enlace>(m_listaEnlaces);
    foto.contadorIds  = m_contadorIds;

    m_atras.append(foto);
    m_adelante.clear();
}

void MoleculaView::pushAdelante() {
    FotoActual foto;

    // Forzamos el aislamiento exacto en el futuro
    foto.listaAtomos  = QVector<Atomo>(m_listaAtomos);
    foto.listaEnlaces = QVector<Enlace>(m_listaEnlaces);
    foto.contadorIds  = m_contadorIds;

    m_adelante.append(foto);
}

void MoleculaView::popAtras() {
    if (m_atras.isEmpty()) return;

    // Extraemos el último plato de la pila del pasado
    FotoActual estadoAnterior = m_atras.last();
    m_atras.removeLast();

    // Sobrescribimos las variables de la clase con el pasado recuperado
    m_listaAtomos  = estadoAnterior.listaAtomos;
    m_listaEnlaces = estadoAnterior.listaEnlaces;
    m_contadorIds  = estadoAnterior.contadorIds;
}

void MoleculaView::popAdelante() {
    if (m_adelante.isEmpty()) return;

    // Extraemos el último plato de la pila del futuro
    FotoActual estadoFuturo = m_adelante.last();
    m_adelante.removeLast();

    // Sobrescribimos las variables de la clase con el futuro recuperado
    m_listaAtomos  = estadoFuturo.listaAtomos;
    m_listaEnlaces = estadoFuturo.listaEnlaces;
    m_contadorIds  = estadoFuturo.contadorIds;
}

// =========================================================================
// 10. ZONA: MÉTODOS INTERNOS PRIVADOS (ESTADO DE SALUD Y PARSERS ENCAPSULADOS)
// =========================================================================

void MoleculaView::setModoLienzo(const ModoLienzo& modoLienzo){
    // 1. Calculamos la realidad física de la memoria en este instante
    bool vacioAhora = m_listaAtomos.isEmpty() && m_listaEnlaces.isEmpty();

    // 2. Evaluamos el estado de guardado de forma robusta
    bool guardadoReal = modoLienzo.estaGuardado;

    if (vacioAhora) {
        guardadoReal = false;
    }
    else {
        // DETECCIÓN CRÍTICA: Si la llamada se hizo usando 'm_modoLienzo' directamente
        // significa que proviene de un clic rutinario de ratón (dibujar, enlazar o borrar).
        // Forzamos la pérdida del guardado para que salte el asterisco de inmediato.
        if (&modoLienzo == &m_modoLienzo) {
            guardadoReal = false;
        }
    }

    // 2.5 EVALUAMOS SI MOPAC ESTÁ CORRIENDO EN ESTE INSTANTE EXACTO
    bool mopacCorriendo = (m_procesoMopac != nullptr && m_procesoMopac->state() != QProcess::NotRunning);

    // 3. Asignamos los estados de forma segura en nuestro contenedor oficial de clase
    m_modoLienzo.estaIniciado       = modoLienzo.estaIniciado;
    m_modoLienzo.estaGuardado       = guardadoReal;
    m_modoLienzo.estaVacio          = vacioAhora;
    m_modoLienzo.formato            = modoLienzo.formato;
    m_modoLienzo.estaMopacActivo    = mopacCorriendo;

    // 4. EMISIÓN CENTRALIZADA SINCRÓNICA
    emit modoEditorCambiado(m_modoActual);
    emit modoLienzoCambiado(m_modoLienzo);
    emit actualizarLienzo();

    setMensajeEstado();
}

void MoleculaView::setMensajeEstado(){
    // ─── PRIORIDAD 1: MONITOREO DEL MOTOR EN VIVO ───
    // Si el proceso existe y está corriendo, la barra de estado debe gritarlo con prioridad absoluta
    if (m_procesoMopac && m_procesoMopac->state() != QProcess::NotRunning) {
        emit notificarMensajeEstado(tr(" 🔬 EJECUTANDO SIMULACIÓN CUÁNTICA MOPAC... Detenga el proceso si es necesario."));
        return;
    }

    // ─── PRIORIDAD 2: ESTADO DEL LIENZO VACÍO ───
    if (!m_modoLienzo.estaIniciado || m_modoLienzo.estaVacio) {
        emit notificarMensajeEstado(tr(" 📂 Proyecto Vacío. Cree un nuevo lienzo o abra un archivo molecular para comenzar."));
        return;
    }

    // ─── PRIORIDAD 3: HERRAMIENTA ACTIVA + ESTADO DE CAMBIOS ───
    QString estadoGuardado = m_modoLienzo.estaGuardado ? tr("Sincronizado con disco")
                                                       : tr("⚠️ Cambios sin guardar");

    QString textoHerramienta = "";
    switch(m_modoActual) {
    case MoleculaView::ModoSeleccion:
        textoHerramienta = tr("Modo Selección (Arrastre átomos para trasladar la estructura)");
        break;
    case MoleculaView::ModoDibujo:
        textoHerramienta = tr("Modo Dibujo: Listo para colocar un átomo de %1").arg(m_atomoActivo);
        break;
    case MoleculaView::ModoCrearEnlace:
        textoHerramienta = tr("Modo Enlace: Seleccione dos átomos para unirlos covalentemente");
        break;
    case MoleculaView::ModoRotacion3D:
        textoHerramienta = tr("Modo Rotación 3D: Arrastre en el lienzo para orbitar el espacio cuántico");
        break;
    }

    // Unificamos toda la información contextual en una sola cadena limpia
    emit notificarMensajeEstado(QString(" 🟢 %1 | [%2]").arg(textoHerramienta).arg(estadoGuardado));
}

QString MoleculaView::loadFileVisMol(const QString& ruta){
    if (ruta.isEmpty()) return "Debes especificar un archivo para cargar los datos.";

    // 1. CREAMOS CONTENEDORES TEMPORALES (Si la carga falla, la molécula actual se salva)
    QVector<Atomo>  listaAtomosTemporal;
    QVector<Enlace> listaEnlacesTemporal;
    QString         comentarioTemporal;

    // 2. Invocamos al parseador estático de DatosIO pasándole las referencias temporales
    QString error = DatosIO::loadVisMol(ruta, listaAtomosTemporal, listaEnlacesTemporal, comentarioTemporal);

    // 3. EN CASO DE ERROR, ABORTAMOS INMEDIATAMENTE (Sin tocar la memoria de la clase)
    if (!error.isEmpty()) {
        return error;
    }

    // 4. SI LA CARGA FUE UN ÉXITO ABSOLUTO, RESPALDAMOS Y ACTUALIZAMOS LA APLICACIÓN
    resetVariablesClase(); // Ahora sí, limpiamos con total seguridad

    // Volcamos las estructuras temporales a las variables oficiales de la clase
    m_listaAtomos  = listaAtomosTemporal;
    m_listaEnlaces = listaEnlacesTemporal;
    m_comentario   = comentarioTemporal;

    // Sincronizar el contador interno de IDs buscando el valor más alto cargado
    int maxId = -1;
    for (const Atomo& atomo : m_listaAtomos) {
        if (atomo.id > maxId) {
            maxId = atomo.id;
        }
    }
    m_contadorIds = maxId;

    // Limpieza profunda del historial al cerrar, limpiar o cambiar de lienzo
    m_atras.clear();
    m_adelante.clear();

    // Forzamos el refresco completo de la salud del documento y los widgets de interfaz
    m_rutaArchivoActual = ruta; // <--- Registramos la ruta AQUÍ primero

    ModoLienzo modo;
    modo.estaVacio     = m_listaAtomos.isEmpty();
    modo.estaGuardado  = true; // Sincronizado con el disco recién abierto
    modo.estaIniciado  = true; // Enciende las barras de herramientas
    modo.formato       = TipoFormato::VISMOL;

    setModoLienzo(modo);

    return ""; // Éxito total
}

QString MoleculaView::loadFileMOPAC(const QString& ruta, TipoFormato formato){
    // 1. Contenedores locales reales en memoria para el intercambio transaccional seguro
    QVector<Atomo>  listaAtomosTemporal;
    QVector<Enlace> listaEnlacesTemporal;
    QString         comentarioTemporal;
    QString         argumentosTemporal;
    QString         textoTemporal; // <--- Variable limpia en la pila

    // 2. Invocación segura enviando referencias a objetos reales creados en la pila
    QString errorParser = DatosIO::loadMOPAC(ruta, listaAtomosTemporal, listaEnlacesTemporal,
                                             comentarioTemporal, argumentosTemporal, formato, textoTemporal);

    if (!errorParser.isEmpty()) {
        return errorParser; // Abortamos la carga si el parser físico falla
    }

    // 3. Tras el éxito del procesamiento, limpiamos de forma segura la memoria activa del negocio
    resetVariablesClase();

    m_listaAtomos  = listaAtomosTemporal;
    m_comentario   = comentarioTemporal;
    m_argumentos   = argumentosTemporal;

    // Reajustar el contador secuencial de IDs al valor máximo de los átomos leídos
    int maxId = 0;
    for (const Atomo& a : m_listaAtomos) {
        if (a.id > maxId) maxId = a.id;
    }
    m_contadorIds = maxId;

    // =========================================================================
    // EMISIÓN REACTIVA DIRECTA HACIA LA VISTA
    // =========================================================================
    if (formato == TipoFormato::OUT || formato == TipoFormato::ARC) {
        emit notificaContenidoLog(textoTemporal);
    } else {
        emit notificaContenidoLog("");
    }

    // 4. INTENTO AUTOMÁTICO DE RECUPERACIÓN DE ENLACES (Búsqueda del espejo estructural .vismol)
    QFileInfo info(ruta);
    QString rutaEspejoVismol = info.absolutePath() + "/" + info.baseName() + ".vismol";

    bool topologiaCargadaConExito = false;

    if (QFile::exists(rutaEspejoVismol)) {
        QVector<Atomo>  basuraAtomos; // Contenedor secundario para descartar duplicación geométrica
        QString         basuraComentario;

        // Intentamos extraer exclusivamente la matriz de conectividad [ENLACES]
        QString errorEspejo = DatosIO::loadVisMol(rutaEspejoVismol, basuraAtomos, listaEnlacesTemporal, basuraComentario);
        if (errorEspejo.isEmpty()) {
            m_listaEnlaces = listaEnlacesTemporal;
            topologiaCargadaConExito = true;
        }
    }

    // 5. Reinicialización completa del estado de salud del Lienzo
    m_atras.clear();
    m_adelante.clear();

    m_rutaArchivoActual = ruta; // <--- Registramos la ruta AQUÍ primero

    ModoLienzo modo;
    modo.estaVacio    = m_listaAtomos.isEmpty();
    modo.estaGuardado = true; // El archivo coincide simétricamente con lo almacenado en disco
    modo.estaIniciado = true; // Reactivamos las paletas de interacción lateral de la interfaz
    modo.formato      = formato;

    setModoLienzo(modo);

    // Si la lectura cuántica es exitosa pero no había archivo .vismol para los enlaces,
    // devolvemos tu aviso personalizado para desplegar el diálogo informativo en la Vista.
    if (!topologiaCargadaConExito) {
        return "AVISO_TOPOLOGIA";
    }

    return ""; // Éxito absoluto sin incidentes topológicos
}


QString MoleculaView::saveFileVisMol(const QString& ruta){
    // Control de seguridad inicial para evitar rutas vacías
    if (ruta.isEmpty()) return "Debes especificar una ruta de archivo para guardar los datos.";

    // Invocamos directamente al método estático unificado de DatosIO pasando el estado interno actual
    return DatosIO::saveVisMol(ruta, m_listaAtomos, m_listaEnlaces, m_comentario);
}


QString MoleculaView::saveFileMOPAC(const QString& ruta, const QString& argumentos){

    if (ruta.isEmpty()) return "Debes especificar una ruta de archivo válida.";
    if (argumentos.isEmpty()) return "MOPAC exige al menos una palabra clave (Keyword) en la primera línea de instrucciones.";

    // =========================================================================
    // 1. GENERACIÓN DEL ARCHIVO PRINCIPAL DE ENTRADA MOPAC (.dat / .mop)
    // =========================================================================
    QString errorMopac = DatosIO::saveMOPAC(ruta, m_listaAtomos, m_comentario, argumentos.trimmed().toUpper());

    // Red de seguridad: Si falla la escritura física del .dat, abortamos el flujo
    if (!errorMopac.isEmpty()) {
        return errorMopac;
    }

    // =========================================================================
    // 2. GENERACIÓN AUTOMÁTICA DEL ARCHIVO ESPEJO TOPOLÓGICO (.vismol)
    // =========================================================================
    QFileInfo info(ruta);
    // Construimos la ruta simétrica reemplazando la extensión en el mismo directorio
    QString rutaEspejoVismol = info.absolutePath() + "/" + info.baseName() + ".vismol";

    // Guardamos el estado estructural actual (átomos con sus IDs y enlaces limpios)
    QString errorEspejo = DatosIO::saveVisMol(rutaEspejoVismol, m_listaAtomos, m_listaEnlaces, m_comentario);

    // NOTA DE CONTROL: Si el espejo falla (ej. disco lleno a mitad de proceso),
    // lo notificamos, pero no destruimos el aviso de que el .dat principal sí se creó.
    if (!errorEspejo.isEmpty()) {
        return "AVISO_RESPALDO_FALLIDO";
    }

    return ""; // Éxito absoluto en la persistencia del combo de archivos
}

TipoFormato MoleculaView::getTipoArchivo(const QString& ruta){
    // Devolvemos .out, .arc, .vismol, .dat, .xyz
    //
    // si primera palabra es SUMMARY OF             -> .arc
    // si existe  JOB ENDED NORMALLY y MOPAC DONE   -> .out
    // si existe [ATOMOS] y [ENLACES]               -> .vismol
    //si existe ATOM_X_UPDATED:ANGSTROMS            -> .aux

    if (ruta.isEmpty()) return TipoFormato::NO_SE_SABE;

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return TipoFormato::NO_SE_SABE;
    }

    QTextStream entrada(&archivo);

    bool esMopacArc         = false;
    bool tieneJobEnded      = false;
    bool tieneMopacDone     = false;
    bool tieneAtomXUpdated  = false;
    bool tieneAtomos        = false;
    bool tieneEnlaces       = false;
    bool linea3Vacia        = false;
    bool linea4Vacia        = false;

    int lineasLeidas = 0;

    // Unificamos toda la lectura en un solo bucle seguro
    while (!entrada.atEnd() && lineasLeidas < 2000) {
        QString linea = entrada.readLine().trimmed();
        lineasLeidas++;

        if (linea.contains("SUMMARY OF", Qt::CaseInsensitive))                  esMopacArc          = true;
        if (linea.contains("JOB ENDED NORMALLY", Qt::CaseInsensitive))          tieneJobEnded       = true;
        if (linea.contains("ATOM_X_UPDATED:ANGSTROMS", Qt::CaseInsensitive))    tieneAtomXUpdated   = true;
        if (linea.contains("MOPAC DONE", Qt::CaseInsensitive))                  tieneMopacDone      = true;
        if (linea == "[ATOMOS]")                                                tieneAtomos         = true;
        if (linea == "[ENLACES]")                                               tieneEnlaces        = true;
        if (lineasLeidas == 3 && linea.isEmpty())                               linea3Vacia         = true;
        if (lineasLeidas == 4 && linea.isEmpty())                               linea4Vacia         = true;
    }
    archivo.close();

    // Maquinaria de decisión jerárquica limpia
    if (tieneAtomos && tieneEnlaces)        return TipoFormato::VISMOL;
    if (esMopacArc)                         return TipoFormato::ARC;
    if (tieneJobEnded || tieneMopacDone)    return TipoFormato::OUT;
    if (linea3Vacia && !linea4Vacia)        return TipoFormato::DAT;
    if(tieneAtomXUpdated)                   return TipoFormato::AUX;

    return TipoFormato::NO_SE_SABE;
}

// =========================================================================
// 11. ZONA: MÉTODOS INTERNOS PRIVADOS (VALIDACIONES Y AUXILIARES QUÍMICOS)
// =========================================================================

int MoleculaView::getIdAtomoFromPos(const QVector3D& posClick){
    for(const Atomo& atomo: m_listaAtomos){
        float distancia = posClick.distanceToPoint(atomo.posicion);

        // Recuperamos el mismo radio que usas en el lienzo para que la zona de click sea exacta
        float radioAtomo = 15.0f; // Valor por defecto

        radioAtomo = Atomos::getRadio(atomo.simbolo);

        // Si la distancia del click al centro es menor o igual al radio, está dentro del átomo
        if(distancia <= radioAtomo){
            return atomo.id;
        }
    }
    return -1;
}

bool MoleculaView::isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2){
    // 1. Evitar que un átomo se enlace consigo mismo
    if (atomo1.id == atomo2.id) return false;

    // 2. Calcular cuántos enlaces TOTALES acumulados tiene cada átomo actualmente
    int enlacesActuales1 = getEnlaces(atomo1);
    int enlacesActuales2 = getEnlaces(atomo2);

    // 3. Obtener sus valencias máximas (ej: C=4, N=3, O=2, H=1)
    int max1 = Atomos::getMaxValencia(atomo1.simbolo);
    int max2 = Atomos::getMaxValencia(atomo2.simbolo);

    // Regla A: Si cualquiera de los dos ya está al límite de su octeto/dueto, se bloquea
    if ((enlacesActuales1 + 1) > max1 || (enlacesActuales2 + 1) > max2) {
        return false;
    }

    // =========================================================================
    // ¡NUEVA REGLA QUÍMICA CRÍTICA!: BLOQUEAR ENLACES CUÁDRUPLES DE FORMA PREVENTIVA
    // =========================================================================
    // Buscamos si ya existe un enlace entre estos dos átomos específicos
    for (const Enlace& enlace : m_listaEnlaces) {
        if ((enlace.id_atomo1 == atomo1.id && enlace.id_atomo2 == atomo2.id) ||
            (enlace.id_atomo1 == atomo2.id && enlace.id_atomo2 == atomo1.id)) {

            // Si el enlace actual ya es de orden 3 (Triple), es imposible físicamente
            // que absorba un cuarto orden, sin importar que al Carbono le sobren electrones.
            if (enlace.orden >= 3) {
                return false;
            }
            break;
        }
    }

    // =========================================================================
    // 4. TABLA DE ENFRENTAMIENTO DIRECTO (COMPATIBILIDAD GEOMÉTRICA)
    // =========================================================================
    bool isCompatible = Atomos::isCompatibles(atomo1.simbolo, atomo2.simbolo);
    if(!isCompatible) return false;

    return true;
}

int MoleculaView::getEnlaces(const Atomo& atomo){
    int totalEnlaces = 0;
    for(Enlace &enlace: m_listaEnlaces){
        if(enlace.id_atomo1 == atomo.id || enlace.id_atomo2 == atomo.id){
            totalEnlaces += enlace.orden;
        }
    }
    return totalEnlaces;
}

float MoleculaView::getDistanciaPuntoSegmento(const QVector3D& punto, const QVector3D& pos1, const QVector3D& pos2){
    QVector3D ab = pos2 - pos1;
    QVector3D ap = punto - pos1;

    // Calcular la proyección del punto sobre el vector del segmento
    float longitudAbCuadrada = ab.lengthSquared();
    if (longitudAbCuadrada < 0.0001f) return pos1.distanceToPoint(punto); // Evitar división por cero

    // Factor de proyección (t) acotado entre 0.0 y 1.0 para mantenerse dentro del segmento
    float t = QVector3D::dotProduct(ap, ab) / longitudAbCuadrada;
    t = qMax(0.0f, qMin(1.0f, t));

    // El punto más cercano sobre el segmento de recta
    QVector3D puntoCercano = pos1 + t * ab;

    // Devolvemos la distancia real entre el clic y ese punto cercano
    return punto.distanceToPoint(puntoCercano);
}



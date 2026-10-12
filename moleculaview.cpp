#include "moleculaview.h"
#include "atomos.h"
#include "datosio.h"

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>


// =========================================================================
// ZONA: CICLO DE VIDA DEL CEREBRO DE NEGOCIO
// =========================================================================

MoleculaView::MoleculaView(QObject *parent)
    : QObject(parent){

    // Estado Inicial, al nacer
    m_modoActual                = ModoSeleccion;
    m_modoLienzo.estaVacio      = true;
    m_modoLienzo.estaGuardado   = false;
    m_modoLienzo.estaIniciado   = false;
    m_atomoActivo               = "C";
}

MoleculaView::~MoleculaView(){

}

//#########################################################################
// ZONA: FUNCIONES PÚBLICAS (CONTROL DE ESTADOS Y HERRAMIENTAS)
//#########################################################################

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

void MoleculaView::setNuevoProyecto(){
    limpiarLienzo();

    ModoLienzo modo;

    modo.estaGuardado   = false;
    modo.estaIniciado   = true;
    modo.estaVacio      = true;

    setModoEditor(ModoSeleccion);
    setModoLienzo(modo);
}

QString MoleculaView::loadFileVisMol(const QString& ruta){
    // Control de seguridad inicial para evitar rutas vacías
    if (ruta.isEmpty()) return "Debes especificar unja archivo para guardar los datos.";

    return DatosIO::loadVisMol(ruta, m_listaAtomos, m_listaEnlaces); // Éxito absoluto en la carga
}

QString MoleculaView::saveFileVisMol(const QString& ruta){
    // Control de seguridad inicial para evitar rutas vacías
    if (ruta.isEmpty()) return "Debes especificar unja archivo para guardar los datos.";

    return DatosIO::saveVisMol(ruta, m_listaAtomos, m_listaEnlaces);
}

//#########################################################################
// ZONA: FUNCIONES PÚBLICAS (INTERACCIÓN CON EL ESCENARIO / RATÓN)
//#########################################################################

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

//#########################################################################
// ZONA: FUNCIONES PÚBLICAS (GESTOR DE MEMORIA Y ELIMINACIÓN)
//#########################################################################

void MoleculaView::delAtomo(const int& idAtomo){
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
    // Alteración de memoria en silencio
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    m_contadorIds   = 0;

    m_atomoActivo   = "C";
    m_modoActual    = ModoSeleccion;

    // Reseteo absoluto de la salud del documento
    ModoLienzo modo;

    modo.estaVacio      = true;
    modo.estaGuardado   = false;
    modo.estaIniciado   = true; // Borra todo pero no cierra el proyecto

    setModoLienzo(modo);
}

void MoleculaView::cerrarProyecto(){
    // Alteración de memoria en silencio
    m_listaAtomos.clear();
    m_listaEnlaces.clear();
    m_contadorIds   = 0;

    m_atomoActivo   = "C";
    m_modoActual    = ModoSeleccion;

    // Reseteo absoluto de la salud del documento
    ModoLienzo modo;

    modo.estaVacio      = true;
    modo.estaGuardado   = false;
    modo.estaIniciado   = false; // Cierra totalmente el proyecto

    setModoLienzo(modo);
}

// =========================================================================
// ZONA: GETTERS DE ACCESO A DATOS (SOLO LECTURA)
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

//#########################################################################
// ZONA: INTERNA / PRIVADA (LÓGICA DE NEGOCIO CEREBRAL)
//#########################################################################

void MoleculaView::setElementoSeleccionado(int idAtomoSeleccionado){

}

void MoleculaView::procesarClickDibujoAtomo(const QVector3D& posClick) {
    if(getIdAtomoFromPos(posClick) < 0){
        Atomo nuevoAtomo;

        nuevoAtomo.id       = m_contadorIds++;
        nuevoAtomo.simbolo  = m_atomoActivo;
        nuevoAtomo.posicion = posClick;

        // Introducido directamente aquí: Almacenamos en el vector del negocio
        m_listaAtomos.append(nuevoAtomo);

        // Actualizamos de forma centralizada la salud y estado de la UI
        setModoLienzo(m_modoLienzo);
    }

}

void MoleculaView::procesarClickSeleccion(const QVector3D& posClick) {
}

void MoleculaView::procesarClickCrearEnlace(const QVector3D& posClick) {
    int idAtomoActual = getIdAtomoFromPos(posClick);
    if(idAtomoActual < 0) return;

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
// ZONA: GESTOR CENTRALIZADO DE LA SALUD DEL DOCUMENTO
// =========================================================================

void MoleculaView::setModoLienzo(const ModoLienzo& modoLienzo){
    // 1. Calculamos la realidad objetiva de los almacenes de memoria
    bool vacioAhora = m_listaAtomos.isEmpty() && m_listaEnlaces.isEmpty();

    // 2. Si el lienzo se vacía por completo, la persistencia se apaga de forma obligatoria
    bool guardadoReal = vacioAhora ? false : modoLienzo.estaGuardado;

    // 3. Asignamos los tres estados en un único rincón controlado
    m_modoLienzo.estaIniciado = modoLienzo.estaIniciado;
    m_modoLienzo.estaGuardado = guardadoReal;
    m_modoLienzo.estaVacio    = vacioAhora;

    // 4. EMISIÓN ATÓMICA CENTRALIZADA HACIA LA INTERFAZ
    emit modoEditorCambiado(m_modoActual);
    emit modoLienzoCambiado(m_modoLienzo);
    emit actualizarLienzo();

    setMensajeEstado();
}

MoleculaView::ModoLienzo MoleculaView::getModoLienzo(){
    return m_modoLienzo;
}

void MoleculaView::setMensajeEstado(){
    // Si el proyecto NO ha sido iniciado o está en letargo, mandamos el texto de bloqueo
    if (!m_modoLienzo.estaIniciado) {
        emit notificarMensajeEstado(tr(" Proyecto Vacío. Cree un nuevo lienzo o abra un archivo para comenzar."));
        return;
    }

    // Evaluamos la herramienta activa actual en el negocio
    switch(m_modoActual) {
    case MoleculaView::ModoSeleccion:
        emit notificarMensajeEstado(tr(" Herramienta activa: Modo Seleccion (Puntero Neutro)."));
        break;

    case MoleculaView::ModoDibujo:
        emit notificarMensajeEstado(tr(" Herramienta activa: Dibujando Átomo de %1.").arg(m_atomoActivo));
        break;

    case MoleculaView::ModoCrearEnlace:
        emit notificarMensajeEstado(tr(" Herramienta activa: Modo Enlace Covalente habilitado."));
        break;

    case MoleculaView::ModoRotacion3D:
        emit notificarMensajeEstado(tr(" Herramienta activa: Modo Rotación y Vista 3D."));
        break;
    }
}


// =========================================================================
// ZONA: VALIDACIONES Y AUXILIARES QUÍMICOS / MATEMÁTICOS
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
    QString s1 = atomo1.simbolo;
    QString s2 = atomo2.simbolo;

    // --- BLOQUE HIDRÓGENO (H) ---
    if (s1 == "H" || s2 == "H") {
        QString compañero = (s1 == "H") ? s2 : s1;
        // El hidrógeno orgánico elemental solo se une a C, H, O o N
        if (compañero != "C" && compañero != "H" && compañero != "O" && compañero != "N") {
            return false;
        }
    }

    // --- BLOQUE CARBONO (C) ---
    if (s1 == "C" || s2 == "C") {
        return true;
    }

    // --- BLOQUE OXÍGENO (O) ---
    if (s1 == "O" && s2 == "O") {
        return true;
    }

    // --- BLOQUE NITRÓGENO (N) ---
    if ((s1 == "N" && s2 == "O") || (s1 == "O" && s2 == "N")) {
        return true;
    }
    if (s1 == "N" && s2 == "N") {
        return true;
    }

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
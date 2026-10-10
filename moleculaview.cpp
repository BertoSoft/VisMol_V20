#include "moleculaview.h"

#include <QObject>

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

//##############################################################
// Funciones Públicas
//################################################################

void MoleculaView::setModoEditor(const ModoEditor& nuevoModo){

    if(getModoEditor() == nuevoModo) return;

    m_modoActual = nuevoModo;
}

void MoleculaView::setElementoActivo(const QString& simbolo){

    if(getSimboloAtomoActivo() == simbolo) return;

    m_atomoActivo = simbolo;
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
    modo.estaIniciado   = false; // Regresa al letargo de espera

    setModoLienzo(modo);
}

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

//################################################################
// Funciones Privadas de MoleculaView
//################################################################

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

MoleculaView::ModoLienzo MoleculaView::getModoLienzo(){
    return m_modoLienzo;
}

void MoleculaView::procesarClickSeleccion(const QVector3D& posClick) {
}

void MoleculaView::procesarClickCrearEnlace(const QVector3D& posClick) {
    // si el click no esta encima de un atomo, volvemos
    int idAtomoActual = getIdAtomoFromPos(posClick);
    if(idAtomoActual < 0) return;

    if(m_idAtomoSeleccionado < 0){
        m_idAtomoSeleccionado = idAtomoActual;
        return;
    }

    // Si llegamos aqui, estamos ante el atomo2 seleccionado
    int idAtomo1 = m_idAtomoSeleccionado;
    int idAtomo2 = idAtomoActual;

    m_idAtomoSeleccionado = -1;


    // 3. Recuperamos las estructuras Atomo completas del vector de memoria
    Atomo atomo1, atomo2;
    bool encontrado1 = false, encontrado2 = false;
    for(const Atomo& a : m_listaAtomos) {
        if(a.id == idAtomo1) { atomo1 = a; encontrado1 = true; }
        if(a.id == idAtomo2) { atomo2 = a; encontrado2 = true; }
        if(encontrado1 && encontrado2) break;
    }

    // =========================================================================
    // CASO A: EL ENLACE YA EXISTE -> EVALUAMOS INCREMENTAR SU ORDEN (1 -> 2 -> 3)
    // =========================================================================
    for(Enlace &enlace : m_listaEnlaces){
        if((enlace.id_atomo1 == idAtomo1 && enlace.id_atomo2 == idAtomo2) ||
            (enlace.id_atomo1 == idAtomo2 && enlace.id_atomo2 == idAtomo1)) {

            // Si ya es triple (orden 3), hemos llegado al límite químico permitido
            if(enlace.orden >= 3) return;

            // Verificamos si los átomos admiten absorber un orden más en sus valencias
            if(isEnlacePosible(atomo1, atomo2)){
                enlace.orden++;              // Incrementamos el orden (Pasa a doble o triple)
                setModoLienzo(m_modoLienzo); // Notificamos a la vista para repintar
            }
            return; // ¡IMPORTANTE! Salimos de la función aquí para evitar que se ejecute el código de abajo
        }
    }

    // =========================================================================
    // CASO B: EL ENLACE NO EXISTE -> CREAMOS UN ENLACE NUEVO DESDE CERO (ORDEN 1)
    // =========================================================================
    if(isEnlacePosible(atomo1, atomo2)){
        Enlace nuevoEnlace;
        nuevoEnlace.id_atomo1 = idAtomo1;
        nuevoEnlace.id_atomo2 = idAtomo2;
        nuevoEnlace.orden     = 1; // Todo enlace nuevo nace siendo simple

        m_listaEnlaces.append(nuevoEnlace);
        setModoLienzo(m_modoLienzo); // Notificamos e invocamos actualizarLienzo()
    }
}

void MoleculaView::procesarClickRotacion(const QVector3D& posClick) {
}

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

    // Le decimos al lienzo: "Las listas de memoria han cambiado, vuelve a pintar"
    emit actualizarLienzo();
}

int MoleculaView::getIdAtomoFromPos(const QVector3D& posClick){
    for(const Atomo& atomo: m_listaAtomos){
        float distancia = posClick.distanceToPoint(atomo.posicion);

        // Recuperamos el mismo radio que usas en el lienzo para que la zona de click sea exacta
        float radioAtomo = 15.0f; // Valor por defecto
        if (atomo.simbolo == "C") radioAtomo = 36.0f;
        else if (atomo.simbolo == "H") radioAtomo = 24.0f;
        else if (atomo.simbolo == "O") radioAtomo = 32.0f;
        else if (atomo.simbolo == "N") radioAtomo = 34.0f;

        // Si la distancia del click al centro es menor o igual al radio, está dentro del átomo
        if(distancia <= radioAtomo){
            return atomo.id;
        }
    }
    return -1;
}

int MoleculaView::getValenciaMax(const Atomo& atomo){
    if (atomo.simbolo == "H") return 1;  // Hidrógeno dueto
    if (atomo.simbolo == "O") return 2;  // Oxígeno
    if (atomo.simbolo == "N") return 3;  // Nitrógeno
    if (atomo.simbolo == "C") return 4;  // Carbono
    if (atomo.simbolo == "S") return 6;  // Azufre (hipervalente común)
    if (atomo.simbolo == "P") return 5;  // Fósforo (hipervalente común)
    if (atomo.simbolo == "F" || atomo.simbolo == "Cl") return 1; // Halógenos
    return 0; // Desconocido / Bloqueado
}

bool MoleculaView::isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2){
    // 1. Evitar que un átomo se enlace consigo mismo (seguridad redundante)
    if (atomo1.id == atomo2.id) return false;

    // 2. Calcular cuántos enlaces TOTALES acumulados tiene cada átomo actualmente
    int enlacesActuales1 = getEnlaces(atomo1);
    int enlacesActuales2 = getEnlaces(atomo2);

    // 3. Obtener sus valencias máximas desde la tabla periódica configurada
    int max1 = getValenciaMax(atomo1);
    int max2 = getValenciaMax(atomo2);

    // Si cualquiera de los dos supera su octeto/dueto al añadir un orden más, se bloquea
    if ((enlacesActuales1 + 1) > max1 || (enlacesActuales2 + 1) > max2) {
        return false;
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
        // Habiendo comprobado el máximo de 4 enlaces arriba, se puede unir libremente a C, H, O, N
        return true;
    }

    // --- BLOQUE OXÍGENO (O) ---
    if (s1 == "O" && s2 == "O") {
        return true; // Enlaces peróxido u Ozono permitidos
    }

    // --- BLOQUE NITRÓGENO (N) ---
    if ((s1 == "N" && s2 == "O") || (s1 == "O" && s2 == "N")) {
        return true; // Grupos funcionales Nitro u óxidos directos
    }
    if (s1 == "N" && s2 == "N") {
        return true; // Enlaces azo-compuestos o di-nitrógeno
    }

    return true; // Flexibilidad para el resto de elementos añadidos
}

int MoleculaView::getElectronesValenciaNaturales(const Atomo& atomo){
    if (atomo.simbolo == "H") return 1;
    if (atomo.simbolo == "C") return 4;
    if (atomo.simbolo == "N" || atomo.simbolo == "P") return 5;
    if (atomo.simbolo == "O" || atomo.simbolo == "S") return 6;
    if (atomo.simbolo == "F" || atomo.simbolo == "Cl") return 7;
    return 0;
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
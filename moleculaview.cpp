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

    // Regla 1: No se puede enlazar un átomo consigo mismo
    if(idAtomo1 == idAtomo2) return;

    // Recuperamos las estructuras Atomo completas de la lista
    Atomo atomo1, atomo2;
    for(const Atomo& a : m_listaAtomos) {
        if(a.id == idAtomo1) atomo1 = a;
        if(a.id == idAtomo2) atomo2 = a;
    }

    // REGLAS 2 Y 3: Si ya existe el enlace, recuperar orden y evaluar incremento

    for(Enlace &enlace: m_listaEnlaces){
        if((enlace.id_atomo1 == idAtomo1 && enlace.id_atomo2 == idAtomo2) ||
            (enlace.id_atomo1 == idAtomo2 && enlace.id_atomo2 == idAtomo1)) {

            if(enlace.orden >= 3) return;

            if(isEnlacePosible(atomo1, atomo2)){
                enlace.orden++;
                setModoLienzo(m_modoLienzo); // Repinta el lienzo
            }
            return;
        }
    }

    // REGLA 4: Si no existe enlace, comprobar viabilidad y crearlo

    if(isEnlacePosible(atomo1, atomo2)){
        Enlace nuevoEnlace;
        nuevoEnlace.id_atomo1 = idAtomo1;
        nuevoEnlace.id_atomo2 = idAtomo2;
        nuevoEnlace.orden     = 1; // Nace como enlace simple

        m_listaEnlaces.append(nuevoEnlace);
        setModoLienzo(m_modoLienzo); // Notifica e invoca actualizarLienzo()
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
    // 2. Calcular cuántos enlaces tendrían si aceptamos esta nueva unión/incremento
    // Sumamos +1 al conteo actual de ambos
    int futurosEnlaces1 = getEnlaces(atomo1) + 1;
    int futurosEnlaces2 = getEnlaces(atomo2) + 1;

    // 3. Obtener sus valencias ideales de la tabla periódica
    int ideal1 = getValenciaMax(atomo1); // Tu función anterior (C=4, O=2, H=1...)
    int ideal2 = getValenciaMax(atomo2);

    // 4. Calcular las cargas formales resultantes (Desplazamiento de valencia)
    int cargaFutura1 = ideal1 - futurosEnlaces1;
    int cargaFutura2 = ideal2 - futurosEnlaces2;

    // =========================================================================
    // ZONA DE EXCEPCIONES EXPLÍCITAS (Casos moleculares puros tolerados)
    // =========================================================================

    // Excepción 1: Hidrógeno molecular (H-H).
    // Aunque el H tiene valencia 1, el enlace H-H es perfectamente válido y su carga formal es 0.
    if (atomo1.simbolo == "H" && atomo2.simbolo == "H" && futurosEnlaces1 == 1) {
        return true;
    }

    // Excepción 2: Estructuras resonantes de Ozono (O3) o similares (Enlaces coordinados/Dativos)
    // En el ozono, un oxígeno central tiene 3 enlaces (carga +1) y uno terminal tiene 1 enlace (carga -1).
    if (atomo1.simbolo == "O" && atomo2.simbolo == "O") {
        // Permitimos de forma controlada que el oxígeno llegue a tener 3 enlaces
        // SI Y SOLO SI está unido a otro oxígeno que equilibre el sistema.
        if (futurosEnlaces1 <= 3 && futurosEnlaces2 <= 3) {
            return true;
        }
    }

    // =========================================================================
    // REGLAS DE EXCLUSIÓN ABSOLUTA (Enlaces imposibles)
    // =========================================================================

    // Regla A: El Hidrógeno JAMÁS puede tener más de 1 enlace (No soporta carga covalente positiva)
    if ((atomo1.simbolo == "H" && futurosEnlaces1 > 1) ||
        (atomo2.simbolo == "H" && futurosEnlaces2 > 1)) {
        return false;
    }

    // Regla B: El Carbono es muy estricto. No expande octeto (máx 4 enlaces)
    // y tener un carbono con menos de 3 enlaces en una estructura estable sin terminar es inaceptable.
    if ((atomo1.simbolo == "C" && futurosEnlaces1 > 4) ||
        (atomo2.simbolo == "C" && futurosEnlaces2 > 4)) {
        return false;
    }

    // Regla C: Filtro general de inestabilidad por carga (Límite del Octeto rígido para periodo 2)
    // Si la carga resultante se dispara más allá de un estado catiónico/aniónico razonable (+1 o -1)
    if (qAbs(cargaFutura1) > 1 || qAbs(cargaFutura2) > 1) {
        return false;
    }
    return true; // Si pasa todos los filtros, el enlace es químicamente viable
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
            totalEnlaces = enlace.orden;
        }
    }
    return totalEnlaces;
}
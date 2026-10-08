#include "moleculaview.h"

#include <QObject>

MoleculaView::MoleculaView(QObject *parent)
    : QObject(parent){


}

MoleculaView::~MoleculaView(){

}

//##############################################################
// Funciones Públicas
//################################################################

void MoleculaView::setModoEditor(ModoEditor nuevoModo){

    if(getModoEditor() == nuevoModo) return;

    m_modoActual = nuevoModo;

    emit modoEditorCambiado(nuevoModo);
}

void MoleculaView::setElementoActivo(int idNuevoAtomo){

    if(getIdAtomoActivo() == idNuevoAtomo) return;

    m_idAtomoActivo = idNuevoAtomo;

    emit atomoActivoCambiado(idNuevoAtomo);
}

void MoleculaView::setClick(QVector3D posClick){
    switch(m_modoActual) {
    // --- GRUPO 1: MODOS DE DIBUJO DE ÁTOMOS ---
    case ModoDibujoCarbono:
    case ModoDibujoHidrogeno:
    case ModoDibujoNitrogeno:
    case ModoDibujoOxigeno:
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

    // Limpieza absoluta de los almacenes de memoria
    m_listaAtomos.clear();
    m_listaEnlaces.clear();

    // Reseteamos los estados a sus valores de fábrica
    m_contadorIds   = 0;
    m_idAtomoActivo = -1;
    m_modoActual    = ModoSeleccion;

    // Emitimos las señales obligatorias para sincronizar la interfaz gráfica
    emit modoEditorCambiado(m_modoActual);
    emit atomoActivoCambiado(m_idAtomoActivo);
}

MoleculaView::ModoEditor MoleculaView::getModoEditor(){

    return m_modoActual;
}

int MoleculaView::getIdAtomoActivo(){

    return m_idAtomoActivo;
}

//################################################################
// Funciones Privadas de MoleculaView
//################################################################

void MoleculaView::addNuevoAtomo(const Atomo& nuevoAtomo){
    m_listaAtomos.append(nuevoAtomo);

    emit atomoAdd(nuevoAtomo);
}

void MoleculaView::addNuevoEnlace(const Enlace& nuevoEnlace){
    m_listaEnlaces.append(nuevoEnlace);
    emit enlaceAdd(nuevoEnlace);
}

void MoleculaView::procesarClickDibujoAtomo(const QVector3D& posClick) {
    QString simbolo = "";

    // Volvemos a evaluar el modo solo para extraer el símbolo químico exacto
    switch(m_modoActual) {
    case ModoDibujoCarbono:   simbolo = "C"; break;
    case ModoDibujoHidrogeno: simbolo = "H"; break;
    case ModoDibujoNitrogeno: simbolo = "N"; break;
    case ModoDibujoOxigeno:   simbolo = "O"; break;
    default: return;
    }

    Atomo nuevoAtomo;
    nuevoAtomo.id       = m_contadorIds++; // Mantenemos tu regla: asigna e incrementa aquí
    nuevoAtomo.simbolo  = simbolo;
    nuevoAtomo.posicion = posClick;

    addNuevoAtomo(nuevoAtomo);
}

void MoleculaView::procesarClickSeleccion(const QVector3D& posClick) {
}

void MoleculaView::procesarClickCrearEnlace(const QVector3D& posClick) {
}

void MoleculaView::procesarClickRotacion(const QVector3D& posClick) {
}
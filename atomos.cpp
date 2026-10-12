#include "atomos.h"

//Iniciazion de la variable estatica m_listaAtomos
QList<Atomos::Atomo> Atomos::m_listaAtomos;

// Funciones Publicas
qreal Atomos::getRadio(const QString& simbolo){
    return getAtomo(simbolo).radio;
}

int Atomos::getMaxValencia(const QString& simbolo){
    return getAtomo(simbolo).maxValencia;
}

QColor Atomos::getColor(const QString& simbolo){
    return getAtomo(simbolo).color;
}

bool Atomos::isTextoOscuro(const QString& simbolo){
    return getAtomo(simbolo).textoOscuro;
}


//Funciones Privadas

void Atomos::initAtomos(){
    if(!m_listaAtomos.isEmpty()) return;

    m_listaAtomos = {
        { 1, "Hidrógeno", "H", QColor("#F3F4F6"), 24.0, 1, true  },
        { 2, "Carbono",   "C", QColor("#4B5563"), 36.0, 4, false },
        { 3, "Oxígeno",   "O", QColor("#EF4444"), 32.0, 2, false },
        { 4, "Nitrógeno", "N", QColor("#3B82F6"), 34.0, 3, false },
        { 5, "", "", QColor("#000000"), 0.0, 0, false }
    };
}

Atomos::Atomo Atomos::getAtomo(const QString& simbolo){
    initAtomos();

    for(const Atomo &atomo: m_listaAtomos){
        if(atomo.simbolo == simbolo){
            return atomo;
        }
    }
    return { -1, "Desconocido", "?", QColor("#EC4899"), 15.0, 0, false };
}
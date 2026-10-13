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

bool Atomos::isCompatibles(const QString& simbolo1, const QString& simbolo2){
    QString s1 = simbolo1;
    QString s2 = simbolo2;

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


//Funciones Privadas

void Atomos::initAtomos(){
    if(!m_listaAtomos.isEmpty()) return;

    m_listaAtomos = {
        { 1, "Hidrógeno", "H", QColor("#F3F4F6"), 24.0, 1, true  },
        { 2, "Carbono",   "C", QColor("#4B5563"), 36.0, 4, false },
        { 3, "Oxígeno",   "O", QColor("#EF4444"), 32.0, 2, false },
        { 4, "Nitrógeno", "N", QColor("#3B82F6"), 34.0, 3, false },
    };
}

const Atomos::Atomo& Atomos::getAtomo(const QString& simbolo){
    initAtomos();

    for(const Atomo &atomo: m_listaAtomos){
        if(atomo.simbolo == simbolo){
            return atomo;
        }
    }

    static const Atomo atomoDesconocido = {
        -1, "Desconocido", "?", QColor("#EC4899"), 15.0, 0, false
    };

    return atomoDesconocido;
}
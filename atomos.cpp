#include "atomos.h"

// Inicialización obligatoria de la estructura física de almacenamiento estático
QList<Atomos::Atomo> Atomos::m_listaAtomos;

// =========================================================================
// ZONA: INTERFAZ DE CONSULTA PÚBLICA (ENCAPSULAMIENTO DE PROPIEDADES)
// =========================================================================

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

/**
 * @brief Evalúa la viabilidad química de un enlace covalente orgánico entre dos elementos.
 * @note [CORREGIDO]: Se elimina la copia de variables y se blinda la salida contra falsos positivos.
 * @return true si el enlace es compatible con la química orgánica elemental estándar, false si se bloquea.
 */
bool Atomos::isCompatibles(const QString& simbolo1, const QString& simbolo2){
    if (simbolo1 == simbolo2) {
        return (simbolo1 == "C" || simbolo1 == "O" || simbolo1 == "N" || simbolo1 == "S");
    }

    if (simbolo1 == "H" || simbolo2 == "H") {
        QString compañero = (simbolo1 == "H") ? simbolo2 : simbolo1;
        return (compañero == "C" || compañero == "O" || compañero == "N" || compañero == "S");
    }

    if (simbolo1 == "C" || simbolo2 == "C") {
        return true;
    }

    // Permitir enlaces Nitrógeno-Oxígeno y Oxígeno-Azufre / Nitrógeno-Azufre comunes
    if ((simbolo1 == "N" && simbolo2 == "O") || (simbolo1 == "O" && simbolo2 == "N")) return true;
    if ((simbolo1 == "S" && simbolo2 == "O") || (simbolo1 == "O" && simbolo2 == "S")) return true;
    if ((simbolo1 == "S" && simbolo2 == "N") || (simbolo1 == "N" && simbolo2 == "S")) return true;

    return false;
}

// =========================================================================
// ZONA: PROCESAMIENTO INTERNO Y MÉTODOS PRIVADOS
// =========================================================================

/**
 * @brief Inicializa y vuelca en memoria los elementos químicos estables permitidos de fábrica.
 * @note Implementa un filtro de guarda para asegurar una única ejecución transaccional en la sesión.
 */
void Atomos::initAtomos(){
    if(!m_listaAtomos.isEmpty()) return;

    m_listaAtomos = {
        { 1, "Hidrógeno", "H", QColor("#F3F4F6"), 24.0, 1, true  }, // Configuración CPK clara
        { 2, "Carbono",   "C", QColor("#4B5563"), 36.0, 4, false }, // Configuración CPK gris oscuro
        { 3, "Oxígeno",   "O", QColor("#EF4444"), 32.0, 2, false }, // Configuración CPK rojo
        { 4, "Nitrógeno", "N", QColor("#3B82F6"), 34.0, 3, false }, // Configuración CPK azul eléctrico
        { 5, "Azufre", "S", QColor("#FBBF24"), 50.0, 6, false },    // Configuración CPK amarillo
    };
}

/**
 * @brief Recupera la ficha técnica estructurada de un átomo buscando por su símbolo inmutable.
 * @param simbolo Nombre corto del elemento a localizar (ej: "C").
 * @return Referencia constante segura al registro del Atomo en memoria.
 */
const Atomos::Atomo& Atomos::getAtomo(const QString& simbolo){
    // Aseguramos que la base de datos local esté levantada en la RAM
    initAtomos();

    // Búsqueda secuencial limpia sobre la lista oficial de elementos
    for(const Atomo &atomo: m_listaAtomos){
        if(atomo.simbolo == simbolo){
            return atomo; // Retorno inmediato al localizar la coincidencia exacta
        }
    }

    // FALLBACK DEFENSIVO ANTIPÁNICO: Si el parser lee del archivo un elemento no controlado (ej: "S", "Cl")
    // devolvemos una ficha genérica de color magenta para alertar visualmente al usuario sin tumbar la CPU.
    static const Atomo atomoDesconocido = {
        -1, "Desconocido", "?", QColor("#EC4899"), 15.0, 0, false
    };

    return atomoDesconocido;
}
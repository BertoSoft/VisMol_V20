#ifndef ATOMOS_H
#define ATOMOS_H

#include <QString>
#include <QColor>
#include <QList>

/**
 * @class Atomos
 * @brief Clase estática que actúa como base de datos y validador de propiedades químicas de la aplicación.
 *
 * Gestiona de forma centralizada los radios moleculares, colores, valencias máximas
 * y reglas de hibridación/compatibilidad orgánica para el lienzo gráfico.
 */
class Atomos {

public:
    // Forzamos el comportamiento estático puro eliminando constructores
    Atomos() = delete;
    ~Atomos() = delete;

    // Funciones Públicas de Consulta de Atributos
    static qreal   getRadio(const QString& simbolo);
    static int     getMaxValencia(const QString& simbolo);
    static QColor  getColor(const QString& simbolo);
    static bool    isTextoOscuro(const QString& simbolo);
    static bool    isCompatibles(const QString& simbolo1, const QString& simbolo2);

private:
    /**
     * @struct Atomo
     * @brief Define las propiedades físicas y de renderizado de un elemento químico en la aplicación.
     */
    struct Atomo {
        int         id;           // ID único secuencial del elemento
        QString     nombre;       // Nombre completo del elemento químico
        QString     simbolo;      // Símbolo químico inmutable (ej: "C", "H")
        QColor      color;        // Color asignado según el estándar CPK
        qreal       radio;        // Radio geométrico para el área de clic en el lienzo
        int         maxValencia;  // Valencia máxima permitida (octeto/dueto)
        bool        textoOscuro;  // Flag para aplicar contraste al renderizar texto interno
    };

    // Inicializadores y gestores internos
    static void            initAtomos();
    static const Atomo&    getAtomo(const QString& simbolo);

    // Contenedor principal de la tabla periódica local de la aplicación
    static QList<Atomo>    m_listaAtomos;
};

#endif // ATOMOS_H
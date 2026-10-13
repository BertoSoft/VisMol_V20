#ifndef ATOMOS_H
#define ATOMOS_H

#include <QString>
#include <QColor>

class Atomos{

public:
    Atomos() = delete;
    ~Atomos() = delete;

    // Funciones Publicas
    static qreal   getRadio(const QString& simbolo);
    static int     getMaxValencia(const QString& simbolo);
    static QColor  getColor(const QString& simbolo);
    static bool    isTextoOscuro(const QString& simbolo);
    static bool    isCompatibles(const QString& simbolo1, const QString& simbolo2);

private:

    struct Atomo{
        int         id;
        QString     nombre;
        QString     simbolo;
        QColor      color;
        qreal       radio;
        int         maxValencia;
        bool        textoOscuro;
    };

    static void            initAtomos();
    static const Atomo&    getAtomo(const QString& simbolo);

    static QList<Atomo>   m_listaAtomos;
};

#endif // ATOMOS_H

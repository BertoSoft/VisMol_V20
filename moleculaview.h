#ifndef MOLECULAVIEW_H
#define MOLECULAVIEW_H

#include <QObject>
#include <QVector3D>
#include <QString>

// =========================================================================
// ESTRUCTURAS DE DATOS ORIGINALES
// =========================================================================

// Representa la información química y la ubicación en pantalla de un átomo
struct Atomo {
    int         id;        // Identificador único y numérico para cada átomo (0, 1, 2...)
    QString     simbolo;    // Símbolo químico del elemento (Ej: "C", "H", "O", "N")
    QVector3D   posicion;  // Coordenadas bidimensionales (X, Y) del átomo en el lienzo de dibujo
};

// Representa la unión covalente o enlace químico entre dos átomos
struct Enlace {
    int id_atomo1;      // ID del átomo donde inicia el enlace
    int id_atomo2;      // ID del átomo donde termina el enlace
    int orden;          // Enlace sencillo = 1, doble = 2, y triple = 3
};

class MoleculaView : public QObject
{
    Q_OBJECT

public:

    // 1.- Catalogo Limpio de los estado en los que puede estar el editor
    enum ModoEditor{
        ModoSeleccion,         // Puntero neutro (Mide distancias, activa clic derecho)
        ModoDibujoCarbono,     // Añadir átomos de C
        ModoDibujoHidrogeno,   // Añadir átomos de H
        ModoDibujoOxigeno,     // Añadir átomos de O
        ModoDibujoNitrogeno,   // Añadir átomos de N
        ModoCrearEnlace,       // Unir dos átomos
        ModoRotacion3D         // Rotar la cámara del lienzo
    };

    explicit MoleculaView(QObject *parent = nullptr);
    ~MoleculaView() override;

    // 2. Métodos de control que invocará la Vista al interactuar
    void setModoEditor(ModoEditor nuevoModo);
    void setElementoActivo(int idNuevoAtomo);
    void setClick(QVector3D posClick);
    void limpiarLienzo();

    // 3. Consultores de estado
    ModoEditor  getModoEditor();
    int         getIdAtomoActivo();

signals:

    // 4. Señales para ordenar a la Vista que se actualice
    void modoEditorCambiado(ModoEditor nuevoModo);
    void atomoActivoCambiado(int idNuevoAtomo);
    void atomoAdd(Atomo nuevoAtomo);
    void enlaceAdd(Enlace nuevoEnlace);

private:

    // 5. Estado interno privado (El cerebro)
    ModoEditor          m_modoActual        = ModoEditor::ModoSeleccion;
    int                 m_contadorIds       = 0;
    int                 m_idAtomoActivo     = -1;
    QVector<Atomo>      m_listaAtomos;
    QVector<Enlace>     m_listaEnlaces;

    // 6.- Funciones Privadas solo invocadas por el View
    void addNuevoAtomo(const Atomo& nuevoAtomo);
    void addNuevoEnlace(const Enlace& nuevoEnlace);
    void procesarClickDibujoAtomo(const QVector3D& posClick);
    void procesarClickSeleccion(const QVector3D& posClick);
    void procesarClickCrearEnlace(const QVector3D& posClick);
    void procesarClickRotacion(const QVector3D& posClick);

};

#endif // MOLECULAVIEW_H

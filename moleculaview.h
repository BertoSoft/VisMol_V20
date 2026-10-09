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
        ModoSeleccion,      // Puntero neutro (Mide distancias, activa clic derecho)
        ModoDibujo,         // Modo de adiccion de atomos
        ModoCrearEnlace,    // Unir dos átomos
        ModoRotacion3D      // Rotar la cámara del lienzo
    };

    struct ModoLienzo{
        bool estaVacio      = true;
        bool estaGuardado   = false;
        bool estaIniciado   = false;
    };

    explicit MoleculaView(QObject *parent = nullptr);
    ~MoleculaView() override;

    // 2. Métodos de control que invocará la Vista al interactuar
    void setModoEditor(const ModoEditor& nuevoModo);
    void setElementoActivo(const QString& simbolo);
    void setNuevoProyecto();
    void setClick(const QVector3D& posClick);


    // 3. Consultores de estado
    ModoEditor          getModoEditor();
    ModoLienzo          getModoLienzo();
    QString             getSimboloAtomoActivo();
    QVector<Atomo>      getListaAtomos();
    QVector<Enlace>     getListaEnlaces();

signals:

    // 4. Señales para ordenar a la Vista que se actualice
    void modoEditorCambiado(MoleculaView::ModoEditor nuevoModo);
    void modoLienzoCambiado(MoleculaView::ModoLienzo nuevoModo);
    void actualizarLienzo();

private:

    // 5. Estado interno privado (El cerebro)
    ModoEditor          m_modoActual            = ModoEditor::ModoSeleccion;
    ModoLienzo          m_modoLienzo;
    int                 m_contadorIds           = 0;
    QString             m_atomoActivo           = "";
    int                 m_idAtomoSeleccionado   = -1;
    QVector<Atomo>      m_listaAtomos;
    QVector<Enlace>     m_listaEnlaces;

    // 6.- Funciones Privadas solo invocadas por el View
    void    procesarClickDibujoAtomo(const QVector3D& posClick);
    void    procesarClickSeleccion(const QVector3D& posClick);
    void    procesarClickCrearEnlace(const QVector3D& posClick);
    void    procesarClickRotacion(const QVector3D& posClick);
    void    setElementoSeleccionado(int idAtomoSelecccionado);
    void    setModoLienzo(const ModoLienzo& modoLienzo);
    void    limpiarLienzo();
    int     getIdAtomoFromPos(const QVector3D& pos3D);

};

#endif // MOLECULAVIEW_H

#ifndef MOLECULAVIEW_H
#define MOLECULAVIEW_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>

// Structures necesarias para el negocio molecular
struct Atomo {
    int id;
    QString simbolo;
    QVector3D posicion;
};

struct Enlace {
    int id_atomo1;
    int id_atomo2;
    int orden; // 1 = Simple, 2 = Doble, 3 = Triple
};

class MoleculaView : public QObject
{
    Q_OBJECT

public:
    // Enumeraciones del sistema
    enum ModoEditor {
        ModoSeleccion,
        ModoDibujo,
        ModoCrearEnlace,
        ModoRotacion3D
    };

    struct ModoLienzo {
        bool estaVacio      = true;
        bool estaGuardado   = false;
        bool estaIniciado   = false;
    };

    // =========================================================================
    // ZONA: CICLO DE VIDA DEL CEREBRO DE NEGOCIO
    // =========================================================================
public:
    explicit MoleculaView(QObject *parent = nullptr);
    ~MoleculaView() override;

    //#########################################################################
    // ZONA: FUNCIONES PÚBLICAS (CONTROL DE ESTADOS Y HERRAMIENTAS)
    //#########################################################################
public:
    void        setModoEditor(const ModoEditor& nuevoModo);
    void        setElementoActivo(const QString& simbolo);
    void        setComentario(const QString& comentario);
    void        setNuevoProyecto();
    QString     loadFileVisMol(const QString& ruta);
    QString     saveFileVisMol(const QString& ruta);
    void        deshacer();
    void        rehacer();

    //#########################################################################
    // ZONA: FUNCIONES PÚBLICAS (INTERACCIÓN CON EL ESCENARIO / RATÓN)
    //#########################################################################
public:
    void setClick(const QVector3D& posClick);
    void setClickDerecho(const QVector3D& posClickDerecho);

    //#########################################################################
    // ZONA: FUNCIONES PÚBLICAS (GESTOR DE MEMORIA Y ELIMINACIÓN)
    //#########################################################################
public:
    void delAtomo(const int& idAtomo);
    void delEnlace(const int& idEnlace);
    void limpiarLienzo();
    void cerrarProyecto();

    // =========================================================================
    // ZONA: GETTERS DE ACCESO A DATOS (SOLO LECTURA)
    // =========================================================================
public:
    ModoEditor      getModoEditor();
    QString         getSimboloAtomoActivo();
    QVector<Atomo>  getListaAtomos();
    QVector<Enlace> getListaEnlaces();
    int             getIdAtomoSeleccionado();
    ModoLienzo      getModoLienzo();
    QString         getComentario();
    bool            hayPasado() const;
    bool            hayFuturo() const;


    //#########################################################################
    // ZONA: INTERNA / PRIVADA (LÓGICA DE NEGOCIO CEREBRAL)
    //#########################################################################
private:
    void setElementoSeleccionado(int idAtomoSeleccionado);
    void procesarClickDibujoAtomo(const QVector3D& posClick);
    void procesarClickSeleccion(const QVector3D& posClick);
    void procesarClickCrearEnlace(const QVector3D& posClick);
    void procesarClickRotacion(const QVector3D& posClick);
    void resetVariablesClase();
    void pushAtras();
    void popAtras();
    void pushAdelante();
    void popAdelante();

    // =========================================================================
    // ZONA: GESTOR CENTRALIZADO DE LA SALUD DEL DOCUMENTO
    // =========================================================================
private:
    void setModoLienzo(const ModoLienzo& modoLienzo);
    void setMensajeEstado();

    // =========================================================================
    // ZONA: VALIDACIONES Y AUXILIARES QUÍMICOS / MATEMÁTICOS
    // =========================================================================
private:
    int     getIdAtomoFromPos(const QVector3D& posClick);
    bool    isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2);
    int     getEnlaces(const Atomo& atomo);
    float   getDistanciaPuntoSegmento(const QVector3D& punto, const QVector3D& pos1, const QVector3D& pos2);

    // =========================================================================
    // ZONA: SEÑALES DE NOTIFICACIÓN (MOBILES DE SINCRONIZACIÓN)
    // =========================================================================
signals:
    void actualizarLienzo();
    void modoEditorCambiado(MoleculaView::ModoEditor modo);
    void modoLienzoCambiado(MoleculaView::ModoLienzo modo);
    void notificarMensajeEstado(const QString& mensaje);
    void setMenuEliminar(const int& idAtomo, const int& idEnlace);

    // =========================================================================
    // ZONA: ALMACÉN FÍSICO DE VARIABLES PRIVADAS (ESTADOS Y MEMORIA)
    // =========================================================================
private:

    struct FotoActual{
        QVector<Atomo>      listaAtomos;
        QVector<Enlace>     listaEnlaces;
        int                 contadorIds;
    };

    QVector<FotoActual>     m_atras;
    QVector<FotoActual>     m_adelante;

    ModoEditor              m_modoActual;
    ModoLienzo              m_modoLienzo;
    QString                 m_atomoActivo;

    QVector<Atomo>          m_listaAtomos;
    QVector<Enlace>         m_listaEnlaces;
    QString                 m_comentario;

    int                     m_contadorIds         = 0;
    int                     m_idAtomoSeleccionado = -1;
};

#endif // MOLECULAVIEW_H
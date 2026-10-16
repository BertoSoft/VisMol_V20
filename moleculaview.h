#ifndef MOLECULAVIEW_H
#define MOLECULAVIEW_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QProcess>

// =========================================================================
// ZONA: ESTRUCTURAS DE DATOS GLOBALES (MODELO QUÍMICO BASE)
// =========================================================================
enum TipoFormato {
    VISMOL,
    DAT,
    OUT,
    ARC,
    NO_SE_SABE
};

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
    // Enumeraciones del sistema y estructuras de estado de interfaz
    enum ModoEditor {
        ModoSeleccion,
        ModoDibujo,
        ModoCrearEnlace,
        ModoRotacion3D,
    };

    struct ModoLienzo {
        bool            estaVacio       = true;
        bool            estaGuardado    = false;
        bool            estaIniciado    = false;
        TipoFormato     formato         = TipoFormato::NO_SE_SABE;
    };

    // =========================================================================
    // 1. ZONA: CICLO DE VIDA DEL CEREBRO DE NEGOCIO
    // =========================================================================
public:
    explicit MoleculaView(QObject *parent = nullptr);
    ~MoleculaView() override;

    // =========================================================================
    // 2. ZONA: FUNCIONES PÚBLICAS (CONTROL DE ESTADOS Y HERRAMIENTAS)
    // =========================================================================
public:
    void        setModoEditor(const ModoEditor& nuevoModo);
    void        setElementoActivo(const QString& simbolo);
    void        setComentario(const QString& comentario);
    void        setArgumentos(const QString& argumentos);
    void        setNuevoProyecto();
    void        deshacer();
    void        rehacer();

    // =========================================================================
    // 3. ZONA: FUNCIONES PÚBLICAS (PERSISTENCIA Y ENTRADA/SALIDA UNIFICADA)
    // =========================================================================
public:
    QString     abrirArchivo(const QString& ruta); // <--- Punto único de entrada de lectura
    QString     guardarArchivo(const QString& ruta);

    // =========================================================================
    // 4. ZONA: FUNCIONES PÚBLICAS (INTERACCIÓN CON EL ESCENARIO / RATÓN)
    // =========================================================================
public:
    void        setClick(const QVector3D& posClick);
    void        setClickDerecho(const QVector3D& posClickDerecho);
    void        simularMopac();
    void        detenerSimularMopac();

    // =========================================================================
    // 5. ZONA: FUNCIONES PÚBLICAS (GESTOR DE MEMORIA Y ELIMINACIÓN)
    // =========================================================================
public:
    void        delAtomo(const int& idAtomo);
    void        delEnlace(const int& idEnlace);
    void        limpiarLienzo();
    void        cerrarProyecto();

    // =========================================================================
    // 6. ZONA: GETTERS DE ACCESO A DATOS (ESTRICTO SOLO LECTURA)
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
    QString         getRutaArchivoActual();

    // =========================================================================
    // 7. ZONA: SEÑALES DE NOTIFICACIÓN (MÓVILES DE SINCRONIZACIÓN VISTA-MODELO)
    // =========================================================================
signals:
    void            actualizarLienzo();
    void            modoEditorCambiado(MoleculaView::ModoEditor modo);
    void            modoLienzoCambiado(MoleculaView::ModoLienzo modo);
    void            notificarMensajeEstado(const QString& mensaje);
    void            notificaContenidoLog(const QString& log);
    void            setMenuEliminar(const int& idAtomo, const int& idEnlace);

    // =========================================================================
    // 8. ZONA: MÉTODOS INTERNOS PRIVADOS (PROCESAMIENTO DE CLICKS GRÁFICOS)
    // =========================================================================
private:
    void        procesarClickDibujoAtomo(const QVector3D& posClick);
    void        procesarClickSeleccion(const QVector3D& posClick);
    void        procesarClickCrearEnlace(const QVector3D& posClick);
    void        procesarClickRotacion(const QVector3D& posClick);

    // =========================================================================
    // 9. ZONA: MÉTODOS INTERNOS PRIVADOS (GESTIÓN DE HISTORIAL / PILAS / RESET)
    // =========================================================================
private:
    void        resetVariablesClase();
    void        pushAtras();
    void        popAtras();
    void        pushAdelante();
    void        popAdelante();

    // =========================================================================
    // 10. ZONA: MÉTODOS INTERNOS PRIVADOS (ESTADO DE SALUD Y PARSERS ENCAPSULADOS)
    // =========================================================================
private:
    void        setModoLienzo(const ModoLienzo& modoLienzo);
    void        setMensajeEstado();
    TipoFormato getTipoArchivo(const QString& ruta); // Tu clasificador inteligente de tokens
    QString     loadFileVisMol(const QString& ruta); // Oculto y protegido de la interfaz gráfica
    QString     loadFileMOPAC(const QString& ruta, TipoFormato formato);  // Oculto y protegido de la interfaz gráfica
    QString     saveFileVisMol(const QString& ruta);
    QString     saveFileMOPAC(const QString& ruta, const QString& argumentos);

    // =========================================================================
    // 11. ZONA: MÉTODOS INTERNOS PRIVADOS (VALIDACIONES Y AUXILIARES QUÍMICOS)
    // =========================================================================
private:
    int         getIdAtomoFromPos(const QVector3D& posClick);
    bool        isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2);
    int         getEnlaces(const Atomo& atomo);
    float       getDistanciaPuntoSegmento(const QVector3D& punto, const QVector3D& pos1, const QVector3D& pos2);

    // =========================================================================
    // 12. ZONA: ALMACÉN FÍSICO DE VARIABLES PRIVADAS (ESTADOS Y MEMORIA EN RAM)
    // =========================================================================
private:
    struct FotoActual {
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
    QString                 m_argumentos;
    QString                 m_rutaArchivoActual;
    QVector3D               m_ultimaPosRat;
    QProcess*               m_procesoMopac; // Puntero al subproceso de fondo de MOPAC

    int                     m_contadorIds         = 0;
    int                     m_idAtomoSeleccionado = -1;
};

#endif // MOLECULAVIEW_H
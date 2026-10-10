#ifndef MOLECULAVISTA_H
#define MOLECULAVISTA_H

#include "moleculalienzo.h"
#include "moleculaview.h"

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QActionGroup>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QDockWidget>
#include <QListWidget>
#include <QTextEdit>

class MoleculaView;

class MoleculaVista : public QMainWindow
{
    Q_OBJECT

public:
    explicit MoleculaVista(QWidget *parent = nullptr);
    ~MoleculaVista() override;

    const QString NOMBRE_APP = "VisMol V 2.0";

    // Funciones Públicas
    void setMoleculaView(MoleculaView* view);

private:
    // =========================================================================
    // BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
    // =========================================================================
    void initUi();
    void initMenu();
    void initToolBars();
    void initAnimacionBar();
    void initDockPanels();
    void initPanelConsola();
    void initStatusBar();
    void initConnect();
    void initLienzo();
    void setTemaApp();

private slots:
    // =========================================================================
    // BLOQUE: PULSACIONES DE MENÚ Y ACCIONES LÓGICAS (SLOTS INTERMEDIOS)
    // =========================================================================
    void nuevoProyecto();
    void abrirProyecto();
    void cerrarProyecto();
    void guardarProyecto();
    void salir();
    void importarXYZ();
    void exportarMOPAC();
    void deshacer();
    void rehacer();
    void limpiarLienzo();
    void configurarMopac();
    void mostrarFrecuencias();
    void verLogOut();

    // =========================================================================
    // BLOQUE: GESTIÓN DE BOTONERÍA Y CONTROLES INTERNOS DE INTERFAZ
    // =========================================================================
    void alCambiarModoLienzo(const MoleculaView::ModoLienzo& modoLienzo);
    void alCambiarModoEditor(const MoleculaView::ModoEditor& modoEditor);
    void alSeleccionarElemento(const QString& elemento);
    void alActivarModoSeleccion();
    void alActivarModoEnlace();
    void alActivarModoRotar();
    void alAlternarReproduccionAnimacion(bool activado);
    void alCambiarDeslizadorFotograma(int fotograma);
    void alSeleccionarFrecuencia(int indice);

protected:
    // =========================================================================
    // ZONA: EVENTOS PROTEGIDOS DEL SISTEMA OPERATIVO
    // =========================================================================

    void closeEvent(QCloseEvent *evento) override;

private:
    // Contenedores semánticos de acciones
    struct AccionesArchivo {
        QAction* nuevo    = nullptr;
        QAction* abrir    = nullptr;
        QAction* guardar  = nullptr;
        QAction* cerrar   = nullptr;
        QAction* salir    = nullptr;
    } m_accionesArchivo;

    struct AccionesIO {
        QAction* importarXYZ   = nullptr;
        QAction* exportarMOPAC = nullptr;
    } m_accionesIO;

    struct AccionesEdicion {
        QAction* deshacer      = nullptr;
        QAction* rehacer       = nullptr;
        QAction* limpiarLienzo = nullptr;
    } m_accionesEdicion;

    struct AccionesCalculo {
        QAction* configurarMopac = nullptr;
    } m_accionesCalculo;

    struct AccionesAnálisis {
        QAction* mostrarFreq    = nullptr;
        QAction* mostrarConsola = nullptr;
        QAction* verLogOut      = nullptr;
    } m_accionesAnalisis;

    struct AccionesElementos {
        QAction* carbono        = nullptr;
        QAction* hidrogeno      = nullptr;
        QAction* oxigeno        = nullptr;
        QAction* nitrogeno      = nullptr;
        QAction* modoSeleccion  = nullptr;
        QAction* modoEnlace     = nullptr;
        QAction* modoRotar      = nullptr;
    } m_accionesElementos;

    struct AccionesAnimacion {
        QAction* reproducirPausar   = nullptr;
        QAction* fotogramaSiguiente = nullptr;
        QAction* fotogramaAnterior  = nullptr;
    } m_accionesAnimacion;

    // Componentes gráficos del sistema de ventanas
    QToolBar*      m_barraProyecto        = nullptr;
    QToolBar*      m_barraElementos       = nullptr;
    QToolBar*      m_barraAnimacion       = nullptr;
    QActionGroup*  m_grupoElementos       = nullptr;

    QStatusBar*    m_barraEstado          = nullptr;
    QLabel*        m_lblEstadoTexto       = nullptr;
    QLabel*        m_lblEstadoFecha       = nullptr;
    QLabel*        m_lblEstadoHora        = nullptr;

    QSlider*       m_deslizadorFotogramas = nullptr;
    QLabel*        m_lblFotogramaInfo     = nullptr;

    QDockWidget*   m_panelFrecuencias   = nullptr;
    QDockWidget*   m_panelConsola       = nullptr;
    QTextEdit*     m_textoConsola       = nullptr;
    QListWidget*   m_listaFrecuencias   = nullptr;

    MoleculaLienzo* m_lienzo            = nullptr;
    MoleculaView*   m_view              = nullptr;

    //#####################################################################
    // Funciones Privadas
    //#####################################################################

    bool sePuedeCerrar();
    void setMenuBotonDerecho(const int& idAtomo, const int& idEnlace);
};
#endif // MOLECULAVISTA_H
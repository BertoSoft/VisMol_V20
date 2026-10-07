#ifndef MOLECULAVISTA_H
#define MOLECULAVISTA_H

#include "moleculalienzo.h"

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QActionGroup>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QDockWidget>
#include <QListWidget>

class MoleculaView;

class MoleculaVista : public QMainWindow
{
    Q_OBJECT

public:
    explicit MoleculaVista(QWidget *parent = nullptr);
    ~MoleculaVista() override;

    const QString NOMBRE_APP = "VisMol V 2.0";

private:
    // =========================================================================
    // BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
    // =========================================================================
    void initUi();
    void initMenu();
    void initToolBars();
    void initAnimationBar();
    void initDockPanels();
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
    void alSeleccionarElemento(const QString& elemento);
    void alActivarModoEnlace();
    void alActivarModoRotar();
    void alAlternarReproduccionAnimacion(bool activado);
    void alCambiarDeslizadorFotograma(int fotograma);
    void alSeleccionarFrecuencia(int indice);

protected:
    void closeEvent(QCloseEvent *evento) override;

private:
    // Contenedores semánticos de acciones
    struct AccionesArchivo {
        QAction* nuevo    = nullptr;
        QAction* abrir    = nullptr;
        QAction* guardar  = nullptr;
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
        QAction* mostrarFreq = nullptr;
        QAction* verLogOut   = nullptr;
    } m_accionesAnalisis;

    struct AccionesElementos {
        QAction* carbono    = nullptr;
        QAction* hidrogeno  = nullptr;
        QAction* oxigeno    = nullptr;
        QAction* nitrogeno  = nullptr;
        QAction* modoEnlace = nullptr;
        QAction* modoRotar  = nullptr;
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

    QDockWidget*   m_panelFrecuencias     = nullptr;
    QListWidget*   m_listaFrecuencias     = nullptr;

    MoleculaLienzo* m_lienzo              = nullptr;
    MoleculaView*   m_negocio             = nullptr;
};
#endif // MOLECULAVISTA_H
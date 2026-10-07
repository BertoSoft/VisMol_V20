#ifndef MOLECULAVIEW_H
#define MOLECULAVIEW_H


#include "moleculalienzo.h"

#include <QMainWindow>
#include <QToolBar>
#include <QActionGroup>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QDockWidget>
#include <QListWidget>

class MoleculaView
{
    Q_OBJECT

public:
    explicit MoleculaVista(QWidget *parent = nullptr);
    ~MoleculaVista() override;

    static inline const QString NOMBRE_APP = "VisMol V 2.0";

private:
    // -------------------------------------------------------------
    // FILOSOFÍA DE INICIALIZACIÓN POR CAPAS MODULARES EN CASTELLANO
    // -------------------------------------------------------------
    void inicializarInterfaz();
    void inicializarMenu();
    void inicializarBarrasHerramientas(); // Barras superiores y lateral
    void inicializarBarraAnimacion();     // Barra inferior de reproducción
    void inicializarBarraEstado();        // Barra inferior con reloj y estado
    void inicializarPanelesAcoplables();  // Paneles laterales (Frecuencias)
    void inicializarConexiones();
    void actualizarEstadosInterfaz();

private slots:
    // Intermediarios de la UI: Capturan eventos antes de delegar en el negocio
    void alActivarNuevoProyecto();
    void alActivarAbrirProyecto();
    void alActivarGuardarProyecto();
    void alActivarSalir();
    void alActivarImportarXYZ();
    void alActivarExportarMOPAC();
    void alActivarDeshacer();
    void alActivarRehacer();
    void alActivarLimpiarLienzo();
    void alActivarConfigurarMopac();
    void alActivarMostrarFrecuencias();
    void alActivarVerLogOut();

    // Slots locales para el control de la simulación y herramientas
    void alSeleccionarElemento(const QString& elemento);
    void alActivarModoEnlace();
    void alActivarModoRotar();
    void alAlternarReproduccionAnimacion(bool activado);
    void alCambiarDeslizadorFotograma(int fotograma);
    void alSeleccionarFrecuencia(int indice);

protected:
    void closeEvent(QCloseEvent *evento) override;

private:
    // -------------------------------------------------------------
    // CONTENEDORES SEMÁNTICOS DE ACCIONES
    // -------------------------------------------------------------
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

    // -------------------------------------------------------------
    // COMPONENTES GRÁFICOS DE LA VENTANA PRINCIPAL
    // -------------------------------------------------------------
    QToolBar*      m_barraProyecto      = nullptr;
    QToolBar*      m_barraElementos     = nullptr; // Barra lateral izquierda
    QToolBar*      m_barraAnimacion     = nullptr; // Reproductor inferior
    QActionGroup*  m_grupoElementos     = nullptr;

    // Barra de Estado y Etiquetas internas
    QStatusBar*    m_barraEstado        = nullptr;
    QLabel*        m_lblEstadoTexto     = nullptr;
    QLabel*        m_lblEstadoFecha     = nullptr;
    QLabel*        m_lblEstadoHora      = nullptr;

    // Controles de la barra de animación
    QSlider*       m_deslizadorFotogramas = nullptr;
    QLabel*        m_lblFotogramaInfo     = nullptr;

    // Paneles Acoplables de Análisis Avanzado
    QDockWidget*   m_panelFrecuencias   = nullptr;
    QListWidget*   m_listaFrecuencias   = nullptr;


    bool m_isModificado = false;
};
#endif // MOLECULAVIEW_H

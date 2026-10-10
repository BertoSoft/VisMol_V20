#include "moleculavista.h"
#include "moleculaview.h"

#include <QMenu>
#include <QMenuBar>
#include <QGraphicsView>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <QCloseEvent>
#include <QMessageBox>
#include <QCursor>

// =========================================================================
// ZONA: CICLO DE VIDA DE LA VENTANA
// =========================================================================

MoleculaVista::MoleculaVista(QWidget *parent)
    : QMainWindow(parent){

    initUi();
}

MoleculaVista::~MoleculaVista() {}

//#########################################################################
// Zona de Funciones Públicas
//########################################################################

void MoleculaVista::setMoleculaView(MoleculaView* view){
    if(!view) return;

    m_view = view; // La ventana guarda el cerebro

    // Comparte el cerebro con el lienzo privado de forma limpia
    if(m_lienzo) {
        m_lienzo->setMoleculaView(view);
        connect(m_view, &MoleculaView::actualizarLienzo, m_lienzo, &MoleculaLienzo::actualizarLienzo);
    }

    // Cable 1: Gobierna la salud del documento (Persistencia, Datos, Docks)
    connect(m_view, &MoleculaView::modoLienzoCambiado, this, &MoleculaVista::alCambiarModoLienzo);

    // Cable 2: Gobierna la sincronización visual de botones hundidos (Ratón)
    connect(m_view, &MoleculaView::modoEditorCambiado, this, &MoleculaVista::alCambiarModoEditor);

    // Cable 3 (¡NUEVO!): Desvía el texto generado por el negocio hacia la barra de estado de la UI
    connect(m_view, &MoleculaView::notificarMensajeEstado, m_lblEstadoTexto, &QLabel::setText);

    // Cabñle 4 conecta la señal de menuEliminar con el menu de boton derecho
    connect(m_view, &MoleculaView::setMenuEliminar, this, &MoleculaVista::setMenuBotonDerecho);
}

// =========================================================================
// ZONA: Funciones Privadas
// =========================================================================

bool MoleculaVista::sePuedeCerrar(){
    if (!m_view) return true;

    MoleculaView::ModoLienzo modoLienzo = m_view->getModoLienzo();

    // Si no hay cambios sin guardar o está vacío, es seguro continuar
    if (modoLienzo.estaGuardado || modoLienzo.estaVacio) {
        return true;
    }

    // Si hay peligro, abrimos el cuadro de diálogo (responsabilidad de la Vista)
    QMessageBox::StandardButton respuesta = QMessageBox::warning(
        this,
        tr("Cambios sin guardar"),
        tr("El proyecto actual tiene cambios sin guardar.\n¿Desea guardarlos antes de continuar?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

    if (respuesta == QMessageBox::Save) {
        guardarProyecto();
        return true; // Continuamos tras guardar
    }
    else if (respuesta == QMessageBox::Discard) {
        return true; // Continuamos descartando los cambios
    }

    return false; // El usuario pulsó Cancelar, detenemos cualquier acción
}

void MoleculaVista::setMenuBotonDerecho(const int& idAtomo, const int& idEnlace){
    QMenu menuDerecho(this);

    // CASO A: El clic derecho cayó sobre un átomo válido
    if (idAtomo >= 0) {
        QAction* actEliminarAtomo = menuDerecho.addAction(tr("Eliminar Átomo"));
        QAction* accionSeleccionada = menuDerecho.exec(QCursor::pos());

        if (accionSeleccionada == actEliminarAtomo) {
            m_view->delAtomo(idAtomo); // El cerebro limpia la memoria
        }
        return; // ¡CRUCIAL! Salimos inmediatamente del método para evitar lecturas de basura
    }

    // CASO B: El clic derecho cayó sobre un enlace limpio
    if (idEnlace >= 0) {
        QAction* actEliminarEnlace = menuDerecho.addAction(tr("Eliminar Enlace"));
        QAction* accionSeleccionada = menuDerecho.exec(QCursor::pos());

        if (accionSeleccionada == actEliminarEnlace) {
            m_view->delEnlace(idEnlace);
        }
        return;
    }
}

// =========================================================================
// ZONA: INICIALIZACIONES DE INTERFAZ DE USUARIO (init)
// =========================================================================

void MoleculaVista::initUi(){

    setWindowTitle(NOMBRE_APP);

    initMenu();
    initToolBars();
    initAnimacionBar();
    initDockPanels();
    initPanelConsola();
    initStatusBar();
    initLienzo();
    initConnect();
    setTemaApp();
}

void MoleculaVista::initMenu(){

    QMenuBar* menuBarSuperior = menuBar();

    QMenu* menuArchivo = menuBarSuperior->addMenu(tr("&Archivo"));

    // CORREGIDO: El atajo de teclado pasa a ser el segundo argumento, antes de 'this'
    m_accionesArchivo.nuevo = menuArchivo->addAction(tr("Nuevo Proyecto"), QKeySequence::New, this, &MoleculaVista::nuevoProyecto);
    m_accionesArchivo.nuevo->setIcon(QIcon(":/iconos/nuevo.png"));

    m_accionesArchivo.abrir = menuArchivo->addAction(tr("Abrir Proyecto (.json)"), QKeySequence::Open, this, &MoleculaVista::abrirProyecto);
    m_accionesArchivo.abrir->setIcon(QIcon(":/iconos/abrir.png"));

    m_accionesArchivo.cerrar = menuArchivo->addAction(tr("Cerrar Proyecto"), QKeySequence::Close, this, &MoleculaVista::cerrarProyecto);
    m_accionesArchivo.cerrar->setIcon(QIcon(":/iconos/cerrar.png"));

    m_accionesArchivo.guardar = menuArchivo->addAction(tr("Guardar Proyecto (.json)"), QKeySequence::Save, this, &MoleculaVista::guardarProyecto);
    m_accionesArchivo.guardar->setIcon(QIcon(":/iconos/guardar.png"));

    menuArchivo->addSeparator();
    m_accionesArchivo.salir = menuArchivo->addAction(tr("Salir"), QKeySequence::Quit, this, &MoleculaVista::salir);
    m_accionesArchivo.salir->setIcon(QIcon(":/iconos/salir.png"));

    QMenu* menuFormatos = menuBarSuperior->addMenu(tr("&Importar/Exportar"));
    m_accionesIO.importarXYZ = menuFormatos->addAction(tr("Importar Coordenadas XYZ (.xyz)"), this, &MoleculaVista::importarXYZ);
    m_accionesIO.exportarMOPAC = menuFormatos->addAction(tr("Exportar Entrada MOPAC (.mop)"), this, &MoleculaVista::exportarMOPAC);

    QMenu* menuEdicion = menuBarSuperior->addMenu(tr("&Edición"));
    m_accionesEdicion.deshacer = menuEdicion->addAction(tr("Deshacer"), QKeySequence::Undo, this, &MoleculaVista::deshacer);
    m_accionesEdicion.deshacer->setIcon(QIcon(":/iconos/deshacer.png"));

    m_accionesEdicion.rehacer = menuEdicion->addAction(tr("Rehacer"), QKeySequence::Redo, this, &MoleculaVista::rehacer);
    m_accionesEdicion.rehacer->setIcon(QIcon(":/iconos/rehacer.png"));

    menuEdicion->addSeparator();
    m_accionesEdicion.limpiarLienzo = menuEdicion->addAction(tr("Limpiar Lienzo"), this, &MoleculaVista::limpiarLienzo);
    m_accionesEdicion.limpiarLienzo->setIcon(QIcon(":/iconos/limpiar.png"));

    QMenu* menuCalculo = menuBarSuperior->addMenu(tr("&Cálculo"));
    m_accionesCalculo.configurarMopac = menuCalculo->addAction(tr("Configurar Simulación MOPAC..."), this, &MoleculaVista::configurarMopac);
    m_accionesCalculo.configurarMopac->setIcon(QIcon(":/iconos/configurar.png"));

    QMenu* menuAnalisis = menuBarSuperior->addMenu(tr("&Análisis"));
    m_accionesAnalisis.mostrarFreq = menuAnalisis->addAction(tr("Mostrar Analizador Vibracional"), this, &MoleculaVista::mostrarFrecuencias);
    m_accionesAnalisis.mostrarConsola = menuAnalisis->addAction(tr("Mostrar Monitor de Simulación"), this, [this](){
        if(m_panelConsola) m_panelConsola->setVisible(!m_panelConsola->isVisible());
    });
    m_accionesAnalisis.verLogOut = menuAnalisis->addAction(tr("Ver Log de Salida MOPAC (.out)"), this, &MoleculaVista::verLogOut);
}

void MoleculaVista::initToolBars(){

    m_barraProyecto = addToolBar(tr("Proyecto"));
    m_barraProyecto->setObjectName("barraProyecto");
    m_barraProyecto->addAction(m_accionesArchivo.salir);
    m_barraProyecto->addAction(m_accionesArchivo.nuevo);
    m_barraProyecto->addAction(m_accionesArchivo.abrir);
    m_barraProyecto->addAction(m_accionesArchivo.guardar);
    m_barraProyecto->addAction(m_accionesArchivo.cerrar);
    m_barraProyecto->addSeparator();
    m_barraProyecto->addSeparator();
    m_barraProyecto->addAction(m_accionesEdicion.deshacer);
    m_barraProyecto->addAction(m_accionesEdicion.rehacer);
    m_barraProyecto->addAction(m_accionesEdicion.limpiarLienzo);
    m_barraProyecto->addSeparator();
    m_barraProyecto->addSeparator();
    m_barraProyecto->addAction(m_accionesCalculo.configurarMopac);

    m_barraElementos = new QToolBar(tr("Herramientas de Elementos"), this);
    m_barraElementos->setObjectName("barraElementos");
    addToolBar(Qt::LeftToolBarArea, m_barraElementos);

    m_accionesElementos.carbono = new QAction(tr("Carbono (C)"), this);
    m_accionesElementos.carbono->setCheckable(true);

    m_accionesElementos.hidrogeno = new QAction(tr("Hidrógeno (H)"), this);
    m_accionesElementos.hidrogeno->setCheckable(true);

    m_accionesElementos.oxigeno = new QAction(tr("Oxígeno (O)"), this);
    m_accionesElementos.oxigeno->setCheckable(true);

    m_accionesElementos.nitrogeno = new QAction(tr("Nitrógeno (N)"), this);
    m_accionesElementos.nitrogeno->setCheckable(true);

    m_accionesElementos.modoSeleccion = new QAction(tr("Seleccionar / Puntero"), this);
    m_accionesElementos.modoSeleccion->setCheckable(true);

    m_accionesElementos.modoEnlace = new QAction(tr("Crear Enlace"), this);
    m_accionesElementos.modoEnlace->setCheckable(true);

    m_accionesElementos.modoRotar = new QAction(tr("Rotar Molécula"), this);
    m_accionesElementos.modoRotar->setCheckable(true);

    m_grupoElementos = new QActionGroup(this);
    m_grupoElementos->addAction(m_accionesElementos.carbono);
    m_grupoElementos->addAction(m_accionesElementos.hidrogeno);
    m_grupoElementos->addAction(m_accionesElementos.oxigeno);
    m_grupoElementos->addAction(m_accionesElementos.nitrogeno);
    m_grupoElementos->addAction(m_accionesElementos.modoSeleccion);
    m_grupoElementos->addAction(m_accionesElementos.modoEnlace);
    m_grupoElementos->addAction(m_accionesElementos.modoRotar);
    m_grupoElementos->setExclusive(true);

    m_barraElementos->addAction(m_accionesElementos.carbono);
    m_barraElementos->addAction(m_accionesElementos.hidrogeno);
    m_barraElementos->addAction(m_accionesElementos.oxigeno);
    m_barraElementos->addAction(m_accionesElementos.nitrogeno);
    m_barraElementos->addSeparator();
    m_barraElementos->addSeparator();
    m_barraElementos->addSeparator();
    m_barraElementos->addAction(m_accionesElementos.modoSeleccion);
    m_barraElementos->addAction(m_accionesElementos.modoEnlace);
    m_barraElementos->addAction(m_accionesElementos.modoRotar);
}

void MoleculaVista::initPanelConsola(){
    m_panelConsola = new QDockWidget(tr("Monitor de Simulación en Vivo (MOPAC)"), this);
    m_panelConsola->setObjectName("panelConsolaMopac");
    m_panelConsola->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    m_textoConsola = new QTextEdit(m_panelConsola);
    m_textoConsola->setObjectName("textConsolaMopac");
    m_textoConsola->setReadOnly(true);

    // Texto de simulación inicial simulado para desarrollo visual
    m_textoConsola->append("--- VISMOL: ESPERANDO EJECUCIÓN DEL MOTOR MOPAC ---");
    m_textoConsola->append(" > Use 'Configurar Simulación MOPAC...' para iniciar.");

    m_panelConsola->setWidget(m_textoConsola);
    addDockWidget(Qt::BottomDockWidgetArea, m_panelConsola);

}

void MoleculaVista::initAnimacionBar(){
    m_barraAnimacion = new QToolBar(tr("Control de Trayectoria / Animación"), this);
    m_barraAnimacion->setObjectName("barraAnimacion");
    addToolBar(Qt::BottomToolBarArea, m_barraAnimacion);

    // =========================================================================
    // MODIFICACIÓN DE CENTRADO COMPLETO (WIDGETS ESPACIADORES ELÁSTICOS)
    // =========================================================================
    // 1. Creamos el espaciador izquierdo
    QWidget* espaciadorIzquierdo = new QWidget(this);
    espaciadorIzquierdo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_barraAnimacion->addWidget(espaciadorIzquierdo);

    // 2. Instanciamos las acciones de reproducción normales
    m_accionesAnimacion.reproducirPausar = new QAction(tr("Reproducir"), this);
    m_accionesAnimacion.reproducirPausar->setCheckable(true);

    m_accionesAnimacion.fotogramaAnterior = new QAction(tr("<< Paso Anter."), this);
    m_accionesAnimacion.fotogramaSiguiente = new QAction(tr("Paso Siguiente >>"), this);

    m_deslizadorFotogramas = new QSlider(Qt::Horizontal, this);
    m_deslizadorFotogramas->setMinimum(1);
    m_deslizadorFotogramas->setMaximum(10);
    m_deslizadorFotogramas->setFixedWidth(250);

    m_lblFotogramaInfo = new QLabel(tr("Paso: 1 / 10"), this);
    m_lblFotogramaInfo->setStyleSheet("padding: 0 8px; font-weight: bold;");

    // 3. Añadimos el bloque compacto de controles al lienzo de la barra
    m_barraAnimacion->addAction(m_accionesAnimacion.fotogramaAnterior);
    m_barraAnimacion->addAction(m_accionesAnimacion.reproducirPausar);
    m_barraAnimacion->addAction(m_accionesAnimacion.fotogramaSiguiente);
    m_barraAnimacion->addWidget(m_deslizadorFotogramas);
    m_barraAnimacion->addWidget(m_lblFotogramaInfo);

    // 4. Creamos el espaciador derecho para cerrar el sándwich de centrado
    QWidget* espaciadorDerecho = new QWidget(this);
    espaciadorDerecho->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_barraAnimacion->addWidget(espaciadorDerecho);
}

void MoleculaVista::initDockPanels(){
    m_panelFrecuencias = new QDockWidget(tr("Analizador de Modos Vibracionales (IR)"), this);
    m_panelFrecuencias->setObjectName("panelFrecuencias");
    m_panelFrecuencias->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    m_listaFrecuencias = new QListWidget(m_panelFrecuencias);
    m_listaFrecuencias->setObjectName("listaFrecuencias");
    m_listaFrecuencias->addItem("3120.4 cm⁻¹ - Tensión C-H (Simétrica)");
    m_listaFrecuencias->addItem("1650.1 cm⁻¹ - Estiramiento C=C");
    m_listaFrecuencias->addItem("1430.5 cm⁻¹ - Flexión en el plano");

    m_panelFrecuencias->setWidget(m_listaFrecuencias);
    addDockWidget(Qt::RightDockWidgetArea, m_panelFrecuencias);
}

void MoleculaVista::initStatusBar(){
    m_barraEstado = new QStatusBar(this);
    setStatusBar(m_barraEstado);

    m_lblEstadoTexto = new QLabel(tr(" Listo para iniciar proyecto."), this);
    m_lblEstadoFecha = new QLabel(this);
    m_lblEstadoHora  = new QLabel(this);

    QList<QLabel*> etiquetas = { m_lblEstadoTexto, m_lblEstadoFecha, m_lblEstadoHora };
    for (QLabel* etiqueta : etiquetas) {
        etiqueta->setFrameShape(QFrame::Panel);
        etiqueta->setFrameShadow(QFrame::Sunken);
        etiqueta->setAlignment(Qt::AlignCenter);
    }
    m_lblEstadoTexto->setAlignment(Qt::AlignLeft);

    m_barraEstado->addWidget(m_lblEstadoTexto, 1);
    m_barraEstado->addPermanentWidget(m_lblEstadoFecha);
    m_barraEstado->addPermanentWidget(m_lblEstadoHora);

    m_lblEstadoFecha->setText(QDate::currentDate().toString("dddd, d 'de' MMMM 'de' yyyy"));
    m_lblEstadoHora->setText(QTime::currentTime().toString("hh:mm:ss"));

    QTimer* timerReloj = new QTimer(this);
    connect(timerReloj, &QTimer::timeout, this, [this]() {
        m_lblEstadoFecha->setText(QDate::currentDate().toString("dddd, d 'de' MMMM 'de' yyyy"));
        m_lblEstadoHora->setText(QTime::currentTime().toString("hh:mm:ss"));
    });
    timerReloj->start(1000);
}

void MoleculaVista::initLienzo(){
    // Instanciamos tu clase personalizada pasando 'this' como padre visual
    m_lienzo = new MoleculaLienzo(this);

    // Le asignamos el nombre de objeto para que la hoja de estilos QSS lo pinte de negro
    m_lienzo->setObjectName("visorMolecular");

    // Lo colocamos de forma oficial en el centro de la pantalla
    setCentralWidget(m_lienzo);
}

void MoleculaVista::initConnect(){
    connect(m_accionesElementos.carbono, &QAction::triggered, this, [this](){ alSeleccionarElemento("C"); });
    connect(m_accionesElementos.hidrogeno, &QAction::triggered, this, [this](){ alSeleccionarElemento("H"); });
    connect(m_accionesElementos.oxigeno, &QAction::triggered, this, [this](){ alSeleccionarElemento("O"); });
    connect(m_accionesElementos.nitrogeno, &QAction::triggered, this, [this](){ alSeleccionarElemento("N"); });
    connect(m_accionesElementos.modoSeleccion, &QAction::triggered, this, &MoleculaVista::alActivarModoSeleccion);
    connect(m_accionesElementos.modoEnlace, &QAction::triggered, this, &MoleculaVista::alActivarModoEnlace);
    connect(m_accionesElementos.modoRotar, &QAction::triggered, this, &MoleculaVista::alActivarModoRotar);

    connect(m_accionesAnimacion.reproducirPausar, &QAction::toggled, this, &MoleculaVista::alAlternarReproduccionAnimacion);
    connect(m_deslizadorFotogramas, &QSlider::valueChanged, this, &MoleculaVista::alCambiarDeslizadorFotograma);

    connect(m_listaFrecuencias, &QListWidget::currentRowChanged, this, &MoleculaVista::alSeleccionarFrecuencia);

    // Iniciamos los valores de Fabrica
    MoleculaView::ModoLienzo modo;

    modo.estaVacio      = true;
    modo.estaGuardado   = false;

    alCambiarModoLienzo(modo);
    alActivarModoSeleccion();
}

// =========================================================================
// ZONA: PULSACIONES DE MENÚ SUPERIOR (SLOTS)
// =========================================================================

void MoleculaVista::nuevoProyecto(){
    if (sePuedeCerrar()) {
        m_view->setNuevoProyecto(); // El cerebro hace el trabajo limpio
    }
}
void MoleculaVista::abrirProyecto()   {}
void MoleculaVista::guardarProyecto() {}
void MoleculaVista::cerrarProyecto(){
    if (sePuedeCerrar()){
        m_view->cerrarProyecto(); // El cerebro hace el trabajo limpio
    }
}
void MoleculaVista::salir()           { this->close(); }
void MoleculaVista::importarXYZ()     {}
void MoleculaVista::exportarMOPAC()   {}
void MoleculaVista::deshacer()        {}
void MoleculaVista::rehacer()         {}
void MoleculaVista::limpiarLienzo()   {m_view->limpiarLienzo();}
void MoleculaVista::configurarMopac() {}
void MoleculaVista::mostrarFrecuencias(){}
void MoleculaVista::verLogOut()        {}

// =========================================================================
// ZONA: INTERACCIONES Y BOTONERÍA DE INTERFAZ
// =========================================================================

void MoleculaVista::alCambiarModoLienzo(const MoleculaView::ModoLienzo& modoLienzo){
    // =========================================================================
    // REGLA 1: GESTIÓN DE LA BARRA DE HERRAMIENTAS LATERAL (FÁBRICA)
    // =========================================================================
    // La barra de elementos se enciende única y exclusivamente si hay un proyecto activo
    m_barraElementos->setEnabled(modoLienzo.estaIniciado);

    // =========================================================================
    // REGLA 2: CONTROL POR CONTENIDO (estaVacio) - DISPONIBILIDAD DE ACTIONS
    // =========================================================================
    m_accionesArchivo.cerrar->setEnabled(!modoLienzo.estaVacio);
    m_accionesEdicion.limpiarLienzo->setEnabled(!modoLienzo.estaVacio);
    m_accionesIO.exportarMOPAC->setEnabled(!modoLienzo.estaVacio);
    m_accionesCalculo.configurarMopac->setEnabled(!modoLienzo.estaVacio);
    m_accionesEdicion.deshacer->setEnabled(!modoLienzo.estaVacio);
    m_accionesEdicion.rehacer->setEnabled(!modoLienzo.estaVacio);
    m_accionesElementos.modoEnlace->setEnabled(!modoLienzo.estaVacio);
    m_accionesElementos.modoRotar->setEnabled(!modoLienzo.estaVacio);
    m_accionesAnalisis.mostrarFreq->setEnabled(!modoLienzo.estaVacio);
    m_accionesAnalisis.mostrarConsola->setEnabled(!modoLienzo.estaVacio);
    m_accionesAnalisis.verLogOut->setEnabled(!modoLienzo.estaVacio);

    // =========================================================================
    // REGLA 3: CONTROL DE INTERFACES ACOPLABLES Y BARRA DE ESTADO (TEXTO)
    // =========================================================================
    if (modoLienzo.estaVacio) {
        m_panelFrecuencias->setVisible(false);
        m_panelConsola->setVisible(false);
        m_barraAnimacion->setVisible(false);
    }

    // =========================================================================
    // REGLA 4: CONTROL POR PERSISTENCIA (estaGuardado)
    // =========================================================================
    m_accionesArchivo.guardar->setEnabled(!modoLienzo.estaGuardado && !modoLienzo.estaVacio);

    // =========================================================================
    // REGLA 5: TÍTULO DINÁMICO DE LA VENTANA
    // =========================================================================
    if (modoLienzo.estaVacio) {
        setWindowTitle(NOMBRE_APP);
    } else if (modoLienzo.estaGuardado) {
        setWindowTitle(NOMBRE_APP + tr(" - Proyecto Guardado"));
    } else {
        setWindowTitle(NOMBRE_APP + tr(" - Proyecto Actual* (Cambios sin guardar)"));
    }
}

void MoleculaVista::alCambiarModoEditor(const MoleculaView::ModoEditor& modoEditor){
    if (!m_view || !m_lienzo) return; // Protección de punteros

    // 1. Sincronización visual de la botonería (Efecto hundido)
    switch(modoEditor) {
    case MoleculaView::ModoSeleccion:
        m_accionesElementos.modoSeleccion->setChecked(true);
        m_lienzo->setCursor(Qt::ArrowCursor); // Flecha neutra profesional
        break;

    case MoleculaView::ModoDibujo:
        if (m_view->getSimboloAtomoActivo() == "C") m_accionesElementos.carbono->setChecked(true);
        else if (m_view->getSimboloAtomoActivo() == "H") m_accionesElementos.hidrogeno->setChecked(true);
        else if (m_view->getSimboloAtomoActivo() == "O") m_accionesElementos.oxigeno->setChecked(true);
        else if (m_view->getSimboloAtomoActivo() == "N") m_accionesElementos.nitrogeno->setChecked(true);
        m_lienzo->setCursor(Qt::CrossCursor); // Cruz de precisión para dibujar
        break;

    case MoleculaView::ModoCrearEnlace:
        m_accionesElementos.modoEnlace->setChecked(true);
        m_lienzo->setCursor(Qt::PointingHandCursor); // Mano para seleccionar átomos a unir
        break;

    case MoleculaView::ModoRotacion3D:
        m_accionesElementos.modoRotar->setChecked(true);
        m_lienzo->setCursor(Qt::OpenHandCursor); // Mano abierta para arrastrar/rotar espacio
        break;
    }
}

void MoleculaVista::alSeleccionarElemento(const QString& elemento){
    if(m_view){
        m_view->setElementoActivo(elemento);
        m_view->setModoEditor(MoleculaView::ModoDibujo);
    }
}

void MoleculaVista::alActivarModoSeleccion(){
    if(m_view) m_view->setModoEditor(MoleculaView::ModoSeleccion);
}

void MoleculaVista::alActivarModoEnlace(){
    if(m_view) m_view->setModoEditor(MoleculaView::ModoCrearEnlace);
}

void MoleculaVista::alActivarModoRotar(){
    if(m_view) m_view->setModoEditor(MoleculaView::ModoRotacion3D);
}

void MoleculaVista::alAlternarReproduccionAnimacion(bool activado){
    m_accionesAnimacion.reproducirPausar->setText(activado ? tr("Pausar") : tr("Reproducir"));
}

void MoleculaVista::alCambiarDeslizadorFotograma(int fotograma){
    m_lblFotogramaInfo->setText(tr("Paso: %1 / %2").arg(fotograma).arg(m_deslizadorFotogramas->maximum()));
}

void MoleculaVista::alSeleccionarFrecuencia(int indice){
    if (indice < 0) return;
    QString textoModo = m_listaFrecuencias->item(indice)->text();
}



// =========================================================================
// ZONA: EVENTOS PROTEGIDOS DEL SISTEMA OPERATIVO
// =========================================================================
void MoleculaVista::closeEvent(QCloseEvent *evento){
    if (sePuedeCerrar()) {
        evento->accept(); // Cierra la aplicación de forma segura
    } else {
        evento->ignore(); // Cancela el cierre de la ventana
    }
}

// =========================================================================
// ZONA: GESTIÓN DE HOJAS DE ESTILO (QSS)
// =========================================================================

void MoleculaVista::setTemaApp(){

    QString estilo =
        // Ventana Principal y Textos base
        "QMainWindow { "
        "background-color: #181818; "
        "color: #e5e5e5; "
        "font-family: 'Segoe UI', Arial, sans-serif; "
        "}"

        // Menú Superior - Unificado con un azul moderno y sutil
        "QMenuBar { "
        "background-color: #222222; "
        "color: #cccccc; "
        "border-bottom: 1px solid #2d2d2d; "
        "padding: 2px; "
        "}"

        "QMenuBar::item { "
        "padding: 4px 10px; "
        "border-radius: 4px; "
        "}"

        "QMenuBar::item:selected { "
        "background-color: #2a2a2a; "
        "color: #ffffff; "
        "}"

        "QMenu { "
        "background-color: #222222; "
        "color: #e5e5e5; "
        "border: 1px solid #333333; "
        "padding: 4px; "
        "}"

        "QMenu::item { "
        "padding: 6px 25px 6px 20px; "
        "border-radius: 3px; "
        "}"

        "QMenu::item:selected { "
        "background-color: #0f52ba; "
        "color: #ffffff; "
        "}" // Azul Zafiro comercial

        "QMenu::separator { "
        "height: 1px; "
        "background: #333333; "
        "margin: 4px 0; "
        "}"

        // Barras de Herramientas - Limpieza de Paddings e Iconos Estables
        "QToolBar { "
        "background-color: #222222; "
        "border: 1px solid #2d2d2d; "
        "spacing: 6px; "
        "padding: 4px; "
        "}"

        "QToolBar QToolButton { "
        "background-color: #2b2b2b; "
        "color: #e0e0e0; "
        "border: 1px solid #3a3a3a; "
        "border-radius: 4px; "
        "padding: 4px 8px; "
        "font-size: 13px; "
        "icon-size: 20px 20px; "
        "}"

        "QToolBar QToolButton:hover { "
        "background-color: #383838; "
        "border: 1px solid #4a4a4a; "
        "color: #ffffff; "
        "}"

        "QToolBar QToolButton:checked { "
        "background-color: #0f52ba; "
        "border: 1px solid #3b82f6; "
        "color: #ffffff; "
        "font-weight: bold; "
        "}"

        // Ajuste barra de elementos (Adiós a los 120px forzados incómodos)
        "QToolBar#barraElementos QToolButton { "
        "   min-width: 160px; "         /* <--- Fuerzas el ancho mínimo */
        "   max-width: 160px; "         /* <--- Fuerzas el ancho máximo (juntos hacen ancho fijo) */
        "   text-align: left; "         /* <--- Alinea el texto a la izquierda para un look limpio */
        "   padding: 8px 12px; "        /* <--- Espaciado interno cómodo */
        "   font-size: 14px; "          /* <--- Tamaño de letra legible que configuramos antes */
        "   margin-bottom: 2px; "       /* <--- Separación sutil vertical entre botones */
        "}"

        // Lienzo Central
        "QGraphicsView#visorMolecular { "
        "background-color: #101010; "
        "border: 1px solid #252525; "
        "border-radius: 4px; "
        "}"

        // Paneles Acoplables (Docks) - Más finos y profesionales
        "QDockWidget { "
        "color: #bbbbbb; "
        "font-weight: bold; "
        "font-size: 12px; "
        "}"

        "QDockWidget::title { "
        "background-color: #1c1c1c; "
        "text-align: left; "
        "padding: 8px 12px; "
        "border-bottom: 1px solid #2d2d2d; "
        "}"

        // Listas (Frecuencias) - Un verde esmeralda más tecnológico y menos "Matrix"
        "QListWidget#listaFrecuencias { "
        "background-color: #141414; "
        "color: #39ff14; "
        "border: 1px solid #2d2d2d; "
        "border-radius: 4px; "
        "font-family: 'Consolas', 'Courier New', monospace; "
        "font-size: 12px; "
        "padding: 6px; "
        "}"

        "QListWidget#listaFrecuencias::item { "
        "padding: 6px; "
        "border-radius: 3px; "
        "}"

        "QListWidget#listaFrecuencias::item:selected { "
        "background-color: #0f52ba; "
        "color: #ffffff; "
        "}"

        // Controles de Animación
        "QLabel#lblFotogramaInfo { "
        "color: #3b82f6; "
        "font-weight: bold; "
        "font-size: 14px; "
        "}"

        // Barra de Estado
        "QStatusBar { "
        "   background-color: #151515; " // Fondo base ligeramente más oscuro
        "   border-top: 1px solid #2d2d2d; "
        "}"

        "QStatusBar QLabel { "
        "   font-size: 13px; "
        "   color: #cccccc; "
        "   padding: 4px 12px; "
        "   background-color: #101010; " /* Fondo interior más oscuro para simular el hueco */
            /* Bordes simulados para potenciar el efecto Sunken (Hundido) */
        "   border-top: 1px solid #080808; "    /* Sombra superior interna */
        "   border-left: 1px solid #080808; "   /* Sombra izquierda interna */
        "   border-bottom: 1px solid #2a2a2a; " /* Brillo inferior (reflejo del borde) */
        "   border-right: 1px solid #2a2a2a; "  /* Brillo derecho */
        "   border-radius: 2px; "
        "   margin-right: 4px; " /* Separación elegante entre los cuadros */
        "}"

        // Consola de Simulación
        "QTextEdit#textConsolaMopac { "
        "background-color: #101010; "
        "color: #e5e5e5; border: 1px solid #2d2d2d; "
        "font-family: 'Consolas', monospace; "
        "font-size: 12px; "
        "padding: 8px; "
        "}";

    this->setStyleSheet(estilo);
}
#include "moleculavista.h"
#include "moleculaview.h"
#include "dialogomopac.h"

#include <QMenu>
#include <QMenuBar>
#include <QGraphicsView>
#include <QTimer>
#include <QDate>
#include <QTime>
#include <QCloseEvent>
#include <QMessageBox>
#include <QCursor>
#include <QFileDialog>
#include <QLineEdit>
#include <QInputDialog>
#include <QBoxLayout>
#include <QPushButton>

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

    // Cable 3.5 Muestra el texto generado por el log hacia el dock de la UI
    connect(m_view, &MoleculaView::notificaContenidoLog, this, [this](const QString& texto) {
        if (m_textoConsola) {
            m_textoConsola->setText(texto);
        }
    });

    // Cable 4 conecta la señal de menuEliminar con el menu de boton derecho
    connect(m_view, &MoleculaView::setMenuEliminar, this, &MoleculaVista::setMenuBotonDerecho);

    // Sincronización forzada inicial directa desde el Emisor real:
    m_view->setModoEditor(MoleculaView::ModoSeleccion);
    //emit m_view->modoLienzoCambiado(m_view->getModoLienzo());
}

// =========================================================================
// ZONA: Funciones Privadas
// =========================================================================

QString MoleculaVista::dialogoAbrirArchivo(){
    // 1. Creamos el filtro global que junta todas tus extensiones permitidas
    QString filtroGlobal = tr("Proyectos VisMol (*.vismol)");

    // 2. Creamos los filtros individuales desglosados
    QString filtroVismol = tr("Todos los archivos soportados (*.vismol *.out *.arc *.aux *.dat *.xyz)");
    QString filtroDat    = tr("Entradas de MOPAC (*.dat)");
    QString filtroOut    = tr("Ficheros de salida MOPAC (*.out)");
    QString filtroAux    = tr("Ficheros de salida MOPAC (*.aux)");
    QString filtroArc    = tr("Archivos de resumen MOPAC (*.arc)");
    QString filtroXyz    = tr("Coordenadas Cartesianas (*.xyz)");
    QString filtroTodos  = tr("Todos los archivos (*)");

    // 3. Concatenamos TODO usando el separador oficial ";;" de Qt
    QString filtrosCombinados = filtroGlobal + ";;"
                                + filtroVismol + ";;"
                                + filtroDat    + ";;"
                                + filtroAux    + ";;"
                                + filtroOut    + ";;"
                                + filtroArc    + ";;"
                                + filtroDat    + ";;"
                                + filtroXyz    + ";;"
                                + filtroTodos;

    return QFileDialog::getOpenFileName(
        this,
        tr("Abrir Proyecto VisMol"),
        "",
        filtrosCombinados
        );
}

QString MoleculaVista::dialogoGuardarArchivo(const QString& filtro){
    QString titulo;
    QString extensionesSoportadas;
    QString extDefecto = filtro.toLower().trimmed();

    // El diálogo se adapta dinámicamente a la intención del flujo
    if (extDefecto == ".dat" || extDefecto == ".mop") {
        titulo = tr("Exportar Entrada de MOPAC");
        extensionesSoportadas = tr("Entradas de MOPAC (*.dat *.mop)");
        extDefecto = "dat"; // Extensión estándar por defecto para tu DatosIO
    }
    else if (extDefecto == ".xyz") {
        titulo = tr("Exportar Coordenadas Cartesianas");
        extensionesSoportadas = tr("Archivos XYZ (*.xyz)");
    }
    else {
        // Opción nativa por defecto ("vismol")
        titulo = tr("Guardar Proyecto VisMol");
        extensionesSoportadas = tr("Proyectos VisMol (*.vismol)");
        extDefecto = "vismol";
    }

    // Lanzamos el QFileDialog nativo inyectando los parámetros limpios
    QString ruta = QFileDialog::getSaveFileName(this, titulo, "", extensionesSoportadas);

    // Red de seguridad: si el usuario escribe el nombre a mano sin la extensión, se la concatenamos
    if (!ruta.isEmpty()) {
        QFileInfo info(ruta);
        if (info.suffix().toLower() != extDefecto) {
            ruta += "." + extDefecto;
        }
    }
    return ruta;
}

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

    // 1. Crear primero las estructuras y paneles (Docks) en memoria
    initDockPanels();    // Genera m_panelFrecuencias
    initPanelConsola();  // Genera m_panelConsola y m_textoConsola

    // 2. Crear las barras y menús que apuntarán a esos paneles
    initMenu();
    initToolBarSuperior();
    initToolBarIzda();
    initAnimacionBar();
    initStatusBar();
    initLienzo();

    // 3. Establecer cables y sincronizaciones seguras
    initConnect();
    setTemaApp();
}

void MoleculaVista::initMenu(){


    QMenuBar* menuBarSuperior = menuBar();

    // =========================================================================
    // 1. PASO CIENTÍFICO: GESTIÓN DE PROYECTO (ARCHIVO)
    // =========================================================================
    QMenu* menuArchivo = menuBarSuperior->addMenu(tr("&Archivo"));

    m_accionesArchivo.nuevo = menuArchivo->addAction(tr("Nuevo Proyecto"), QKeySequence::New, this, &MoleculaVista::nuevoProyecto);
    m_accionesArchivo.nuevo->setIcon(QIcon(":/iconos/nuevo.png"));

    m_accionesArchivo.abrir = menuArchivo->addAction(tr("Abrir Proyecto / Archivo..."), QKeySequence::Open, this, &MoleculaVista::abrirProyecto);
    m_accionesArchivo.abrir->setIcon(QIcon(":/iconos/abrir.png"));

    m_accionesArchivo.guardar = menuArchivo->addAction(tr("Guardar Proyecto"), QKeySequence::Save, this, &MoleculaVista::guardarProyecto);
    m_accionesArchivo.guardar->setIcon(QIcon(":/iconos/guardar.png"));

    m_accionesArchivo.guardarComo = menuArchivo->addAction(tr("Guardar Proyecto en ..."), QKeySequence::SaveAs, this, &MoleculaVista::guardarProyectoComo);
    m_accionesArchivo.guardarComo->setIcon(QIcon(":/iconos/guardar_como.png"));

    m_accionesArchivo.cerrar = menuArchivo->addAction(tr("Cerrar Proyecto"), QKeySequence::Close, this, &MoleculaVista::cerrarProyecto);
    m_accionesArchivo.cerrar->setIcon(QIcon(":/iconos/cerrar.png"));

    menuArchivo->addSeparator();
    m_accionesArchivo.salir = menuArchivo->addAction(tr("Salir"), QKeySequence::Quit, this, &MoleculaVista::salir);
    m_accionesArchivo.salir->setIcon(QIcon(":/iconos/salir.png"));

    // =========================================================================
    // 2. PASO CIENTÍFICO: CONSTRUCCIÓN Y GEOMETRÍA (EDICIÓN)
    // =========================================================================
    QMenu* menuEdicion = menuBarSuperior->addMenu(tr("&Edición"));

    m_accionesEdicion.deshacer = menuEdicion->addAction(tr("Deshacer"), QKeySequence::Undo, this, &MoleculaVista::deshacer);
    m_accionesEdicion.deshacer->setIcon(QIcon(":/iconos/deshacer.png"));

    m_accionesEdicion.rehacer = menuEdicion->addAction(tr("Rehacer"), QKeySequence::Redo, this, &MoleculaVista::rehacer);
    m_accionesEdicion.rehacer->setIcon(QIcon(":/iconos/rehacer.png"));

    menuEdicion->addSeparator();
    m_accionesEdicion.editar = menuEdicion->addAction(tr("Comentario del proyecto..."), this, &MoleculaVista::editar);
    m_accionesEdicion.editar->setIcon(QIcon(":/iconos/editar.png"));

    menuEdicion->addSeparator();
    m_accionesEdicion.limpiarLienzo = menuEdicion->addAction(tr("Limpiar Lienzo"), this, &MoleculaVista::limpiarLienzo);
    m_accionesEdicion.limpiarLienzo->setIcon(QIcon(":/iconos/limpiar.png"));

    // =========================================================================
    // 3. PASO CIENTÍFICO: EJECUCIÓN CUÁNTICA (CÁLCOLO) - ESCALABLE
    // =========================================================================
    QMenu* menuCalculo = menuBarSuperior->addMenu(tr("&Cálculo"));

    // Submenú dedicado a MOPAC para permitir meter otros motores en el futuro
    QMenu* subMenuMopac = menuCalculo->addMenu(tr("Motor MOPAC"));

    m_accionesCalculo.configurarMopac = subMenuMopac->addAction(tr("Configurar y Lanzar Simulación..."), this, &MoleculaVista::configurarMopac);
    m_accionesCalculo.configurarMopac->setIcon(QIcon(":/iconos/configurar.png"));

    // ─── PASO 1: CREACIÓN DE LA ACCIÓN DE PARADA ───
    m_accionesCalculo.detenerMopac = subMenuMopac->addAction(tr("Detener Simulación MOPAC"), this, &MoleculaVista::detenerMopac);
    m_accionesCalculo.detenerMopac->setIcon(QIcon(":/iconos/detener.png")); // Asegúrate de tener este .png en tus recursos

    // =========================================================================
    // 4. PASO CIENTÍFICO: INTERPRETACIÓN DE RESULTADOS (ANÁLISIS)
    // =========================================================================
    QMenu* menuAnalisis = menuBarSuperior->addMenu(tr("&Análisis"));

    m_accionesAnalisis.mostrarFreq = menuAnalisis->addAction(tr("Espectroscopía Vibracional (IR)"), this, &MoleculaVista::mostrarFrecuencias);
    m_accionesAnalisis.mostrarFreq->setIcon(QIcon(":/iconos/frecuencias.png"));
    m_accionesAnalisis.mostrarFreq->setCheckable(true);
    m_accionesAnalisis.mostrarFreq->setChecked(false);

    m_accionesAnalisis.mostrarConsola = menuAnalisis->addAction(tr("Mostrar fichero salida MOPAC"), this, &MoleculaVista::mostrarConsolaLog);
    m_accionesAnalisis.mostrarConsola->setIcon(QIcon(":/iconos/ver_texto.png")); // Añade su icono correspondiente
    m_accionesAnalisis.mostrarConsola->setCheckable(true);
    m_accionesAnalisis.mostrarConsola->setChecked(false);
}

void MoleculaVista::initToolBarSuperior(){
    m_barraProyecto = addToolBar(tr("Proyecto"));
    m_barraProyecto->setObjectName("barraProyecto");

    m_barraProyecto->addAction(m_accionesArchivo.salir);
    m_barraProyecto->addAction(m_accionesArchivo.nuevo);
    m_barraProyecto->addAction(m_accionesArchivo.abrir);
    m_barraProyecto->addAction(m_accionesArchivo.guardar);
    m_barraProyecto->addAction(m_accionesArchivo.guardarComo);
    m_barraProyecto->addAction(m_accionesArchivo.cerrar);
    m_barraProyecto->addSeparator();
    m_barraProyecto->addSeparator();

    m_barraProyecto->addAction(m_accionesEdicion.deshacer);
    m_barraProyecto->addAction(m_accionesEdicion.rehacer);
    m_barraProyecto->addAction(m_accionesEdicion.limpiarLienzo);
    m_barraProyecto->addAction(m_accionesEdicion.editar);
    m_barraProyecto->addSeparator();
    m_barraProyecto->addSeparator();

    m_barraProyecto->addAction(m_accionesAnalisis.mostrarFreq);
    m_barraProyecto->addAction(m_accionesAnalisis.mostrarConsola);
    m_barraProyecto->addSeparator();
    m_barraProyecto->addSeparator();

    m_barraProyecto->addAction(m_accionesCalculo.configurarMopac);
    m_barraProyecto->addAction(m_accionesCalculo.detenerMopac);
}

void MoleculaVista::initToolBarIzda(){

    // =========================================================================
    // BARRA LATERAL: HERRAMIENTAS Y ELEMENTOS QUÍMICOS (MINIMALISTA)
    // =========================================================================
    m_barraElementos = new QToolBar(tr("Herramientas de Elementos"));
    m_barraElementos->setObjectName("barraElementos");
    m_barraElementos->setOrientation(Qt::Vertical);
    m_barraElementos->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addToolBar(Qt::LeftToolBarArea, m_barraElementos);

    // --- SUBSECCIÓN A: HERRAMIENTAS DE INTERACCIÓN ---
    m_accionesElementos.modoSeleccion = new QAction(tr("Seleccionar"), this);
    m_accionesElementos.modoSeleccion->setCheckable(true);
    m_accionesElementos.modoSeleccion->setIcon(QIcon(":/iconos/puntero.png"));
    m_accionesElementos.modoSeleccion->setToolTip(tr("Herramienta de Selección / Puntero"));

    m_accionesElementos.modoEnlace = new QAction(tr("Enlace"), this);
    m_accionesElementos.modoEnlace->setCheckable(true);
    m_accionesElementos.modoEnlace->setIcon(QIcon(":/iconos/enlace.png"));
    m_accionesElementos.modoEnlace->setToolTip(tr("Crear un nuevo enlace entre átomos"));

    m_accionesElementos.modoRotar = new QAction(tr("Rotar"), this);
    m_accionesElementos.modoRotar->setCheckable(true);
    m_accionesElementos.modoRotar->setIcon(QIcon(":/iconos/rotar3d.png"));
    m_accionesElementos.modoRotar->setToolTip(tr("Rotar la molécula en el espacio 3D"));

    m_barraElementos->addAction(m_accionesElementos.modoSeleccion);
    m_barraElementos->addAction(m_accionesElementos.modoEnlace);
    m_barraElementos->addAction(m_accionesElementos.modoRotar);

    // Separador visual elegante que divide las herramientas de los elementos
    m_barraElementos->addSeparator();
    m_barraElementos->addSeparator();
    m_barraElementos->addSeparator();


    // --- SUBSECCIÓN B: ELEMENTOS QUÍMICOS VIGENTES ---
    m_accionesElementos.carbono = new QAction(tr("Carbono"));
    m_accionesElementos.carbono->setCheckable(true);
    m_accionesElementos.carbono->setIcon(QIcon(":/iconos/atomo_c.png"));

    m_accionesElementos.hidrogeno = new QAction(tr("Hidrógeno"));
    m_accionesElementos.hidrogeno->setCheckable(true);
    m_accionesElementos.hidrogeno->setIcon(QIcon(":/iconos/atomo_h.png"));

    m_accionesElementos.oxigeno = new QAction(tr("Oxígeno"));
    m_accionesElementos.oxigeno->setCheckable(true);
    m_accionesElementos.oxigeno->setIcon(QIcon(":/iconos/atomo_o.png"));

    m_accionesElementos.nitrogeno = new QAction(tr("Nitrógeno"));
    m_accionesElementos.nitrogeno->setCheckable(true);
    m_accionesElementos.nitrogeno->setIcon(QIcon(":/iconos/atomo_n.png"));

    m_barraElementos->addAction(m_accionesElementos.carbono);
    m_barraElementos->addAction(m_accionesElementos.hidrogeno);
    m_barraElementos->addAction(m_accionesElementos.oxigeno);
    m_barraElementos->addAction(m_accionesElementos.nitrogeno);

    m_barraElementos->addSeparator();
    m_barraElementos->addSeparator();


    m_accionesElementos.tablaPeriodica = new QAction(tr("Más..."), this);
    m_accionesElementos.tablaPeriodica->setIcon(QIcon(":/iconos/tabla_periodica.png"));
    m_accionesElementos.tablaPeriodica->setToolTip(tr("Abrir la Tabla Periódica completa"));

    m_barraElementos->addAction(m_accionesElementos.tablaPeriodica);

    // =========================================================================
    // EXCLUSIVIDAD: CONFIGURACIÓN DEL GRUPO DE ACCIONES
    // =========================================================================
    m_grupoElementos = new QActionGroup(this);
    m_grupoElementos->addAction(m_accionesElementos.carbono);
    m_grupoElementos->addAction(m_accionesElementos.hidrogeno);
    m_grupoElementos->addAction(m_accionesElementos.oxigeno);
    m_grupoElementos->addAction(m_accionesElementos.nitrogeno);
    m_grupoElementos->addAction(m_accionesElementos.modoSeleccion);
    m_grupoElementos->addAction(m_accionesElementos.modoEnlace);
    m_grupoElementos->addAction(m_accionesElementos.modoRotar);
    m_grupoElementos->setExclusive(true);
}

void MoleculaVista::initPanelConsola(){
    m_panelConsola = new QDockWidget(tr("Monitor de Simulación en Vivo (MOPAC)"), this);
    m_panelConsola->setObjectName("panelConsolaMopac");
    m_panelConsola->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    m_textoConsola = new QTextEdit();
    m_textoConsola->setObjectName("textConsolaMopac");
    m_textoConsola->setReadOnly(true);
    m_textoConsola->append("--- VISMOL: ESPERANDO EJECUCIÓN DEL MOTOR MOPAC ---");
    m_textoConsola->append(" > Use 'Configurar Simulación MOPAC...' para iniciar.");

    m_panelConsola->setWidget(m_textoConsola);
    addDockWidget(Qt::RightDockWidgetArea, m_panelConsola);

    // Ocultar de fábrica para evitar que el render del S.O. lo fuerce al iniciar
    m_panelConsola->hide();
}

void MoleculaVista::initAnimacionBar(){
    m_barraAnimacion = new QToolBar(tr("Control de Trayectoria / Animación"), this);
    m_barraAnimacion->setObjectName("barraAnimacion");
    addToolBar(Qt::BottomToolBarArea, m_barraAnimacion);

    // =========================================================================
    // MODIFICACIÓN DE CENTRADO COMPLETO (WIDGETS ESPACIADORES ELÁSTICOS)
    // =========================================================================
    // 1. Creamos el espaciador izquierdo
    QWidget* espaciadorIzquierdo = new QWidget();
    espaciadorIzquierdo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_barraAnimacion->addWidget(espaciadorIzquierdo);

    // 2. Instanciamos las acciones de reproducción normales
    m_accionesAnimacion.reproducirPausar = new QAction(tr("Reproducir"), this);
    m_accionesAnimacion.reproducirPausar->setCheckable(true);

    m_accionesAnimacion.fotogramaAnterior = new QAction(tr("<< Paso Anter."), this);
    m_accionesAnimacion.fotogramaSiguiente = new QAction(tr("Paso Siguiente >>"), this);

    m_deslizadorFotogramas = new QSlider(Qt::Horizontal);
    m_deslizadorFotogramas->setMinimum(1);
    m_deslizadorFotogramas->setMaximum(10);
    m_deslizadorFotogramas->setFixedWidth(250);

    m_lblFotogramaInfo = new QLabel(tr("Paso: 1 / 10"));
    m_lblFotogramaInfo->setStyleSheet("padding: 0 8px; font-weight: bold;");

    // 3. Añadimos el bloque compacto de controles al lienzo de la barra
    m_barraAnimacion->addAction(m_accionesAnimacion.fotogramaAnterior);
    m_barraAnimacion->addAction(m_accionesAnimacion.reproducirPausar);
    m_barraAnimacion->addAction(m_accionesAnimacion.fotogramaSiguiente);
    m_barraAnimacion->addWidget(m_deslizadorFotogramas);
    m_barraAnimacion->addWidget(m_lblFotogramaInfo);

    // 4. Creamos el espaciador derecho para cerrar el sándwich de centrado
    QWidget* espaciadorDerecho = new QWidget();
    espaciadorDerecho->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_barraAnimacion->addWidget(espaciadorDerecho);

    m_timerAnimacion = new QTimer(this);
}

void MoleculaVista::initDockPanels(){
    m_panelFrecuencias = new QDockWidget(tr("Analizador de Modos Vibracionales (IR)"), this);
    m_panelFrecuencias->setObjectName("panelFrecuencias");
    m_panelFrecuencias->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    m_listaFrecuencias = new QListWidget();
    m_listaFrecuencias->setObjectName("listaFrecuencias");
    m_listaFrecuencias->addItem("3120.4 cm⁻¹ - Tensión C-H (Simétrica)");
    m_listaFrecuencias->addItem("1650.1 cm⁻¹ - Estiramiento C=C");
    m_listaFrecuencias->addItem("1430.5 cm⁻¹ - Flexión en el plano");

    m_panelFrecuencias->setWidget(m_listaFrecuencias);
    addDockWidget(Qt::RightDockWidgetArea, m_panelFrecuencias);

    // Ocultar de fábrica
    m_panelFrecuencias->hide();
}

void MoleculaVista::initStatusBar(){
    m_barraEstado = new QStatusBar(this);
    setStatusBar(m_barraEstado);

    m_lblEstadoTexto = new QLabel(tr(" Listo para iniciar proyecto."));
    m_lblEstadoFecha = new QLabel();
    m_lblEstadoHora  = new QLabel();

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


    // ─── MAQUINARIA DE ANIMACIÓN: CONEXIONES CENTRALIZADAS CON TUS SLOTS REALES ───
    connect(m_accionesAnimacion.reproducirPausar, &QAction::toggled, this, &MoleculaVista::alPlayAnimacion);
    connect(m_deslizadorFotogramas, &QSlider::valueChanged, this, &MoleculaVista::alCambiarDeslizador);
    connect(m_timerAnimacion, &QTimer::timeout, this, &MoleculaVista::adelantePlay);

    // Requisito 3 y 4: Paso manual Atrás (Interrumpe Play + Bucle Extremo)
    connect(m_accionesAnimacion.fotogramaAnterior, &QAction::triggered, this, [this]() {
        if (m_timerAnimacion->isActive()) {
            m_accionesAnimacion.reproducirPausar->setChecked(false);
        }
        if (m_view) {
            int total = m_view->getTamTrayectoria();
            if (total <= 1) return;
            m_deslizadorFotogramas->setValue(m_fotogramaActual > 0 ? m_fotogramaActual : total);
        }
    });

    // Requisito 3 y 4: Paso manual Adelante (Interrumpe Play + Bucle Extremo)
    connect(m_accionesAnimacion.fotogramaSiguiente, &QAction::triggered, this, [this]() {
        if (m_timerAnimacion->isActive()) {
            m_accionesAnimacion.reproducirPausar->setChecked(false);
        }
        if (m_view) {
            int total = m_view->getTamTrayectoria();
            if (total <= 1) return;
            m_deslizadorFotogramas->setValue(m_fotogramaActual < (total - 1) ? m_fotogramaActual + 2 : 1);
        }
    });

    connect(m_listaFrecuencias, &QListWidget::currentRowChanged, this, &MoleculaVista::alSeleccionarFrecuencia);

    connect(m_accionesAnalisis.mostrarConsola, &QAction::toggled, m_panelConsola, &QDockWidget::setVisible);
    connect(m_accionesAnalisis.mostrarFreq, &QAction::toggled, m_panelFrecuencias, &QDockWidget::setVisible);

    connect(m_panelConsola, &QDockWidget::visibilityChanged, m_accionesAnalisis.mostrarConsola, &QAction::setChecked);
    connect(m_panelFrecuencias, &QDockWidget::visibilityChanged, m_accionesAnalisis.mostrarFreq, &QAction::setChecked);

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

void MoleculaVista::abrirProyecto(){
    if (!m_view) return;

    // 1. Antes de abrir, verificamos si hay cambios sin guardar en el lienzo actual
    if (!sePuedeCerrar()) {
        return; // El usuario canceló la operación en el cuadro de diálogo
    }

    // 2. Desplegamos el explorador de archivos nativo
    QString ruta = dialogoAbrirArchivo();

    if (!ruta.isEmpty()) {
        // 3. ENVIAR DIRECTAMENTE LA RUTA AL CEREBRO Y RECIBIR EL RESULTADO
        // La vista no sabe (ni le importa) si el archivo es .vismol, .out, .dat o .xyz
        QString error = m_view->abrirArchivo(ruta);

        // 4. GESTIÓN EXCLUSIVA DE ALERTAS GRÁFICAS SEGÚN LA RESPUESTA
        if (error == "AVISO_TOPOLOGIA") {
            // Escenario controlado de advertencia de tu lógica de MOPAC
            QMessageBox::information(
                this,
                tr("Información de Topología"),
                tr("La geometría cuántica se cargó con éxito, pero no se localizó el archivo espejo "
                   "de enlaces (.vismol) en el directorio.\n\nLos enlaces deberán asignarse manualmente."),
                QMessageBox::Ok
                );
        }
        else if (!error.isEmpty()) {
            // Escenario de error crítico real (archivo corrupto, no soportado, etc.)
            QMessageBox::critical(
                this,
                tr("Error al abrir archivo"),
                QString("Se produjo un error al intentar leer el archivo molecular:\n%1").arg(error),
                QMessageBox::Ok
                );
        }
    }
}

void MoleculaVista::guardarProyecto(){
    if (!m_view) return;

    // 1. Preguntamos al cerebro si ya tiene una ruta en memoria
    QString ruta    = m_view->getRutaArchivoActual();
    QString filtro  = ".vismol";

    // 2. Si NO hay ruta (proyecto nuevo), llamamos a dialogoGuardar para obtenerla
    if (ruta.isEmpty()) {
        ruta = dialogoGuardarArchivo(filtro);
    }

    // 3. Si tenemos una ruta válida (porque ya existía o porque el usuario la acaba de elegir)
    // se la enviamos a m_view para que él se arregle con DatosIO
    if (!ruta.isEmpty()) {
        QString error = m_view->guardarArchivo(ruta);

        // Gestión de la alerta gráfica en la interfaz
        if (!error.isEmpty()) {
            QMessageBox::critical(
                this,
                tr("Error al guardar"),
                QString("Se produjo un error al intentar escribir el archivo:\n%1").arg(error),
                QMessageBox::Ok
                );
        } else {
            m_lblEstadoTexto->setText(tr(" Proyecto guardado correctamente en: %1").arg(QFileInfo(ruta).fileName()));
        }
    }
}

void MoleculaVista::guardarProyectoComo(){
    if (!m_view) return;

    QString filtro = ".vismol";

    // "Guardar como" se salta la pregunta de la memoria y va directo a pedir una nueva ruta
    QString nuevaRuta = dialogoGuardarArchivo(filtro);

    if (!nuevaRuta.isEmpty()) {
        QString error = m_view->guardarArchivo(nuevaRuta);

        if (!error.isEmpty()) {
            QMessageBox::critical(
                this,
                tr("Error al guardar como"),
                QString("Se produjo un error al intentar clonar el archivo:\n%1").arg(error),
                QMessageBox::Ok
                );
        } else {
            m_lblEstadoTexto->setText(tr(" Nuevo archivo de proyecto creado en: %1").arg(QFileInfo(nuevaRuta).fileName()));
        }
    }
}

void MoleculaVista::cerrarProyecto(){
    if (sePuedeCerrar()){
        m_view->cerrarProyecto(); // El cerebro hace el trabajo limpio
    }
}

void MoleculaVista::editar(){
    if (!m_view) return;

    // 1. Instanciamos el cuadro de diálogo por código
    QInputDialog dialogo(this);

    // 2. Configuramos los textos y propiedades básicas
    dialogo.setWindowTitle(tr("Comentario del Proyecto"));
    dialogo.setLabelText(tr("Comentario / Título de la estructura molecular:"));
    dialogo.setTextValue(m_view->getComentario());
    dialogo.setTextEchoMode(QLineEdit::Normal);

    // 3. Forzamos la geometría y el diseño mediante QSS especializado
    dialogo.setStyleSheet(
        "QLineEdit { "
        "   min-width: 600px; "
        "} "
        "QLabel { "
        "   min-height: 40px; "
        "   max-height: 40px; "
        "   font-size: 14px; "
        "}"
        );

    // 4. Bloqueamos la interfaz y esperamos la respuesta del usuario (Modal)
    int resultado = dialogo.exec();

    // Si el usuario acepta la ventana, actualizamos el cerebro
    if (resultado == QDialog::Accepted) {
        QString nuevoComentario = dialogo.textValue();
        m_view->setComentario(nuevoComentario);

        // Sincronización indirecta para forzar el redibujado del lienzo
        m_view->setModoEditor(m_view->getModoEditor());
    }
}

void MoleculaVista::salir(){
    this->close();
}

void MoleculaVista::deshacer(){
    if (m_view) {
        m_view->deshacer(); // El cerebro viaja al pasado y emite las señales de refresco
    }
}

void MoleculaVista::rehacer(){
    if (m_view) {
        m_view->rehacer(); // El cerebro avanza al futuro alternativo
    }
}

void MoleculaVista::limpiarLienzo(){
    m_view->limpiarLienzo();
}

void MoleculaVista::configurarMopac(){
    if (!m_view) return;

    // 1. Instanciamos la ventana por código pasándole "this" (esta misma vista) como jefa y padre visual
    QString comentario;
    QString argumentos;

    DialogoMopac dialogo(argumentos, comentario, this);

    // 2. Bloqueamos la interfaz principal y mostramos el diálogo en modo modal
    m_lblEstadoTexto->setText(tr(" 📋 Configurando parámetros de optimización cuántica en el diálogo..."));
    // exec() se queda esperando pacientemente en silencio a que el usuario pulse "Exportar", "Lanzar" o "Cancelar"
    int resultado = dialogo.exec();

    // ─── ¡NUEVO!: CAPTURAMOS LA CANCELACIÓN O EL BOTÓN CERRAR ───
    if (resultado == QDialog::Rejected) {
        // Le ordenamos al cerebro abortar cualquier ejecución en vivo activa de MOPAC
        m_view->detenerSimularMopac();
        return;
    }

    // ─── ESCENARIO 1: EL USUARIO QUIERE GUARDAR ARCHIVO:DAT  ───
    if(resultado == 1 ){
        // exportar
        QString ruta = dialogoGuardarArchivo(".dat");

        if(m_view){
            m_view->setComentario(comentario);
            m_view->setArgumentos(argumentos);
            m_view->guardarArchivo(ruta);
        }

    }
    // ─── ESCENARIO 2: EL USUARIO QUIERE LANZAR LA SIMULACIÓN EN VIVO (SIN PEDIR RUTA) ───
    else if(resultado == 2){
        if(m_view){
            // 1. Volcamos los datos recogidos del diálogo directamente al cerebro
            m_view->setComentario(comentario);
            m_view->setArgumentos(argumentos);

            // 3. Ordenamos al cerebro arrancar la simulación de forma directa
            m_view->simularMopac();
        }
    }
}

void MoleculaVista::detenerMopac(){
    // La vista no mata procesos directamente, le delega la orden de negocio al cerebro
    if (m_view) {
        m_view->detenerSimularMopac();
    }
}

void MoleculaVista::mostrarFrecuencias(){}

void MoleculaVista::mostrarConsolaLog(){}

// =========================================================================
// ZONA: INTERACCIONES Y BOTONERÍA DE INTERFAZ
// =========================================================================

void MoleculaVista::alCambiarModoLienzo(const MoleculaView::ModoLienzo& modoLienzo){

    // =========================================================================
    // CAPA 1: ESTADOS BINARIOS DIRECTOS (HERRAMIENTAS DE EDICIÓN Y ARCHIVO)
    // =========================================================================

    // Activar barras de dibujo solo si el lienzo se encuentra inicializado en memoria
    m_barraElementos->setEnabled(modoLienzo.estaIniciado);

    // Acciones de persistencia y topología básicas (Exigen que no esté vacío)
    m_accionesArchivo.cerrar->setEnabled(!modoLienzo.estaVacio);
    m_accionesEdicion.limpiarLienzo->setEnabled(!modoLienzo.estaVacio);
    m_accionesEdicion.editar->setEnabled(modoLienzo.estaIniciado && modoLienzo.formato != TipoFormato::OUT && modoLienzo.formato != TipoFormato::ARC);
    m_accionesArchivo.guardar->setEnabled(!modoLienzo.estaVacio);
    m_accionesArchivo.guardarComo->setEnabled(!modoLienzo.estaVacio);
    m_accionesElementos.modoEnlace->setEnabled(!modoLienzo.estaVacio);
    m_accionesElementos.modoRotar->setEnabled(!modoLienzo.estaVacio);

    // =========================================================================
    // CAPA 2: REGLAS DE NEGOCIO CIENTÍFICO (CONFIGURACIÓN DE SIMULACIÓN MOPAC)
    // =========================================================================

    // El motor de cálculo exige que haya átomos en pantalla pero bloquea nuevas simulaciónes
    // si el archivo abierto ya corresponde a un resultado final cuántico compilado (.out)
    m_accionesCalculo.configurarMopac->setEnabled(!modoLienzo.estaVacio
                                                  && modoLienzo.formato != TipoFormato::OUT
                                                  && modoLienzo.formato != TipoFormato::ARC
                                                  && modoLienzo.formato != TipoFormato::AUX);
    m_accionesCalculo.detenerMopac->setEnabled(modoLienzo.estaMopacActivo);

    // =========================================================================
    // CAPA 3: HISTORIAL DINÁMICO (PILAS DE COMANDOS DESHACER / REHACER)
    // =========================================================================

    // El árbol del historial requiere una red de seguridad que verifique la existencia del cerebro
    bool hayView = (m_view != nullptr);
    m_accionesEdicion.deshacer->setEnabled(hayView && m_view->hayPasado());
    m_accionesEdicion.rehacer->setEnabled(hayView && m_view->hayFuturo());

    // =========================================================================
    // CAPA 4: ENTORNO DE ANIMACIÓN (ARRANQUE DIRECTO EN EL ÚLTIMO PASO)
    // =========================================================================
    int tamTrayectoriaReal = m_view ? m_view->getTamTrayectoria() : 0;

    if (tamTrayectoriaReal > 0 && m_barraAnimacion) {
        // Forzamos la parada de seguridad de la reproducción
        if (m_timerAnimacion->isActive()) {
            m_accionesAnimacion.reproducirPausar->blockSignals(true);
            m_accionesAnimacion.reproducirPausar->setChecked(false);
            m_accionesAnimacion.reproducirPausar->blockSignals(false);
            m_timerAnimacion->stop();
        }

        m_deslizadorFotogramas->blockSignals(true);
        m_deslizadorFotogramas->setMinimum(1);
        m_deslizadorFotogramas->setMaximum(tamTrayectoriaReal);

        // ─── REQUISITO 1: LA ANIMACIÓN EMPIEZA EN EL ÚLTIMO FOTOGRAMA FIJA ───
        m_deslizadorFotogramas->setValue(tamTrayectoriaReal);
        m_fotogramaActual = tamTrayectoriaReal - 1;

        m_deslizadorFotogramas->blockSignals(false);

        m_lblFotogramaInfo->setText(tr("Paso: %1 / %2").arg(tamTrayectoriaReal).arg(tamTrayectoriaReal));
        m_barraAnimacion->setVisible(true);
    }
    else if (m_barraAnimacion) {
        m_barraAnimacion->setVisible(false);
    }

    // =========================================================================
    // CAPA 5: COMPORTAMIENTO SEMÁNTICO (TÍTULO DINÁMICO DE LA APLICACIÓN)
    // =========================================================================
    if (modoLienzo.estaVacio) {
        setWindowTitle(NOMBRE_APP);
    } else {
        // Extraemos de manera segura el nombre plano del archivo molecular cargado actualmente
        QString ruta   = m_view ? m_view->getRutaArchivoActual() : "";
        QString nombre = ruta.isEmpty() ? tr("Sin Título") : QFileInfo(ruta).fileName();

        // El título concatena un asterisco (*) de alerta si detecta modificaciones pendientes de guardado
        setWindowTitle(modoLienzo.estaGuardado ? NOMBRE_APP + tr(" - %1").arg(nombre)
                                               : NOMBRE_APP + tr(" - %1* (Cambios sin guardar)").arg(nombre));
    }

    // =========================================================================
    // CAPA 6: MÁQUINA DE ESTADOS COMPORTAMENTAL PARA PANELES DE ANÁLISIS
    // =========================================================================

    // Evaluamos los criterios de compatibilidad química específicos para cada tipo de panel
    bool admiteConsola     = (!modoLienzo.estaVacio && (modoLienzo.formato == TipoFormato::OUT || modoLienzo.formato == TipoFormato::ARC || modoLienzo.formato == TipoFormato::AUX || modoLienzo.formato == TipoFormato::FORCE_OUT));
    bool admiteFrecuencias = (!modoLienzo.estaVacio && modoLienzo.formato == TipoFormato::FORCE_OUT);

    // --- SUBSECCIÓN A: GESTIÓN SEMÁNTICA DE LA CONSOLA DE TEXTO (LOG) ---
    m_accionesAnalisis.mostrarConsola->setEnabled(admiteConsola);

    if (!admiteConsola) {
        // Bloqueamos las señales temporalmente para apagar el botón sin disparar bucles infinitos de eventos
        m_accionesAnalisis.mostrarConsola->blockSignals(true);
        m_accionesAnalisis.mostrarConsola->setChecked(false);
        m_accionesAnalisis.mostrarConsola->blockSignals(false);

        if (m_panelConsola) m_panelConsola->setVisible(false);
    } else {
        // ─── [SINCRO]: Encendemos el interruptor del botón y Qt abrirá el dock a través de tu initConnect
        m_accionesAnalisis.mostrarConsola->blockSignals(true);
        m_accionesAnalisis.mostrarConsola->setChecked(true);
        m_accionesAnalisis.mostrarConsola->blockSignals(false);

        if (m_panelConsola) {
            m_panelConsola->setVisible(true);
            m_panelConsola->raise(); // Trae al frente sobre el lienzo
        }
    }

    // --- SUBSECCIÓN B: GESTIÓN SEMÁNTICA DEL PANEL DE FRECUENCIAS (IR) ---
    m_accionesAnalisis.mostrarFreq->setEnabled(admiteFrecuencias);

    if (!admiteFrecuencias) {
        m_accionesAnalisis.mostrarFreq->blockSignals(true);
        m_accionesAnalisis.mostrarFreq->setChecked(false);
        m_accionesAnalisis.mostrarFreq->blockSignals(false);

        if (m_panelFrecuencias) m_panelFrecuencias->setVisible(false);
        if (m_listaFrecuencias) m_listaFrecuencias->clear();
    } else {
        if (m_view && m_listaFrecuencias) {
            m_listaFrecuencias->clear(); // Borrado preventivo del texto de prueba de fábrica

            // Recuperamos el espectro de cm-1 reales mapeado por tu parser cuántico
            QVector<Frecuencias> espectroReal = m_view->getListaFrecuencias();

            if (!espectroReal.isEmpty()) {
                // Poblamos el widget con los datos reales indexados
                for (const auto& nodo : espectroReal) {
                    QString itemTexto = QString("%1 cm⁻¹ - Modo Normal Vibracional (IR)").arg(nodo.frecuencia, 0, 'f', 1);
                    m_listaFrecuencias->addItem(itemTexto);
                }

                // ─── [SINCRO PERFECTA]: Encendemos de forma oficial el interruptor lógico del Action ───
                m_accionesAnalisis.mostrarFreq->blockSignals(true);
                m_accionesAnalisis.mostrarFreq->setChecked(true);
                m_accionesAnalisis.mostrarFreq->blockSignals(false);

                // Forzamos al Dock a sincronizarse con el estado del Action levantándose en la pantalla
                if (m_panelFrecuencias) {
                    m_panelFrecuencias->setVisible(true);
                    m_panelFrecuencias->raise(); // Empuja la ventana al frente visual
                }
            } else {
                m_accionesAnalisis.mostrarFreq->blockSignals(true);
                m_accionesAnalisis.mostrarFreq->setChecked(false);
                m_accionesAnalisis.mostrarFreq->blockSignals(false);
                if (m_panelFrecuencias) m_panelFrecuencias->setVisible(false);
            }
        }
    }
}

void MoleculaVista::alCambiarModoEditor(const MoleculaView::ModoEditor& modoEditor){
    if (!m_view || !m_lienzo) return; // Protección de punteros

    // =========================================================================
    // LA VISTA ORDENA AL LIENZO RESETEARSE ANTES DE SINCRONIZAR LA BOTONERÍA
    // =========================================================================
    if (modoEditor != MoleculaView::ModoRotacion3D) {
        m_lienzo->resetearVistaFrontal(); // <─── AQUÍ: MoleculaVista ejecuta la orden
    }

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
    if (!m_view) return;

    // Actualizamos el texto informativo de la barra de reproducción
    m_lblFotogramaInfo->setText(tr("Paso: %1 / %2").arg(fotograma).arg(m_deslizadorFotogramas->maximum()));

    // ─── ¡NUEVO!: ACTUALIZAR EL LIENZO CON EL PASO DE LA ANIMACIÓN ───
    // Añade el método público 'setFotogramaTrayectoria' en el Cerebro para transferir las coordenadas
    m_view->setFotogramaTrayectoria(fotograma - 1);
}

void MoleculaVista::alSeleccionarFrecuencia(int indice){
    if (indice < 0 || !m_view || !m_listaFrecuencias) return;

    // 1. Extraemos el texto del renglón seleccionado
    QString textoModo = m_listaFrecuencias->item(indice)->text();

    // 2. Imprimimos feedback contextual inmediato en tu barra de estado Sunken
    m_lblEstadoTexto->setText(tr(" Analizando espectro: %1").arg(textoModo));

    // 3. Informamos al cerebro el índice base cero activo para el cálculo de desplazamientos
    m_view->setFrecuenciaActiva(indice);
}

void MoleculaVista::alPlayAnimacion(bool activado){
    m_accionesAnimacion.reproducirPausar->setText(activado ? tr("Pausar") : tr("Reproducir"));

    if (activado) {
        int totalFrames = m_view ? m_view->getTamTrayectoria() : 0;
        if (totalFrames <= 1) {
            // Protección: Si no hay trayectoria válida o solo tiene 1 paso, abortamos el Play
            m_accionesAnimacion.reproducirPausar->blockSignals(true);
            m_accionesAnimacion.reproducirPausar->setChecked(false);
            m_accionesAnimacion.reproducirPausar->blockSignals(false);
            return;
        }

        // ─── REQUISITO 2: SE REPRODUCE EMPEZANDO POR FOTOGRAMA = 0 ───
        m_fotogramaActual = 0;
        m_deslizadorFotogramas->setValue(1); // El slider moverá visualmente el lienzo al inicio

        m_timerAnimacion->start(100);        // Inicia el latido del motor (10 FPS)
    } else {
        m_timerAnimacion->stop();            // Congela el motor inmediatamente en el frame actual
    }
}

void MoleculaVista::adelantePlay() {
    if (!m_view) return;
    int totalFotogramas = m_view->getTamTrayectoria();

    // ─── REQUISITO 2: SE REPRODUCE INDEFINIDAMENTE (BUCLE AUTOMÁTICO CONTINUO) ───
    if (m_fotogramaActual < (totalFotogramas - 1)) {
        m_deslizadorFotogramas->setValue(m_fotogramaActual + 2); // Avanza un paso normal (base 1)
    } else {
        m_deslizadorFotogramas->setValue(1);                     // Llegó al final: vuelve al fotograma 0 (1 en slider)
    }
}

void MoleculaVista::atrasPlay(){
    if (!m_view) return;
    int totalFotogramas = m_view->getTamTrayectoria();

    // Retroceso automático circular (Por si inviertes la reproducción)
    if (m_fotogramaActual > 0) {
        m_deslizadorFotogramas->setValue(m_fotogramaActual);    // Retrocede un paso normal
    } else {
        m_deslizadorFotogramas->setValue(totalFotogramas);      // Llegó al principio: salta al final de la cola
    }
}

void MoleculaVista::alCambiarDeslizador(int fotograma){
    if (!m_view) return;

    m_fotogramaActual = fotograma - 1; // Sincronización del índice base cero
    m_lblFotogramaInfo->setText(tr("Paso: %1 / %2").arg(fotograma).arg(m_deslizadorFotogramas->maximum()));

    m_view->setFotogramaTrayectoria(m_fotogramaActual); // Mutación y repintado geométrico
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

        // =========================================================================
        // BARRA LATERAL IZQUIERDA: HOMOGÉNEA Y SIMÉTRICA CON LA SUPERIOR
        // =========================================================================
        "QToolBar#barraElementos { "
        "   background-color: #222222; "        /* Mismo color exacto que la superior */
        "   border-right: 1px solid #2d2d2d; "
        "   border-top: none; "
        "   border-left: none; "
        "   border-bottom: none; "
        "   spacing: 6px; "
        "   padding: 6px; "                    /* Padding cuadrado limpio */
        "}"

        /* Hereda todo el comportamiento general. Solo aseguramos que se comporte como IconOnly */
        "QToolBar#barraElementos QToolButton { "
        "   qproperty-toolButtonStyle: ToolButtonIconOnly; "
        "   icon-size: 20px 20px; "            /* Mismo tamaño que arriba */
        "}"

        "QToolBar#barraElementos QToolButton:hover { "
        "   background-color: #383838; "
        "   border: 1px solid #4a4a4a; "
        "   color: #ffffff; "
        "}"

        "QToolBar#barraElementos QToolButton:checked { "
        "   background-color: #0f52ba; "
        "   border: 1px solid #3b82f6; "
        "   color: #ffffff; "
        "   font-weight: bold; "
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
        "font-size: 14px; "
        "}"

        "QDockWidget::title { "
        "background-color: #1c1c1c; "
        "text-align: left; "
        "padding: 8px 12px; "
        "border-bottom: 1px solid #2d2d2d; "
        "}"

        // Listas (Frecuencias) - Un verde esmeralda más tecnológico y menos "Matrix"
        "QListWidget#listaFrecuencias { "
        "background-color: #202225; "
        "color: #39ff14; "
        "border: 1px solid #2d2d2d; "
        "border-radius: 4px; "
        "font-family: 'Consolas', 'Courier New', monospace; "
        "font-size: 14px; "
        "padding: 6px; "
        "min-width: 700px; " /* ¡NUEVO! Fuerza a que el panel sea el triple de ancho */
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
        "background-color: #202225; "
        "color: #e5e5e5; border: 1px solid #2d2d2d; "
        "font-family: 'Consolas', monospace; "
        "font-size: 14px; "
        "padding: 8px; "
        "min-width: 700px; " /* ¡NUEVO! Fuerza a que el panel sea el triple de ancho */
        "}";


    this->setStyleSheet(estilo);
}
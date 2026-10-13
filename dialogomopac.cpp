#include "dialogomopac.h"
#include "moleculavista.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

DialogoMopac::DialogoMopac(MoleculaVista* vistaPrincipal, QWidget *parent)
    : QDialog(parent), m_vista(vistaPrincipal)
{
    // Eliminamos cualquier referencia a ui->setupUi(this) ya que diseñamos a mano
    initUi();
}

DialogoMopac::~DialogoMopac() {
    // Ya no hay puntero ui que borrar aquí, Qt gestiona la memoria de los widgets hijos automáticamente
}

void DialogoMopac::initUi() {
    setWindowTitle(tr("Configuración de Optimización y Modelado"));
    setMinimumWidth(420);

    QVBoxLayout* layoutPrincipal = new QVBoxLayout(this);

    // =========================================================================
    // 1. BLOQUE SUPERIOR: PARÁMETROS CUÁNTICOS (KEYWORDS)
    // =========================================================================
    QGroupBox* grupoKeywords = new QGroupBox(tr("Estrategia del Cálculo Cuántico MOPAC"), this);
    QFormLayout* layoutForm = new QFormLayout(grupoKeywords);

    m_comboMetodo = new QComboBox(this);
    m_comboMetodo->addItems({"AM1", "PM3", "MNDO"});

    m_comboTarea = new QComboBox(this);
    m_comboTarea->addItems({tr("Optimización Geométrica"), tr("Modos Vibracionales (FORCE)")});

    m_checkPrecision = new QCheckBox(tr("Convergencia Estricta (PRECISE)"), this);
    m_checkPrecision->setChecked(true);

    layoutForm->addRow(tr("Método Hamiltoniano:"), m_comboMetodo);
    layoutForm->addRow(tr("Tipo de Tarea:"), m_comboTarea);
    layoutForm->addRow("", m_checkPrecision);

    // =========================================================================
    // 2. BLOQUE CENTRAL: COMENTARIO ESTRUCTURAL
    // =========================================================================
    QGroupBox* grupoDatos = new QGroupBox(tr("Información y Comentario del Compuesto"), this);
    QVBoxLayout* layoutDatos = new QVBoxLayout(grupoDatos);

    m_txtComentario = new QLineEdit(this);
    m_txtComentario->setPlaceholderText(tr("Escriba un comentario o título para las cabeceras..."));
    layoutDatos->addWidget(m_txtComentario);

    // =========================================================================
    // 3. BLOQUE INFERIOR: BOTONERA DE ACCIÓN SEMÁNTICA (TUS INTENCIONES)
    // =========================================================================
    QHBoxLayout* layoutBotones = new QHBoxLayout();
    m_btnExportar = new QPushButton(tr("📦 Exportar Entrada"), this);
    m_btnLanzar   = new QPushButton(tr("🚀 Lanzar Simulación"), this);
    m_btnCancelar = new QPushButton(tr("❌ Cancelar"), this);

    layoutBotones->addStretch();
    layoutBotones->addWidget(m_btnExportar);
    layoutBotones->addWidget(m_btnLanzar);
    layoutBotones->addWidget(m_btnCancelar);

    // Ensamblamos los paneles verticales en el lienzo del QDialog
    layoutPrincipal->addWidget(grupoKeywords);
    layoutPrincipal->addWidget(grupoDatos);
    layoutPrincipal->addLayout(layoutBotones);

    // Disparamos las conexiones directas de los botones de código
    connect(m_btnExportar, &QPushButton::clicked, this, &DialogoMopac::alExportarEntrada);
    connect(m_btnLanzar,   &QPushButton::clicked, this, &DialogoMopac::alLanzarSimulacion);
    connect(m_btnCancelar, &QPushButton::clicked, this, &DialogoMopac::reject);
}

QString DialogoMopac::construirKeywords() {
    QString cmdKeywords = m_comboMetodo->currentText(); // "AM1", "PM3", etc.

    if (m_comboTarea->currentIndex() == 1) {
        cmdKeywords += " FORCE"; // Si elige la tarea de frecuencias, inyectamos su token cuántico
    }

    if (m_checkPrecision->isChecked()) {
        cmdKeywords += " PRECISE";
    }

    return cmdKeywords.trimmed().toUpper();
}

void DialogoMopac::alExportarEntrada() {
    // LO QUE VENDRÁ: Aquí dentro le notificaremos a m_vista que el usuario quiere exportar,
    // pasándole las Keywords calculadas por 'construirKeywords()' y el texto del comentario.
}

void DialogoMopac::alLanzarSimulacion() {
    // LO QUE VENDRÁ: Aquí notificaremos a m_vista que el usuario quiere calcular en vivo.
}
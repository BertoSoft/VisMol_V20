#include "dialogomopac.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

DialogoMopac::DialogoMopac(QString& argumentos, QString& comentario, QWidget *parent)
    : QDialog(parent), m_argumentosOut(argumentos), m_comentarioOut(comentario) {

    initUi();

    // Si el proyecto ya cuenta con un título asignado en la sesión, lo cargamos en el editor
    if (!m_comentarioOut.isEmpty()) {
        m_txtComentario->setText(m_comentarioOut);
    }
}

DialogoMopac::~DialogoMopac() {
    // Qt destruye de forma automática todos los widgets que tengan asignado 'this' como padre
}

void DialogoMopac::initUi() {
    setWindowTitle(tr("Configuración de Optimización y Modelado"));
    setMinimumWidth(600);

    QVBoxLayout* layoutPrincipal = new QVBoxLayout(this);

    // =========================================================================
    // 1. BLOQUE SUPERIOR: PARÁMETROS CUÁNTICOS (KEYWORDS)
    // =========================================================================
    QGroupBox* grupoKeywords = new QGroupBox(tr("Estrategia del Cálculo Cuántico MOPAC"), this);
    QFormLayout* layoutForm = new QFormLayout(grupoKeywords);

    m_comboMetodo = new QComboBox(this);
    m_comboMetodo->addItems({"AM1", "PM3", "MNDO"});

    m_comboTarea = new QComboBox(this);
    m_comboTarea->addItems({
        tr("Optimización Geométrica (Solo Final)"),      // Índice 0
        tr("Optimización Geométrica Detallada (AUX)"),   // Índice 1 -> Crucial para simulación en vivo
        tr("Modos Vibracionales (FORCE)")                // Índice 2
    });

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
    // 3. BLOQUE INFERIOR: BOTONERA DE ACCIÓN
    // =========================================================================
    QHBoxLayout* layoutBotones = new QHBoxLayout();
    m_btnExportar = new QPushButton(tr("📦 Exportar Entrada"), this);
    m_btnLanzar   = new QPushButton(tr("🚀 Lanzar Simulación"), this);
    m_btnCancelar = new QPushButton(tr("❌ Cancelar"), this);

    layoutBotones->addStretch();
    layoutBotones->addWidget(m_btnExportar);
    layoutBotones->addWidget(m_btnLanzar);
    layoutBotones->addWidget(m_btnCancelar);

    // Ensamblado estructural vertical de los paneles
    layoutPrincipal->addWidget(grupoKeywords);
    layoutPrincipal->addWidget(grupoDatos);
    layoutPrincipal->addLayout(layoutBotones);

    // Conexiones de la botonera interna del diálogo
    connect(m_btnExportar, &QPushButton::clicked, this, &DialogoMopac::alExportarEntrada);
    connect(m_btnLanzar,   &QPushButton::clicked, this, &DialogoMopac::alLanzarSimulacion);
    connect(m_btnCancelar, &QPushButton::clicked, this, &DialogoMopac::reject);

    setEstilos();
}

/**
 * @brief Ensambla las directrices cuánticas en la cadena de Keywords final de MOPAC.
 * @note [CORREGIDO]: Sintaxis estricta de 'AUX' purgada de argumentos para garantizar el volcado del .aux
 */
QString DialogoMopac::construirKeywords() {
    QString cmdKeywords = m_comboMetodo->currentText();

    switch (m_comboTarea->currentIndex()) {
    case 0:
        // Optimización geométrica por defecto del motor cuántico v23
        break;

    case 1:
        // 'AUX' y 'LARGE' limpios para asegurar la generación de fotogramas de la trayectoria
        cmdKeywords += " AUX LARGE";
        break;

    case 2:
        // ─── CONFIGURACIÓN DE MODOS NORMALES EN MOPAC v23 ───
        // Al enviar 'AUX FORCE', el motor v23 primero optimizará estrictamente la
        // geometría hasta el equilibrio y luego verterá el bloque 'ATOM_X_FORCE' en el .aux
        cmdKeywords += " AUX FORCE";
        break;
    }

    if (m_checkPrecision->isChecked()) {
        cmdKeywords += " PRECISE";
    }

    return cmdKeywords.trimmed().toUpper();
}

void DialogoMopac::setEstilos(){
    m_txtComentario->setObjectName("txtComentarioResaltado");

    QString estiloComentario =
        "QLineEdit#txtComentarioResaltado {"
        "   background-color: #202225;" // Gris oscuro integrado
        "   color: #39ff14;"            // Texto verde esmeralda analítico
        "   border: 1px solid #2d2d2d;"
        "   border-radius: 4px;"
        "   padding: 6px;"
        "}";


    m_txtComentario->setStyleSheet(estiloComentario);
}

void DialogoMopac::alExportarEntrada() {
    // Volcado atómico seguro de las propiedades seleccionadas hacia las referencias de salida
    m_argumentosOut = construirKeywords();

    QString textoComentario = m_txtComentario->text().trimmed();
    m_comentarioOut = textoComentario.isEmpty() ? tr("Molecula generada con VisorMol_V20") : textoComentario;

    this->done(1); // Transacción confirmada para exportación de archivo .dat
}

void DialogoMopac::alLanzarSimulacion() {
    m_argumentosOut = construirKeywords();

    QString textoComentario = m_txtComentario->text().trimmed();
    m_comentarioOut = textoComentario.isEmpty() ? tr("Molecula generada con VisorMol_V20") : textoComentario;

    this->done(2); // Transacción confirmada para arranque de simulación cuántica asíncrona

}
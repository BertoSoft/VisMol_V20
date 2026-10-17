#ifndef DIALOGOMOPAC_H
#define DIALOGOMOPAC_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

class DialogoMopac : public QDialog
{
    Q_OBJECT

public:
    // El constructor recibe las referencias directas para volcar los parámetros cuánticos elegidos
    explicit DialogoMopac(QString& argumentos, QString& comentario, QWidget *parent = nullptr);
    ~DialogoMopac() override;

private slots:
    void alExportarEntrada();  // Finaliza el diálogo con código 1 (Exportar .dat)
    void alLanzarSimulacion(); // Finaliza el diálogo con código 2 (Simular en vivo)

private:
    void initUi();
    QString construirKeywords(); // Construye la cabecera cuántica estándar de instrucciones
    void    setEstilos();

    // Referencias seguras de salida a las variables del flujo de la ventana
    QString&    m_argumentosOut;
    QString&    m_comentarioOut;

    // Componentes interactivos de configuración molecular
    QComboBox*   m_comboMetodo    = nullptr;
    QComboBox*   m_comboTarea     = nullptr;
    QCheckBox*   m_checkPrecision = nullptr;
    QLineEdit*   m_txtComentario  = nullptr;

    // Botonera de control
    QPushButton* m_btnExportar     = nullptr;
    QPushButton* m_btnLanzar       = nullptr;
    QPushButton* m_btnCancelar     = nullptr;
};

#endif // DIALOGOMOPAC_H
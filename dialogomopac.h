#ifndef DIALOGOMOPAC_H
#define DIALOGOMOPAC_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

// Declaración hacia adelante para enlazar con la ventana principal sin incluir toda su cabecera aquí
class MoleculaVista;

class DialogoMopac : public QDialog
{
    Q_OBJECT

public:
    // El constructor recibe obligatoriamente a MoleculaVista para poder comunicarse con ella
    explicit DialogoMopac(QString& argumentos, QString& comentario, QWidget *parent = nullptr);
    ~DialogoMopac() override;

private slots:
    void alExportarEntrada();  // Slot para cuando el usuario pulse exportar archivo .dat
    void alLanzarSimulacion(); // Slot para cuando el usuario decida calcular

private:
    void initUi();
    QString construirKeywords(); // Ensambla los comandos cuánticos ("AM1 PRECISE", etc.)
    void    setEstilos();

    // Variables de clase que guardan comentario y argumentos
    QString*    m_comentario = nullptr;
    QString*    m_argumentos = nullptr;

    // Componentes de configuración de Keywords generados estrictamente por código
    QComboBox*   m_comboMetodo    = nullptr;
    QComboBox*   m_comboTarea     = nullptr;
    QCheckBox*   m_checkPrecision = nullptr;
    QLineEdit*   m_txtComentario  = nullptr;

    // Botonera de acciones
    QPushButton* m_btnExportar     = nullptr;
    QPushButton* m_btnLanzar       = nullptr;
    QPushButton* m_btnCancelar     = nullptr;
};

#endif // DIALOGOMOPAC_H
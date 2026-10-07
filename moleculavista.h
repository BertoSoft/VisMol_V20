#ifndef MOLECULAVISTA_H
#define MOLECULAVISTA_H

#include <QMainWindow>

class MoleculaVista : public QMainWindow
{
    Q_OBJECT

public:
    explicit MoleculaVista(QWidget *parent = nullptr);
    ~MoleculaVista() override;

    // Variables y Ctes de Clase
    const QString   NOMBRE_APP = "VisMol V 2.0";

private:

    // Funciones Privadas
    void initUi();
    void initMenu();
};
#endif // MOLECULAVISTA_H

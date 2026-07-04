//realizado por: paola pena
//fecha: 2026/07/04
#ifndef LOGIN_DIALOG_HPP
#define LOGIN_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace gestor::frontend::dialogs {
    class LoginDialog : public QDialog {
        Q_OBJECT
    public:
        explicit LoginDialog(QWidget *parent = nullptr);
        ~LoginDialog();

    private slots:
        void onLoginClicked();

    private:
        QLineEdit *usernameEdit;
        QLineEdit *passwordEdit;
        QPushButton *loginButton;
        QPushButton *cancelButton;
        QLabel *statusLabel;
    };
}
#endif
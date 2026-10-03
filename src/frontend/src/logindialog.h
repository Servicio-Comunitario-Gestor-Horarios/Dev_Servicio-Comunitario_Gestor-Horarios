#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private slots:
    void onLoginClicked();

private:
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QLabel *statusLabel;

    static constexpr const char *CORRECT_PASSWORD = "123456";
};

#endif // LOGINDIALOG_H
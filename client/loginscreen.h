#ifndef LOGINSCREEN_H
#define LOGINSCREEN_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

// Forward declaration for UI class (generated from .ui file)
namespace Ui {
    class LoginScreen;
}

class ProtocolHandler;

class LoginScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoginScreen(QWidget *parent = nullptr);
    ~LoginScreen();

    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);
    void showError(const QString& message);
    void clearFields();

signals:
    void loginSuccess();
    void switchToRegister();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onSwitchToRegisterClicked();
    void onSwitchToLoginClicked();

private:
    void setupUI();
    void setupConnections();  // Connect signals and slots
    void setInitialVisibility();  // Set initial visibility state for widgets

    Ui::LoginScreen* ui;  // UI loaded from .ui file
    
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QLineEdit* confirmPasswordEdit_;
    QPushButton* loginButton_;
    QPushButton* registerButton_;
    QPushButton* switchToRegisterButton_;
    QPushButton* switchToLoginButton_;
    QLabel* errorLabel_;
    
    ProtocolHandler* protocol_;
    bool demoMode_;
    bool showingLogin_;
};

#endif // LOGINSCREEN_H

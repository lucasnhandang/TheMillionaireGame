#include "loginscreen.h"
#include "ui_loginscreen.h"  // Generated from loginscreen.ui
#include "protocol_handler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <iostream>

LoginScreen::LoginScreen(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginScreen)
    , protocol_(nullptr)
    , demoMode_(false)
    , showingLogin_(true)
{
    ui->setupUi(this);  // Load UI from .ui file
    
    // Get widget pointers from UI
    usernameEdit_ = ui->usernameEdit;
    passwordEdit_ = ui->passwordEdit;
    confirmPasswordEdit_ = ui->confirmPasswordEdit;
    loginButton_ = ui->loginButton;
    registerButton_ = ui->registerButton;
    switchToRegisterButton_ = ui->switchToRegisterButton;
    errorLabel_ = ui->errorLabel;
    
    // Check if switchToLoginButton exists in UI, if not create it
    // (You may need to add this button to your .ui file)
    switchToLoginButton_ = findChild<QPushButton*>("switchToLoginButton");
    if (!switchToLoginButton_) {
        // Create it if it doesn't exist in UI
        switchToLoginButton_ = new QPushButton("Switch to Login", this);
        switchToLoginButton_->setStyleSheet(
            "QPushButton {"
            "  background-color: transparent;"
            "  color: #1E88E5;"
            "  font-size: 14px;"
            "  padding: 8px;"
            "  border: none;"
            "  text-decoration: underline;"
            "}"
        );
        switchToLoginButton_->setObjectName("switchToLoginButton");
    }
    
    setupConnections();
    setInitialVisibility();
}

LoginScreen::~LoginScreen()
{
    delete ui;
}

void LoginScreen::setupConnections()
{
    // Connect signals and slots
    connect(loginButton_, &QPushButton::clicked, this, &LoginScreen::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginScreen::onRegisterClicked);
    connect(switchToRegisterButton_, &QPushButton::clicked, this, &LoginScreen::onSwitchToRegisterClicked);
    connect(switchToLoginButton_, &QPushButton::clicked, this, &LoginScreen::onSwitchToLoginClicked);
    
    // Set password echo mode (UI file might not have this)
    if (passwordEdit_) {
        passwordEdit_->setEchoMode(QLineEdit::Password);
    }
    if (confirmPasswordEdit_) {
        confirmPasswordEdit_->setEchoMode(QLineEdit::Password);
    }
}

void LoginScreen::setInitialVisibility()
{
    // Initial state: Login mode
    // Hide register-specific widgets
    confirmPasswordEdit_->hide();
    registerButton_->hide();
    switchToLoginButton_->hide();
    
    // Show login-specific widgets (explicit for clarity)
    loginButton_->show();
    switchToRegisterButton_->show();
    
    // Error label starts hidden
    errorLabel_->hide();
}

void LoginScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void LoginScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

void LoginScreen::showError(const QString& message)
{
    errorLabel_->setText(message);
    errorLabel_->setVisible(true);
}

void LoginScreen::clearFields()
{
    usernameEdit_->clear();
    passwordEdit_->clear();
    confirmPasswordEdit_->clear();
    errorLabel_->clear();
    errorLabel_->setVisible(false);
}

void LoginScreen::onLoginClicked()
{
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        showError("Please enter username and password");
        return;
    }
    
    if (demoMode_) {
        emit loginSuccess(username, "admin");  // Demo mode = admin for testing
        return;
    }
    
    if (!protocol_) {
        showError("Not connected to server");
        return;
    }
    
    ProtocolHandler::LoginResponse response = protocol_->login(username.toStdString(), password.toStdString());
    std::cerr << "[DEBUG] LoginScreen - response code: " << response.responseCode 
              << ", role from response: '" << response.role << "'" << std::endl;
    if (response.responseCode == 200) {
        QString role = QString::fromStdString(response.role);
        std::cerr << "[DEBUG] LoginScreen - emitting loginSuccess with role: '" << role.toStdString() << "'" << std::endl;
        emit loginSuccess(username, role);
    } else {
        showError(QString::fromStdString(response.message));
    }
}

void LoginScreen::onRegisterClicked()
{
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();
    QString confirmPassword = confirmPasswordEdit_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        showError("Please fill in all fields");
        return;
    }
    
    if (password.length() < 8) {
        showError("Password must be at least 8 characters");
        return;
    }
    
    if (password != confirmPassword) {
        showError("Passwords do not match");
        return;
    }
    
    if (demoMode_) {
        showError("Registration successful! Please login.");
        emit switchToRegister(); // This will switch to login
        return;
    }
    
    if (!protocol_) {
        showError("Not connected to server");
        return;
    }
    
    int code = protocol_->registerUser(username.toStdString(), password.toStdString());
    if (code == 201) {
        showError("Registration successful! Please login.");
        emit switchToRegister(); // This will switch to login
    } else {
        showError("Registration failed");
    }
}

void LoginScreen::onSwitchToRegisterClicked()
{
    showingLogin_ = false;
    loginButton_->setVisible(false);
    registerButton_->setVisible(true);
    switchToRegisterButton_->setVisible(false);
    switchToLoginButton_->setVisible(true);
    confirmPasswordEdit_->setVisible(true);
    clearFields();
    emit switchToRegister();
}

void LoginScreen::onSwitchToLoginClicked()
{
    showingLogin_ = true;
    loginButton_->setVisible(true);
    registerButton_->setVisible(false);
    switchToRegisterButton_->setVisible(true);
    switchToLoginButton_->setVisible(false);
    confirmPasswordEdit_->setVisible(false);
    clearFields();
}

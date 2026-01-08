#include "loginscreen.h"
#include "protocol_handler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

LoginScreen::LoginScreen(QWidget *parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , demoMode_(false)
    , showingLogin_(true)
{
    setupUI();
}

void LoginScreen::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    QLabel* titleLabel = new QLabel("Who Wants to be a Millionaire", this);
    titleLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: white; margin-bottom: 20px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Login form
    QWidget* loginForm = new QWidget(this);
    QFormLayout* formLayout = new QFormLayout(loginForm);
    formLayout->setSpacing(15);
    
    usernameEdit_ = new QLineEdit(this);
    usernameEdit_->setPlaceholderText("Enter username");
    usernameEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding: 10px;"
        "  font-size: 16px;"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 5px;"
        "  background-color: white;"
        "}"
    );
    formLayout->addRow("Username:", usernameEdit_);
    
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setPlaceholderText("Enter password");
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding: 10px;"
        "  font-size: 16px;"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 5px;"
        "  background-color: white;"
        "}"
    );
    formLayout->addRow("Password:", passwordEdit_);
    
    confirmPasswordEdit_ = new QLineEdit(this);
    confirmPasswordEdit_->setPlaceholderText("Confirm password");
    confirmPasswordEdit_->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit_->setStyleSheet(
        "QLineEdit {"
        "  padding: 10px;"
        "  font-size: 16px;"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 5px;"
        "  background-color: white;"
        "}"
    );
    confirmPasswordEdit_->setVisible(false);
    
    errorLabel_ = new QLabel(this);
    errorLabel_->setStyleSheet("color: #F44336; font-size: 14px;");
    errorLabel_->setAlignment(Qt::AlignCenter);
    errorLabel_->setWordWrap(true);
    errorLabel_->setVisible(false);
    
    loginButton_ = new QPushButton("Login", this);
    loginButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 12px 40px;"
        "  border-radius: 8px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
    );
    
    registerButton_ = new QPushButton("Register", this);
    registerButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 12px 40px;"
        "  border-radius: 8px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45A049;"
        "}"
    );
    registerButton_->setVisible(false);
    
    switchToRegisterButton_ = new QPushButton("Switch to Register", this);
    switchToRegisterButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  color: #1E88E5;"
        "  font-size: 14px;"
        "  padding: 8px;"
        "  border: none;"
        "  text-decoration: underline;"
        "}"
    );
    
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
    switchToLoginButton_->setVisible(false);
    
    connect(loginButton_, &QPushButton::clicked, this, &LoginScreen::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginScreen::onRegisterClicked);
    connect(switchToRegisterButton_, &QPushButton::clicked, this, &LoginScreen::onSwitchToRegisterClicked);
    connect(switchToLoginButton_, &QPushButton::clicked, this, &LoginScreen::onSwitchToLoginClicked);
    
    mainLayout->addWidget(loginForm, 0, Qt::AlignCenter);
    mainLayout->addWidget(errorLabel_);
    mainLayout->addWidget(loginButton_, 0, Qt::AlignCenter);
    mainLayout->addWidget(registerButton_, 0, Qt::AlignCenter);
    mainLayout->addWidget(switchToRegisterButton_, 0, Qt::AlignCenter);
    mainLayout->addWidget(switchToLoginButton_, 0, Qt::AlignCenter);
    
    setStyleSheet("background-color: #0D1B2A; color: white;");
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
        emit loginSuccess();
        return;
    }
    
    if (!protocol_) {
        showError("Not connected to server");
        return;
    }
    
    ProtocolHandler::LoginResponse response = protocol_->login(username.toStdString(), password.toStdString());
    if (response.responseCode == 200) {
        emit loginSuccess();
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

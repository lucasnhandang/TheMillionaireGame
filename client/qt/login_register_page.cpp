#include "login_register_page.h"
#include "network_thread.h"
#include <QApplication>
#include <QStyle>
#include <QFont>
#include <QPalette>

namespace MillionaireGame {

LoginRegisterPage::LoginRegisterPage(NetworkThread* networkThread, QWidget* parent)
    : QWidget(parent), network_thread_(networkThread) {
    setupUI();
    
    // Connect network signals
    if (network_thread_) {
        connect(network_thread_, &NetworkThread::loginResponse,
                this, &LoginRegisterPage::onLoginResponse);
        connect(network_thread_, &NetworkThread::registerResponse,
                this, &LoginRegisterPage::onRegisterResponse);
        connect(network_thread_, &NetworkThread::connectionStatusChanged,
                this, &LoginRegisterPage::onConnectionStatusChanged);
    }
}

LoginRegisterPage::~LoginRegisterPage() {
}

void LoginRegisterPage::setupUI() {
    setWindowTitle("Who Wants to Be a Millionaire - Login");
    resize(800, 600);
    
    // Set dark theme
    setStyleSheet(
        "QWidget { background-color: #1a1a2e; color: #eee; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 10px; padding: 15px; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        "QPushButton:pressed { background-color: #e94560; }"
        "QLabel { color: #eee; font-size: 24px; }"
    );
    
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(30);
    main_layout->setContentsMargins(50, 50, 50, 50);
    
    // Title
    title_label_ = new QLabel("WHO WANTS TO BE A MILLIONAIRE", this);
    title_label_->setAlignment(Qt::AlignCenter);
    QFont title_font = title_label_->font();
    title_font.setPointSize(32);
    title_font.setBold(true);
    title_label_->setFont(title_font);
    title_label_->setStyleSheet("color: #e94560;");
    main_layout->addWidget(title_label_);
    
    main_layout->addStretch();
    
    // Status label
    status_label_ = new QLabel("Đang kết nối đến server...", this);
    status_label_->setAlignment(Qt::AlignCenter);
    status_label_->setStyleSheet("color: #aaa; font-size: 14px;");
    main_layout->addWidget(status_label_);
    
    // Buttons
    login_button_ = new QPushButton("LOGIN", this);
    login_button_->setMinimumHeight(60);
    connect(login_button_, &QPushButton::clicked, this, &LoginRegisterPage::onLoginButtonClicked);
    main_layout->addWidget(login_button_);
    
    register_button_ = new QPushButton("REGISTER", this);
    register_button_->setMinimumHeight(60);
    connect(register_button_, &QPushButton::clicked, this, &LoginRegisterPage::onRegisterButtonClicked);
    main_layout->addWidget(register_button_);
    
    quit_button_ = new QPushButton("QUIT", this);
    quit_button_->setMinimumHeight(60);
    connect(quit_button_, &QPushButton::clicked, this, &LoginRegisterPage::onQuitButtonClicked);
    main_layout->addWidget(quit_button_);
    
    main_layout->addStretch();
    
    setupLoginDialog();
    setupRegisterDialog();
}

void LoginRegisterPage::setupLoginDialog() {
    login_dialog_ = new QDialog(this);
    login_dialog_->setWindowTitle("Login");
    login_dialog_->setModal(true);
    login_dialog_->resize(400, 250);
    login_dialog_->setStyleSheet(
        "QDialog { background-color: #1a1a2e; }"
        "QLabel { color: #eee; font-size: 14px; }"
        "QLineEdit { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 8px; color: #eee; font-size: 14px; }"
        "QLineEdit:focus { border-color: #e94560; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(login_dialog_);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);
    
    QLabel* username_label = new QLabel("Username:", login_dialog_);
    layout->addWidget(username_label);
    
    login_username_ = new QLineEdit(login_dialog_);
    login_username_->setPlaceholderText("Enter username");
    layout->addWidget(login_username_);
    
    QLabel* password_label = new QLabel("Password:", login_dialog_);
    layout->addWidget(password_label);
    
    login_password_ = new QLineEdit(login_dialog_);
    login_password_->setPlaceholderText("Enter password");
    login_password_->setEchoMode(QLineEdit::Password);
    layout->addWidget(login_password_);
    
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->setSpacing(10);
    
    login_submit_ = new QPushButton("Login", login_dialog_);
    connect(login_submit_, &QPushButton::clicked, this, &LoginRegisterPage::onLoginFormSubmitted);
    button_layout->addWidget(login_submit_);
    
    login_cancel_ = new QPushButton("Cancel", login_dialog_);
    connect(login_cancel_, &QPushButton::clicked, login_dialog_, &QDialog::reject);
    button_layout->addWidget(login_cancel_);
    
    layout->addLayout(button_layout);
}

void LoginRegisterPage::setupRegisterDialog() {
    register_dialog_ = new QDialog(this);
    register_dialog_->setWindowTitle("Register");
    register_dialog_->setModal(true);
    register_dialog_->resize(400, 320);
    register_dialog_->setStyleSheet(
        "QDialog { background-color: #1a1a2e; }"
        "QLabel { color: #eee; font-size: 14px; }"
        "QLineEdit { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 8px; color: #eee; font-size: 14px; }"
        "QLineEdit:focus { border-color: #e94560; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(register_dialog_);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);
    
    QLabel* username_label = new QLabel("Username:", register_dialog_);
    layout->addWidget(username_label);
    
    register_username_ = new QLineEdit(register_dialog_);
    register_username_->setPlaceholderText("Enter username");
    layout->addWidget(register_username_);
    
    QLabel* password_label = new QLabel("Password:", register_dialog_);
    layout->addWidget(password_label);
    
    register_password_ = new QLineEdit(register_dialog_);
    register_password_->setPlaceholderText("Enter password");
    register_password_->setEchoMode(QLineEdit::Password);
    layout->addWidget(register_password_);
    
    QLabel* confirm_label = new QLabel("Confirm Password:", register_dialog_);
    layout->addWidget(confirm_label);
    
    register_confirm_password_ = new QLineEdit(register_dialog_);
    register_confirm_password_->setPlaceholderText("Confirm password");
    register_confirm_password_->setEchoMode(QLineEdit::Password);
    layout->addWidget(register_confirm_password_);
    
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->setSpacing(10);
    
    register_submit_ = new QPushButton("Register", register_dialog_);
    connect(register_submit_, &QPushButton::clicked, this, &LoginRegisterPage::onRegisterFormSubmitted);
    button_layout->addWidget(register_submit_);
    
    register_cancel_ = new QPushButton("Cancel", register_dialog_);
    connect(register_cancel_, &QPushButton::clicked, register_dialog_, &QDialog::reject);
    button_layout->addWidget(register_cancel_);
    
    layout->addLayout(button_layout);
}

void LoginRegisterPage::onLoginButtonClicked() {
    if (!network_thread_ || !network_thread_->isConnected()) {
        showError("Chưa kết nối đến server!");
        return;
    }
    login_dialog_->exec();
}

void LoginRegisterPage::onRegisterButtonClicked() {
    if (!network_thread_ || !network_thread_->isConnected()) {
        showError("Chưa kết nối đến server!");
        return;
    }
    register_dialog_->exec();
}

void LoginRegisterPage::onQuitButtonClicked() {
    emit quitRequested();
}

void LoginRegisterPage::onLoginFormSubmitted() {
    QString username = login_username_->text().trimmed();
    QString password = login_password_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        showError("Vui lòng nhập đầy đủ thông tin!");
        return;
    }
    
    if (!network_thread_ || !network_thread_->getProtocolHandler()) {
        showError("Lỗi kết nối!");
        return;
    }
    
    status_label_->setText("Đang đăng nhập...");
    status_label_->setStyleSheet("color: #ffa500; font-size: 14px;");
    
    // Login in a separate thread to avoid blocking UI
    QThread* login_thread = QThread::create([this, username, password]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->login(username.toStdString(), password.toStdString());
        
        QMetaObject::invokeMethod(this, "onLoginResponse", Qt::QueuedConnection,
            Q_ARG(bool, response.success),
            Q_ARG(int, response.responseCode),
            Q_ARG(QString, QString::fromStdString(response.message)),
            Q_ARG(QString, QString::fromStdString(response.authToken)),
            Q_ARG(QString, QString::fromStdString(response.username)),
            Q_ARG(QString, QString::fromStdString(response.role)));
    });
    connect(login_thread, &QThread::finished, login_thread, &QThread::deleteLater);
    login_thread->start();
    
    login_dialog_->accept();
}

void LoginRegisterPage::onRegisterFormSubmitted() {
    QString username = register_username_->text().trimmed();
    QString password = register_password_->text();
    QString confirm = register_confirm_password_->text();
    
    if (username.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        showError("Vui lòng nhập đầy đủ thông tin!");
        return;
    }
    
    if (password != confirm) {
        showError("Mật khẩu xác nhận không khớp!");
        return;
    }
    
    if (!network_thread_ || !network_thread_->getProtocolHandler()) {
        showError("Lỗi kết nối!");
        return;
    }
    
    status_label_->setText("Đang đăng ký...");
    status_label_->setStyleSheet("color: #ffa500; font-size: 14px;");
    
    // Register in a separate thread
    QThread* register_thread = QThread::create([this, username, password]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->registerUser(username.toStdString(), password.toStdString());
        
        QMetaObject::invokeMethod(this, "onRegisterResponse", Qt::QueuedConnection,
            Q_ARG(bool, response.success),
            Q_ARG(int, response.responseCode),
            Q_ARG(QString, QString::fromStdString(response.message)));
    });
    connect(register_thread, &QThread::finished, register_thread, &QThread::deleteLater);
    register_thread->start();
    
    register_dialog_->accept();
}

void LoginRegisterPage::onLoginResponse(bool success, int code, const QString& message,
                                        const QString& authToken, const QString& username, const QString& role) {
    if (success) {
        showSuccess("Đăng nhập thành công!");
        status_label_->setText("Đăng nhập thành công!");
        status_label_->setStyleSheet("color: #0f0; font-size: 14px;");
        emit loginSuccessful(authToken, username, role);
    } else {
        showError("Đăng nhập thất bại: " + message);
        status_label_->setText("Đăng nhập thất bại");
        status_label_->setStyleSheet("color: #f00; font-size: 14px;");
    }
}

void LoginRegisterPage::onRegisterResponse(bool success, int code, const QString& message) {
    if (success) {
        showSuccess("Đăng ký thành công! Vui lòng đăng nhập.");
        status_label_->setText("Đăng ký thành công!");
        status_label_->setStyleSheet("color: #0f0; font-size: 14px;");
    } else {
        showError("Đăng ký thất bại: " + message);
        status_label_->setText("Đăng ký thất bại");
        status_label_->setStyleSheet("color: #f00; font-size: 14px;");
    }
}

void LoginRegisterPage::onConnectionStatusChanged(bool connected) {
    if (connected) {
        status_label_->setText("Đã kết nối đến server");
        status_label_->setStyleSheet("color: #0f0; font-size: 14px;");
    } else {
        status_label_->setText("Mất kết nối đến server");
        status_label_->setStyleSheet("color: #f00; font-size: 14px;");
    }
}

void LoginRegisterPage::showError(const QString& message) {
    QMessageBox::critical(this, "Lỗi", message);
}

void LoginRegisterPage::showSuccess(const QString& message) {
    QMessageBox::information(this, "Thành công", message);
}

} // namespace MillionaireGame


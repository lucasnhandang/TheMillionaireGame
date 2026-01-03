#ifndef LOGIN_REGISTER_PAGE_H
#define LOGIN_REGISTER_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDialog>
#include <QMessageBox>
#include "../src/protocol_handler.h"

namespace MillionaireGame {

class NetworkThread;

/**
 * Login/Register page
 * Main entry point - user must login or register to continue
 */
class LoginRegisterPage : public QWidget {
    Q_OBJECT

public:
    explicit LoginRegisterPage(NetworkThread* networkThread, QWidget* parent = nullptr);
    ~LoginRegisterPage();

signals:
    void loginSuccessful(const QString& authToken, const QString& username, const QString& role);
    void quitRequested();

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();
    void onQuitButtonClicked();
    void onLoginFormSubmitted();
    void onRegisterFormSubmitted();
    void onLoginResponse(bool success, int code, const QString& message,
                        const QString& authToken, const QString& username, const QString& role);
    void onRegisterResponse(bool success, int code, const QString& message);
    void onConnectionStatusChanged(bool connected);

private:
    NetworkThread* network_thread_;
    
    // Main page widgets
    QPushButton* login_button_;
    QPushButton* register_button_;
    QPushButton* quit_button_;
    QLabel* title_label_;
    QLabel* status_label_;
    
    // Login form dialog
    QDialog* login_dialog_;
    QLineEdit* login_username_;
    QLineEdit* login_password_;
    QPushButton* login_submit_;
    QPushButton* login_cancel_;
    
    // Register form dialog
    QDialog* register_dialog_;
    QLineEdit* register_username_;
    QLineEdit* register_password_;
    QLineEdit* register_confirm_password_;
    QPushButton* register_submit_;
    QPushButton* register_cancel_;
    
    void setupUI();
    void setupLoginDialog();
    void setupRegisterDialog();
    void showError(const QString& message);
    void showSuccess(const QString& message);
};

} // namespace MillionaireGame

#endif // LOGIN_REGISTER_PAGE_H


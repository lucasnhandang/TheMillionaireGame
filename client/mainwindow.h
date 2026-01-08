#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <memory>

class ProtocolHandler;
class LoginScreen;
class HomeScreen;
class GameScreen;
class ResultScreen;
class GameState;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);

private slots:
    void onLoginSuccess();
    void onGameStart();
    void onGameEnd();
    void onShowResult();
    void onBackToHome();
    void processGameEvents();

private:
    void setupUI();
    void setupConnections();
    void setupNotificationHandler();

    QStackedWidget* stackedWidget_;
    
    LoginScreen* loginScreen_;
    HomeScreen* homeScreen_;
    GameScreen* gameScreen_;
    ResultScreen* resultScreen_;
    
    ProtocolHandler* protocol_;
    std::unique_ptr<GameState> gameState_;
    bool demoMode_;
    QTimer* eventProcessTimer_;
};

#endif // MAINWINDOW_H

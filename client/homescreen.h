#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>

class ProtocolHandler;

class HomeScreen : public QWidget
{
    Q_OBJECT

public:
    explicit HomeScreen(QWidget *parent = nullptr);

    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);
    void setUsername(const QString& username);
    void showError(const QString& message);

signals:
    void playGameClicked();
    void leaderboardClicked();
    void settingsClicked();
    void instructionsClicked();
    void friendsClicked();

private slots:
    void onPlayGameClicked();
    void onResumeGameClicked();

private:
    void setupUI();
    void loadLogo();

    QLabel* logoLabel_;
    QPushButton* playGameButton_;
    QPushButton* resumeGameButton_;
    QPushButton* leaderboardButton_;
    QPushButton* settingsButton_;
    QPushButton* instructionsButton_;
    QPushButton* friendsButton_;
    QLabel* errorLabel_;
    QLabel* usernameLabel_;
    
    ProtocolHandler* protocol_;
    bool demoMode_;
    QPixmap logoPixmap_;
};

#endif // HOMESCREEN_H

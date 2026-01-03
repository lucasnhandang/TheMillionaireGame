#ifndef LANDING_PAGE_H
#define LANDING_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>

namespace MillionaireGame {

class NetworkThread;
class InstructionDialog;

/**
 * Landing page - Main menu after login
 */
class LandingPage : public QWidget {
    Q_OBJECT

public:
    explicit LandingPage(NetworkThread* networkThread, const QString& username, 
                        const QString& role, QWidget* parent = nullptr);
    ~LandingPage();

signals:
    void playRequested();
    void leaderboardRequested();
    void friendRequested();
    void logoutRequested();
    void quitRequested();

private slots:
    void onPlayButtonClicked();
    void onInstructionButtonClicked();
    void onLeaderboardButtonClicked();
    void onFriendButtonClicked();
    void onQuitButtonClicked();
    void onLogoutMenuTriggered();
    void onLogoutTimerTimeout();

private:
    NetworkThread* network_thread_;
    QString username_;
    QString role_;
    
    QLabel* welcome_label_;
    QPushButton* play_button_;
    QPushButton* instruction_button_;
    QPushButton* leaderboard_button_;
    QPushButton* friend_button_;
    QPushButton* quit_button_;
    QPushButton* logout_button_;
    QMenu* user_menu_;
    
    InstructionDialog* instruction_dialog_;
    QTimer* logout_timer_;
    
    void setupUI();
    void setupUserMenu();
};

} // namespace MillionaireGame

#endif // LANDING_PAGE_H


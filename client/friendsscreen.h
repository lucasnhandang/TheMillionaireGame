#ifndef FRIENDSSCREEN_H
#define FRIENDSSCREEN_H

#include <QWidget>
#include <QString>
#include <QSet>
#include <string>

class ProtocolHandler;
class QListWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QTabWidget;

class FriendsScreen : public QWidget
{
    Q_OBJECT
public:
    explicit FriendsScreen(QWidget* parent = nullptr);
    
    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);
    void refreshData();
    
    void handleFriendRequestNotification(const std::string& data);
    void handleFriendAcceptedNotification(const std::string& data);
    void handleNewMessageNotification(const std::string& data);
    
signals:
    void backToHome();
    
private slots:
    void onBackClicked();
    void onSearchFriend();
    void onSendRequest();
    void onFriendSelectionChanged();
    void onRequestSelectionChanged();
    void onAcceptRequest();
    void onRejectRequest();
    void onRemoveFriend();
    void onSendMessage();
    
private:
    void setupUI();
    void loadFriends();
    void loadFriendRequests();
    void loadConversation(const QString& friendUsername);
    void appendChatMessage(const QString& from, const QString& content, long long timestamp = 0);
    void setChatState(bool enabled);
    QString buildFriendLabel(const QString& username, const QString& status) const;
    
    ProtocolHandler* protocol_;
    bool demoMode_;
    QString currentChatFriend_;
    QString pendingRequestUser_;
    QSet<QString> pendingMessageUsers_;
    
    QLineEdit* searchInput_;
    QLabel* searchResultLabel_;
    QPushButton* sendRequestButton_;
    
    QListWidget* friendsList_;
    QPushButton* removeFriendButton_;
    
    QListWidget* requestsList_;
    QPushButton* acceptRequestButton_;
    QPushButton* rejectRequestButton_;
    
    QLabel* chatHeaderLabel_;
    QListWidget* chatList_;
    QLineEdit* messageInput_;
    QPushButton* sendMessageButton_;
};

#endif // FRIENDSSCREEN_H


#include "friendsscreen.h"
#include "protocol_handler.h"
#include "json_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QDateTime>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

FriendsScreen::FriendsScreen(QWidget* parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , demoMode_(false)
{
    setupUI();
}

void FriendsScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void FriendsScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

void FriendsScreen::refreshData()
{
    searchInput_->clear();
    searchResultLabel_->setText("");
    sendRequestButton_->setEnabled(false);
    pendingRequestUser_.clear();
    loadFriends();
    loadFriendRequests();
    setChatState(false);
    chatList_->clear();
    chatHeaderLabel_->setText("Select a friend to start chatting");
}

QString FriendsScreen::buildFriendLabel(const QString& username, const QString& status) const
{
    QString label = username;
    if (!status.isEmpty()) {
        label += " (" + status + ")";
    }
    if (pendingMessageUsers_.contains(username)) {
        label += "  •";
    }
    return label;
}

void FriendsScreen::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);
    
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Friends & Chat", this);
    titleLabel->setStyleSheet("font-size: 26px; font-weight: bold; color: white;");
    
    QPushButton* backButton = new QPushButton("Back to Home", this);
    backButton->setStyleSheet(
        "QPushButton { background-color: #666; color: white; padding: 8px 24px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #777; }"
    );
    connect(backButton, &QPushButton::clicked, this, &FriendsScreen::onBackClicked);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(backButton);
    mainLayout->addLayout(headerLayout);
    
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);
    mainLayout->addLayout(contentLayout);
    
    // Left panel
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setStyleSheet("background-color: #111827; border-radius: 12px;");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(15);
    
    // Search section
    QGroupBox* searchGroup = new QGroupBox("Find Friend", leftPanel);
    searchGroup->setStyleSheet(
        "QGroupBox { color: #00d4ff; border: 1px solid #1f2937; border-radius: 8px; margin-top: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; }"
    );
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);
    
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    searchInput_ = new QLineEdit(searchGroup);
    searchInput_->setPlaceholderText("Enter username...");
    searchInput_->setStyleSheet("background-color: #1f2937; color: white; padding: 8px; border-radius: 6px; border: 1px solid #374151;");
    QPushButton* searchButton = new QPushButton("Search", searchGroup);
    searchButton->setStyleSheet("QPushButton { background-color: #2563EB; color: white; padding: 8px 18px; border-radius: 6px; }"
                                "QPushButton:hover { background-color: #1D4ED8; }");
    connect(searchButton, &QPushButton::clicked, this, &FriendsScreen::onSearchFriend);
    searchInputLayout->addWidget(searchInput_);
    searchInputLayout->addWidget(searchButton);
    searchLayout->addLayout(searchInputLayout);
    
    searchResultLabel_ = new QLabel("", searchGroup);
    searchResultLabel_->setStyleSheet("color: #E5E7EB;");
    searchLayout->addWidget(searchResultLabel_);
    
    sendRequestButton_ = new QPushButton("Send Friend Request", searchGroup);
    sendRequestButton_->setEnabled(false);
    sendRequestButton_->setStyleSheet("QPushButton { background-color: #10B981; color: white; padding: 8px 18px; border-radius: 6px; }"
                                      "QPushButton:disabled { background-color: #374151; }");
    connect(sendRequestButton_, &QPushButton::clicked, this, &FriendsScreen::onSendRequest);
    searchLayout->addWidget(sendRequestButton_);
    
    leftLayout->addWidget(searchGroup);
    
    // Tabs for friends and requests
    QTabWidget* listTabs = new QTabWidget(leftPanel);
    listTabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #1f2937; border-radius: 6px; }"
        "QTabBar::tab { background-color: #1f2937; color: white; padding: 8px 16px; margin-right: 2px; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
        "QTabBar::tab:selected { background-color: #2563EB; }"
    );
    
    QWidget* friendsTab = new QWidget(listTabs);
    QVBoxLayout* friendsLayout = new QVBoxLayout(friendsTab);
    friendsList_ = new QListWidget(friendsTab);
    friendsList_->setStyleSheet(
        "QListWidget { background-color: #0f172a; color: white; border: 1px solid #1f2937; border-radius: 8px; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: #1E3A8A; }"
    );
    connect(friendsList_, &QListWidget::currentRowChanged, this, &FriendsScreen::onFriendSelectionChanged);
    friendsLayout->addWidget(friendsList_);
    
    removeFriendButton_ = new QPushButton("Remove Friend", friendsTab);
    removeFriendButton_->setEnabled(false);
    removeFriendButton_->setStyleSheet("QPushButton { background-color: #DC2626; color: white; padding: 8px 18px; border-radius: 6px; }"
                                       "QPushButton:disabled { background-color: #374151; }");
    connect(removeFriendButton_, &QPushButton::clicked, this, &FriendsScreen::onRemoveFriend);
    friendsLayout->addWidget(removeFriendButton_);
    
    QWidget* requestsTab = new QWidget(listTabs);
    QVBoxLayout* requestsLayout = new QVBoxLayout(requestsTab);
    
    requestsList_ = new QListWidget(requestsTab);
    requestsList_->setStyleSheet(
        "QListWidget { background-color: #0f172a; color: white; border: 1px solid #1f2937; border-radius: 8px; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: #1E3A8A; }"
    );
    connect(requestsList_, &QListWidget::currentRowChanged, this, &FriendsScreen::onRequestSelectionChanged);
    requestsLayout->addWidget(requestsList_);
    
    QHBoxLayout* requestButtonsLayout = new QHBoxLayout();
    acceptRequestButton_ = new QPushButton("Accept", requestsTab);
    acceptRequestButton_->setEnabled(false);
    acceptRequestButton_->setStyleSheet("QPushButton { background-color: #10B981; color: white; padding: 8px 18px; border-radius: 6px; }"
                                        "QPushButton:disabled { background-color: #374151; }");
    connect(acceptRequestButton_, &QPushButton::clicked, this, &FriendsScreen::onAcceptRequest);
    
    rejectRequestButton_ = new QPushButton("Reject", requestsTab);
    rejectRequestButton_->setEnabled(false);
    rejectRequestButton_->setStyleSheet("QPushButton { background-color: #DC2626; color: white; padding: 8px 18px; border-radius: 6px; }"
                                        "QPushButton:disabled { background-color: #374151; }");
    connect(rejectRequestButton_, &QPushButton::clicked, this, &FriendsScreen::onRejectRequest);
    
    requestButtonsLayout->addWidget(acceptRequestButton_);
    requestButtonsLayout->addWidget(rejectRequestButton_);
    requestsLayout->addLayout(requestButtonsLayout);
    
    listTabs->addTab(friendsTab, "Friends");
    listTabs->addTab(requestsTab, "Requests");
    leftLayout->addWidget(listTabs);
    
    contentLayout->addWidget(leftPanel, 1);
    
    // Right chat panel
    QWidget* chatPanel = new QWidget(this);
    chatPanel->setStyleSheet("background-color: #111827; border-radius: 12px;");
    QVBoxLayout* chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setSpacing(12);
    
    chatHeaderLabel_ = new QLabel("Select a friend to start chatting", chatPanel);
    chatHeaderLabel_->setStyleSheet("color: #E5E7EB; font-size: 18px; font-weight: bold;");
    chatLayout->addWidget(chatHeaderLabel_);
    
    chatList_ = new QListWidget(chatPanel);
    chatList_->setStyleSheet(
        "QListWidget { background-color: #0f172a; color: white; border: 1px solid #1f2937; border-radius: 8px; }"
        "QListWidget::item { padding: 10px; }"
    );
    chatLayout->addWidget(chatList_, 1);
    
    QHBoxLayout* messageLayout = new QHBoxLayout();
    messageInput_ = new QLineEdit(chatPanel);
    messageInput_->setPlaceholderText("Type a message...");
    messageInput_->setStyleSheet("background-color: #1f2937; color: white; padding: 10px; border-radius: 6px; border: 1px solid #374151;");
    connect(messageInput_, &QLineEdit::returnPressed, this, &FriendsScreen::onSendMessage);
    messageLayout->addWidget(messageInput_);
    
    sendMessageButton_ = new QPushButton("Send", chatPanel);
    sendMessageButton_->setStyleSheet("QPushButton { background-color: #2563EB; color: white; padding: 10px 20px; border-radius: 6px; }"
                                      "QPushButton:disabled { background-color: #374151; }");
    connect(sendMessageButton_, &QPushButton::clicked, this, &FriendsScreen::onSendMessage);
    messageLayout->addWidget(sendMessageButton_);
    
    chatLayout->addLayout(messageLayout);
    
    contentLayout->addWidget(chatPanel, 2);
    
    setChatState(false);
}

void FriendsScreen::onBackClicked()
{
    emit backToHome();
}

void FriendsScreen::onSearchFriend()
{
    QString target = searchInput_->text().trimmed();
    if (target.isEmpty()) {
        searchResultLabel_->setText("<span style='color:#F87171'>Please enter a username.</span>");
        sendRequestButton_->setEnabled(false);
        return;
    }
    
    if (demoMode_) {
        searchResultLabel_->setText(QString("<span style='color:#A7F3D0'>User %1 found (demo mode)</span>").arg(target));
        pendingRequestUser_ = target;
        sendRequestButton_->setEnabled(true);
        return;
    }
    
    if (!protocol_) {
        searchResultLabel_->setText("<span style='color:#F87171'>Not connected to server.</span>");
        sendRequestButton_->setEnabled(false);
        return;
    }
    
    ProtocolHandler::FindFriendResponse resp = protocol_->findFriend(target.toStdString());
    if (resp.responseCode != 200) {
        searchResultLabel_->setText("<span style='color:#F87171'>User not found.</span>");
        sendRequestButton_->setEnabled(false);
        return;
    }
    
    pendingRequestUser_ = QString::fromStdString(resp.username);
    if (resp.status == "self") {
        searchResultLabel_->setText("<span style='color:#FCD34D'>That's you!</span>");
        sendRequestButton_->setEnabled(false);
    } else if (resp.status == "friend") {
        searchResultLabel_->setText("<span style='color:#A5B4FC'>You're already friends.</span>");
        sendRequestButton_->setEnabled(false);
    } else {
        searchResultLabel_->setText(QString("<span style='color:#A7F3D0'>User %1 found. You can send a request.</span>")
                                    .arg(pendingRequestUser_));
        sendRequestButton_->setEnabled(true);
    }
}

void FriendsScreen::onSendRequest()
{
    if (pendingRequestUser_.isEmpty()) return;
    if (demoMode_) {
        searchResultLabel_->setText("<span style='color:#A7F3D0'>Request sent (demo).</span>");
        sendRequestButton_->setEnabled(false);
        return;
    }
    
    if (!protocol_) return;
    
    int code = protocol_->addFriend(pendingRequestUser_.toStdString());
    if (code == 200) {
        searchResultLabel_->setText("<span style='color:#A7F3D0'>Friend request sent.</span>");
        sendRequestButton_->setEnabled(false);
    } else if (code == 409) {
        searchResultLabel_->setText("<span style='color:#FCD34D'>Request already pending.</span>");
    } else {
        searchResultLabel_->setText(QString("<span style='color:#F87171'>Failed (code %1).</span>").arg(code));
    }
}

void FriendsScreen::loadFriends()
{
    friendsList_->clear();
    removeFriendButton_->setEnabled(false);
    QString selectedUsername = currentChatFriend_;
    
    if (demoMode_) {
        struct DemoFriend { QString name; QString status; };
        const QList<DemoFriend> demoData = {
            {"alice", "online"},
            {"bob", "offline"}
        };
        for (const auto& entry : demoData) {
            QListWidgetItem* item = new QListWidgetItem(buildFriendLabel(entry.name, entry.status), friendsList_);
            item->setData(Qt::UserRole, entry.name);
        }
        if (!selectedUsername.isEmpty()) {
            for (int i = 0; i < friendsList_->count(); ++i) {
                QListWidgetItem* item = friendsList_->item(i);
                if (item->data(Qt::UserRole).toString() == selectedUsername) {
                    friendsList_->setCurrentRow(i);
                    break;
                }
            }
        }
        return;
    }
    
    if (!protocol_) {
        friendsList_->addItem("Not connected to server.");
        return;
    }
    
    ProtocolHandler::FriendStatusResponse resp = protocol_->getFriendStatus();
    if (resp.responseCode != 200) {
        friendsList_->addItem(QString("Failed to load friends (code %1)").arg(resp.responseCode));
        return;
    }
    
    if (resp.friends.empty()) {
        friendsList_->addItem("No friends yet.");
        return;
    }
    
    for (const auto& friendStatus : resp.friends) {
        const QString username = QString::fromStdString(friendStatus.username);
        const QString status = QString::fromStdString(friendStatus.status);
        QListWidgetItem* item = new QListWidgetItem(buildFriendLabel(username, status), friendsList_);
        item->setData(Qt::UserRole, username);
    }
    
    if (!selectedUsername.isEmpty()) {
        for (int i = 0; i < friendsList_->count(); ++i) {
            QListWidgetItem* item = friendsList_->item(i);
            if (item->data(Qt::UserRole).toString() == selectedUsername) {
                friendsList_->setCurrentRow(i);
                break;
            }
        }
    }
}

void FriendsScreen::loadFriendRequests()
{
    requestsList_->clear();
    acceptRequestButton_->setEnabled(false);
    rejectRequestButton_->setEnabled(false);
    
    if (demoMode_) {
        QListWidgetItem* item = new QListWidgetItem("charlie - pending (demo)", requestsList_);
        item->setData(Qt::UserRole, "charlie");
        return;
    }
    
    if (!protocol_) {
        requestsList_->addItem("Not connected to server.");
        return;
    }
    
    ProtocolHandler::FriendReqListResponse resp = protocol_->getFriendReqList();
    if (resp.responseCode != 200) {
        requestsList_->addItem(QString("Failed to load requests (code %1)").arg(resp.responseCode));
        return;
    }
    
    if (resp.friendRequests.empty()) {
        requestsList_->addItem("No pending requests.");
        return;
    }
    
    for (const auto& request : resp.friendRequests) {
        QDateTime sentTime = QDateTime::fromSecsSinceEpoch(request.sentAt);
        QString label = QString::fromStdString(request.username) +
                        " - " + sentTime.toString("MMM dd hh:mm");
        QListWidgetItem* item = new QListWidgetItem(label, requestsList_);
        item->setData(Qt::UserRole, QString::fromStdString(request.username));
    }
}

void FriendsScreen::loadConversation(const QString& friendUsername)
{
    chatList_->clear();
    
    // Clear pending message indicator when opening conversation
    pendingMessageUsers_.remove(friendUsername);
    
    if (demoMode_) {
        appendChatMessage(friendUsername, "Hey there! (demo)", QDateTime::currentSecsSinceEpoch());
        appendChatMessage("You", "Hello!", QDateTime::currentSecsSinceEpoch());
        return;
    }
    
    if (!protocol_) {
        chatList_->addItem("Not connected to server.");
        setChatState(false);
        return;
    }
    
    ProtocolHandler::GetMessagesResponse resp = protocol_->getMessages(friendUsername.toStdString(), 1, 50);
    if (resp.responseCode != 200) {
        chatList_->addItem(QString("Failed to load messages (code %1)").arg(resp.responseCode));
        return;
    }
    
    for (const auto& message : resp.messages) {
        appendChatMessage(QString::fromStdString(message.from),
                          QString::fromStdString(message.content),
                          message.timestamp);
    }
    
    chatList_->scrollToBottom();
}

void FriendsScreen::appendChatMessage(const QString& from, const QString& content, long long timestamp)
{
    QString timeString;
    if (timestamp > 0) {
        timeString = QDateTime::fromSecsSinceEpoch(timestamp).toString("hh:mm");
    } else {
        timeString = QDateTime::currentDateTime().toString("hh:mm");
    }
    
    QString line = QString("[%1] %2: %3").arg(timeString, from, content);
    chatList_->addItem(line);
}

void FriendsScreen::setChatState(bool enabled)
{
    sendMessageButton_->setEnabled(enabled);
    messageInput_->setEnabled(enabled);
    if (!enabled) {
        messageInput_->clear();
    }
}

void FriendsScreen::onFriendSelectionChanged()
{
    QListWidgetItem* item = friendsList_->currentItem();
    if (!item) {
        currentChatFriend_.clear();
        chatHeaderLabel_->setText("Select a friend to start chatting");
        setChatState(false);
        removeFriendButton_->setEnabled(false);
        chatList_->clear();
        return;
    }
    
    QString username = item->data(Qt::UserRole).toString();
    currentChatFriend_ = username;
    chatHeaderLabel_->setText(QString("Chat with %1").arg(username));
    removeFriendButton_->setEnabled(true);
    setChatState(true);
    loadConversation(username);
}

void FriendsScreen::onRequestSelectionChanged()
{
    QListWidgetItem* item = requestsList_->currentItem();
    bool hasSelection = (item != nullptr);
    acceptRequestButton_->setEnabled(hasSelection);
    rejectRequestButton_->setEnabled(hasSelection);
}

void FriendsScreen::onAcceptRequest()
{
    QListWidgetItem* item = requestsList_->currentItem();
    if (!item) return;
    QString username = item->data(Qt::UserRole).toString();
    
    if (demoMode_) {
        requestsList_->takeItem(requestsList_->currentRow());
        loadFriends();
        return;
    }
    
    if (!protocol_) return;
    
    int code = protocol_->acceptFriend(username.toStdString());
    if (code == 200) {
        loadFriendRequests();
        loadFriends();
    }
}

void FriendsScreen::onRejectRequest()
{
    QListWidgetItem* item = requestsList_->currentItem();
    if (!item) return;
    QString username = item->data(Qt::UserRole).toString();
    
    if (demoMode_) {
        requestsList_->takeItem(requestsList_->currentRow());
        return;
    }
    
    if (!protocol_) return;
    
    int code = protocol_->declineFriend(username.toStdString());
    if (code == 200) {
        loadFriendRequests();
    }
}

void FriendsScreen::onRemoveFriend()
{
    QListWidgetItem* item = friendsList_->currentItem();
    if (!item) return;
    QString username = item->data(Qt::UserRole).toString();
    
    if (demoMode_) {
        delete friendsList_->takeItem(friendsList_->currentRow());
        setChatState(false);
        chatHeaderLabel_->setText("Select a friend to start chatting");
        chatList_->clear();
        return;
    }
    
    if (!protocol_) return;
    
    int code = protocol_->deleteFriend(username.toStdString());
    if (code == 200) {
        loadFriends();
        setChatState(false);
        chatHeaderLabel_->setText("Select a friend to start chatting");
        chatList_->clear();
    }
}

void FriendsScreen::onSendMessage()
{
    QString text = messageInput_->text().trimmed();
    if (text.isEmpty() || currentChatFriend_.isEmpty()) {
        return;
    }
    
    if (demoMode_) {
        appendChatMessage("You", text);
        messageInput_->clear();
        return;
    }
    
    if (!protocol_) return;
    
    int code = protocol_->sendChat(currentChatFriend_.toStdString(), text.toStdString());
    if (code == 200) {
        appendChatMessage("You", text);
        messageInput_->clear();
        chatList_->scrollToBottom();
    }
}

void FriendsScreen::handleFriendRequestNotification(const std::string& data)
{
    Q_UNUSED(data);
    if (isVisible() && !demoMode_) {
        loadFriendRequests();
    }
}

void FriendsScreen::handleFriendAcceptedNotification(const std::string& data)
{
    Q_UNUSED(data);
    if (isVisible() && !demoMode_) {
        loadFriends();
    }
}

void FriendsScreen::handleNewMessageNotification(const std::string& data)
{
    if (demoMode_ || !protocol_) return;
    
    // Parse notification data: {"from":"username","to":"username","content":"...","timestamp":...}
    std::string from = MillionaireGame::JsonUtils::extractString(data, "from");
    std::string content = MillionaireGame::JsonUtils::extractString(data, "content");
    long long timestamp = MillionaireGame::JsonUtils::extractLongLong(data, "timestamp", 0);
    
    if (from.empty()) return;
    
    QString fromQStr = QString::fromStdString(from);
    
    if (isVisible()) {
        // If we're currently chatting with this friend, append the message
        if (currentChatFriend_ == fromQStr) {
            appendChatMessage(fromQStr, QString::fromStdString(content), timestamp);
            chatList_->scrollToBottom();
            // Clear pending indicator since we're viewing the conversation
            pendingMessageUsers_.remove(fromQStr);
        } else {
            // Mark friend as having new messages (could add visual indicator)
            pendingMessageUsers_.insert(fromQStr);
            // Update friends list to show indicator
            loadFriends();
        }
    } else {
        // Screen not visible, just mark for later
        pendingMessageUsers_.insert(fromQStr);
    }
}


#include "landing_page.h"
#include "network_thread.h"
#include "instruction_dialog.h"
#include <QApplication>
#include <QFont>
#include <QIcon>

namespace MillionaireGame {

LandingPage::LandingPage(NetworkThread* networkThread, const QString& username,
                         const QString& role, QWidget* parent)
    : QWidget(parent), network_thread_(networkThread), username_(username), role_(role),
      instruction_dialog_(nullptr) {
    setupUI();
    setupUserMenu();
    
    logout_timer_ = new QTimer(this);
    logout_timer_->setSingleShot(true);
    connect(logout_timer_, &QTimer::timeout, this, &LandingPage::onLogoutTimerTimeout);
}

LandingPage::~LandingPage() {
    if (instruction_dialog_) {
        delete instruction_dialog_;
    }
}

void LandingPage::setupUI() {
    setWindowTitle("Who Wants to Be a Millionaire - Main Menu");
    resize(1000, 700);
    
    // Set dark theme with game colors
    setStyleSheet(
        "QWidget { background-color: #1a1a2e; color: #eee; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 10px; padding: 20px; font-size: 20px; font-weight: bold; "
        "min-width: 250px; min-height: 60px; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        "QPushButton:pressed { background-color: #e94560; }"
        "QLabel { color: #eee; }"
    );
    
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(20);
    main_layout->setContentsMargins(30, 30, 30, 30);
    
    // Top bar with welcome message and logout
    QHBoxLayout* top_layout = new QHBoxLayout();
    top_layout->setContentsMargins(0, 0, 0, 0);
    
    welcome_label_ = new QLabel("Welcome, " + username_, this);
    QFont welcome_font = welcome_label_->font();
    welcome_font.setPointSize(16);
    welcome_font.setBold(true);
    welcome_label_->setFont(welcome_font);
    welcome_label_->setStyleSheet("color: #e94560;");
    top_layout->addWidget(welcome_label_);
    
    top_layout->addStretch();
    
    logout_button_ = new QPushButton("⚙", this);
    logout_button_->setMaximumSize(40, 40);
    logout_button_->setStyleSheet(
        "QPushButton { font-size: 20px; padding: 5px; border-radius: 20px; }"
    );
    connect(logout_button_, &QPushButton::clicked, this, &LandingPage::onLogoutMenuTriggered);
    top_layout->addWidget(logout_button_);
    
    main_layout->addLayout(top_layout);
    
    main_layout->addStretch();
    
    // Logo/Title
    QLabel* logo_label = new QLabel("WHO WANTS TO BE A MILLIONAIRE", this);
    logo_label->setAlignment(Qt::AlignCenter);
    QFont logo_font = logo_label->font();
    logo_font.setPointSize(36);
    logo_font.setBold(true);
    logo_label->setFont(logo_font);
    logo_label->setStyleSheet("color: #e94560; margin: 20px;");
    main_layout->addWidget(logo_label);
    
    main_layout->addStretch();
    
    // Menu buttons
    play_button_ = new QPushButton("PLAY", this);
    connect(play_button_, &QPushButton::clicked, this, &LandingPage::onPlayButtonClicked);
    main_layout->addWidget(play_button_);
    
    instruction_button_ = new QPushButton("INSTRUCTION", this);
    connect(instruction_button_, &QPushButton::clicked, this, &LandingPage::onInstructionButtonClicked);
    main_layout->addWidget(instruction_button_);
    
    leaderboard_button_ = new QPushButton("LEADERBOARD", this);
    connect(leaderboard_button_, &QPushButton::clicked, this, &LandingPage::onLeaderboardButtonClicked);
    main_layout->addWidget(leaderboard_button_);
    
    friend_button_ = new QPushButton("FRIEND", this);
    connect(friend_button_, &QPushButton::clicked, this, &LandingPage::onFriendButtonClicked);
    main_layout->addWidget(friend_button_);
    
    quit_button_ = new QPushButton("QUIT", this);
    connect(quit_button_, &QPushButton::clicked, this, &LandingPage::onQuitButtonClicked);
    main_layout->addWidget(quit_button_);
    
    main_layout->addStretch();
}

void LandingPage::setupUserMenu() {
    user_menu_ = new QMenu(this);
    user_menu_->setStyleSheet(
        "QMenu { background-color: #16213e; border: 2px solid #0f3460; }"
        "QMenu::item { padding: 10px 30px; }"
        "QMenu::item:selected { background-color: #0f3460; }"
    );
    
    QAction* logout_action = user_menu_->addAction("Logout");
    connect(logout_action, &QAction::triggered, this, &LandingPage::onLogoutMenuTriggered);
}

void LandingPage::onPlayButtonClicked() {
    emit playRequested();
}

void LandingPage::onInstructionButtonClicked() {
    if (!instruction_dialog_) {
        instruction_dialog_ = new InstructionDialog(this);
    }
    instruction_dialog_->exec();
}

void LandingPage::onLeaderboardButtonClicked() {
    emit leaderboardRequested();
}

void LandingPage::onFriendButtonClicked() {
    emit friendRequested();
}

void LandingPage::onQuitButtonClicked() {
    emit quitRequested();
}

void LandingPage::onLogoutMenuTriggered() {
    // Show logout confirmation
    welcome_label_->setText("Đang đăng xuất...");
    welcome_label_->setStyleSheet("color: #ffa500; font-size: 16px;");
    
    // Wait 2 seconds then logout
    logout_timer_->start(2000);
}

void LandingPage::onLogoutTimerTimeout() {
    emit logoutRequested();
}

} // namespace MillionaireGame


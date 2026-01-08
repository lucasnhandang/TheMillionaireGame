#include "homescreen.h"
#include "protocol_handler.h"
#include "qt_texture_loader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>

HomeScreen::HomeScreen(QWidget *parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , demoMode_(false)
{
    setupUI();
    loadLogo();
}

void HomeScreen::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(50, 50, 50, 50);
    mainLayout->setSpacing(50);
    
    // Left side - Logo
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setAlignment(Qt::AlignCenter);
    
    logoLabel_ = new QLabel(this);
    logoLabel_->setAlignment(Qt::AlignCenter);
    logoLabel_->setStyleSheet("background-color: transparent;");
    leftLayout->addWidget(logoLabel_);
    
    mainLayout->addWidget(leftPanel, 2);
    
    // Right side - Menu buttons
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setAlignment(Qt::AlignCenter);
    rightLayout->setSpacing(20);
    
    usernameLabel_ = new QLabel("Welcome!", this);
    usernameLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: white; margin-bottom: 30px;");
    usernameLabel_->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(usernameLabel_);
    
    playGameButton_ = new QPushButton("Play Game", this);
    playGameButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
    );
    connect(playGameButton_, &QPushButton::clicked, this, &HomeScreen::onPlayGameClicked);
    rightLayout->addWidget(playGameButton_);
    
    resumeGameButton_ = new QPushButton("Resume Game", this);
    resumeGameButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45A049;"
        "}"
    );
    connect(resumeGameButton_, &QPushButton::clicked, this, &HomeScreen::onResumeGameClicked);
    rightLayout->addWidget(resumeGameButton_);
    
    leaderboardButton_ = new QPushButton("Leaderboard", this);
    leaderboardButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF9800;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #F57C00;"
        "}"
    );
    connect(leaderboardButton_, &QPushButton::clicked, this, &HomeScreen::leaderboardClicked);
    rightLayout->addWidget(leaderboardButton_);
    
    settingsButton_ = new QPushButton("Settings", this);
    settingsButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #9E9E9E;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #757575;"
        "}"
    );
    connect(settingsButton_, &QPushButton::clicked, this, &HomeScreen::settingsClicked);
    rightLayout->addWidget(settingsButton_);
    
    instructionsButton_ = new QPushButton("Instructions", this);
    instructionsButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #9E9E9E;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #757575;"
        "}"
    );
    connect(instructionsButton_, &QPushButton::clicked, this, &HomeScreen::instructionsClicked);
    rightLayout->addWidget(instructionsButton_);
    
    friendsButton_ = new QPushButton("Friends", this);
    friendsButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #9E9E9E;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 80px;"
        "  border-radius: 10px;"
        "  border: none;"
        "  min-width: 280px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #757575;"
        "}"
    );
    connect(friendsButton_, &QPushButton::clicked, this, &HomeScreen::friendsClicked);
    rightLayout->addWidget(friendsButton_);
    
    errorLabel_ = new QLabel(this);
    errorLabel_->setStyleSheet("color: #F44336; font-size: 14px;");
    errorLabel_->setAlignment(Qt::AlignCenter);
    errorLabel_->setWordWrap(true);
    errorLabel_->setVisible(false);
    rightLayout->addWidget(errorLabel_);
    
    rightPanel->setLayout(rightLayout);
    mainLayout->addWidget(rightPanel, 1);
    
    setStyleSheet("background-color: #0D1B2A; color: white;");
}

void HomeScreen::loadLogo()
{
    QStringList candidates = {
        "client/assets/millionaire_logo.png",
        "assets/millionaire_logo.png",
        "millionaire_logo.png"
    };
    
    std::vector<QString> paths;
    for (const QString& path : candidates) {
        paths.push_back(path);
    }
    
    logoPixmap_ = LoadPixmapFromAny(paths);
    if (!logoPixmap_.isNull()) {
        logoPixmap_ = logoPixmap_.scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        logoLabel_->setPixmap(logoPixmap_);
    } else {
        logoLabel_->setText("MILLIONAIRE");
        logoLabel_->setStyleSheet("font-size: 64px; font-weight: bold; color: white;");
    }
}

void HomeScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void HomeScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

void HomeScreen::setUsername(const QString& username)
{
    usernameLabel_->setText(QString("Welcome, %1!").arg(username));
}

void HomeScreen::showError(const QString& message)
{
    errorLabel_->setText(message);
    errorLabel_->setVisible(true);
}

void HomeScreen::onPlayGameClicked()
{
    if (demoMode_) {
        emit playGameClicked();
        return;
    }
    
    if (!protocol_) {
        showError("Not connected to server");
        return;
    }
    
    int code = protocol_->startGame(false);
    if (code == 200) {
        emit playGameClicked();
    } else if (code == 412) {
        showError("You have a saved game. Use Resume or override.");
    } else {
        showError(QString("Failed to start game (code %1)").arg(code));
    }
}

void HomeScreen::onResumeGameClicked()
{
    if (demoMode_) {
        showError("No saved game in demo mode");
        return;
    }
    
    if (!protocol_) {
        showError("Not connected to server");
        return;
    }
    
    int code = protocol_->resumeGame();
    if (code == 404) {
        showError("No saved game found");
    } else if (code == 200) {
        emit playGameClicked();
    } else {
        showError("Failed to resume game");
    }
}

#include "mainwindow.h"
#include "loginscreen.h"
#include "homescreen.h"
#include "gamescreen.h"
#include "resultscreen.h"
#include "adminpanelscreen.h"
#include "friendsscreen.h"
#include "protocol_handler.h"
#include "game_event.h"
#include "gamestate.h"
#include "json_utils.h"
#include "socket_client.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QStringList>
#include <iostream>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , stackedWidget_(nullptr)
    , loginScreen_(nullptr)
    , homeScreen_(nullptr)
    , gameScreen_(nullptr)
    , resultScreen_(nullptr)
    , adminPanelScreen_(nullptr)
    , protocol_(nullptr)
    , demoMode_(false)
    , userRole_("user")
    , eventProcessTimer_(nullptr)
{
    gameState_ = std::make_unique<GameState>();
    setupUI();
    setupConnections();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Who Wants to be a Millionaire");
    setMinimumSize(1280, 800);
    resize(1280, 800);
    
    setStyleSheet("background-color: #0D1B2A;");
    
    stackedWidget_ = new QStackedWidget(this);
    setCentralWidget(stackedWidget_);
    
    // Create screens
    loginScreen_ = new LoginScreen(this);
    homeScreen_ = new HomeScreen(this);
    gameScreen_ = new GameScreen(this);
    resultScreen_ = new ResultScreen(this);
    adminPanelScreen_ = new AdminPanelScreen(this);
    friendsScreen_ = new FriendsScreen(this);
    
    // Add screens to stacked widget
    stackedWidget_->addWidget(loginScreen_);
    stackedWidget_->addWidget(homeScreen_);
    stackedWidget_->addWidget(gameScreen_);
    stackedWidget_->addWidget(resultScreen_);
    stackedWidget_->addWidget(adminPanelScreen_);
    stackedWidget_->addWidget(friendsScreen_);
    
    // Show login screen initially
    stackedWidget_->setCurrentWidget(loginScreen_);
    
    // Setup event processing timer
    eventProcessTimer_ = new QTimer(this);
    eventProcessTimer_->setInterval(100); // Check every 100ms
    connect(eventProcessTimer_, &QTimer::timeout, this, &MainWindow::processGameEvents);
}

void MainWindow::setupConnections()
{
    // Login screen
    connect(loginScreen_, &LoginScreen::loginSuccess, this, &MainWindow::onLoginSuccess);
    
    // Home screen
    connect(homeScreen_, &HomeScreen::playGameClicked, this, &MainWindow::onGameStart);
    connect(homeScreen_, &HomeScreen::adminPanelClicked, this, &MainWindow::onAdminPanelClicked);
    connect(homeScreen_, &HomeScreen::friendsClicked, this, &MainWindow::onFriendsClicked);
    
    // Game screen
    connect(gameScreen_, &GameScreen::answerSubmitted, this, [this](int answerIndex) {
        if (!protocol_ || demoMode_) {
            // Demo mode handling
            return;
        }
        
        ProtocolHandler::AnswerResponse response = protocol_->answerQuestion(answerIndex);
        if (response.responseCode == 200) {
            if (response.gameOver) {
                onGameEnd();
            } else if (response.correct) {
                // Wait for next question
            }
        }
    });
    
    connect(gameScreen_, &GameScreen::lifelineUsed, this, [this](const QString& lifelineType) {
        // Lifeline handling is done in processGameEvents
    });
    
    connect(gameScreen_, &GameScreen::walkAwayClicked, this, &MainWindow::onGameEnd);
    connect(gameScreen_, &GameScreen::gameEnded, this, &MainWindow::onGameEnd);
    
    // Result screen
    connect(resultScreen_, &ResultScreen::backToHomeClicked, this, &MainWindow::onBackToHome);
    
    // Admin panel screen
    connect(adminPanelScreen_, &AdminPanelScreen::backToHome, this, &MainWindow::onBackToHome);
    connect(friendsScreen_, &FriendsScreen::backToHome, this, &MainWindow::onBackToHome);
}

void MainWindow::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
    loginScreen_->setProtocolHandler(protocol);
    homeScreen_->setProtocolHandler(protocol);
    gameScreen_->setProtocolHandler(protocol);
    adminPanelScreen_->setProtocolHandler(protocol);
    friendsScreen_->setProtocolHandler(protocol);
    
    setupNotificationHandler();
}

void MainWindow::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
    loginScreen_->setDemoMode(demoMode);
    homeScreen_->setDemoMode(demoMode);
    gameScreen_->setDemoMode(demoMode);
    adminPanelScreen_->setDemoMode(demoMode);
    friendsScreen_->setDemoMode(demoMode);
}

void MainWindow::setupNotificationHandler()
{
    if (!protocol_ || demoMode_) {
        return;
    }
    
    SocketClient* client = protocol_->getClient();
    if (!client) {
        return;
    }
    
    // Start notification handler thread
    std::thread* notificationThread = new std::thread([this, client]() {
        GameEventQueue* eventQueue = protocol_->getEventQueue();
        if (!eventQueue) {
            return;
        }
        
        SocketClient::Message msg;
        while (true) {
            if (client->getMessage(msg, 100)) {
                // Skip RESPONSE messages that have responseCode
                if (msg.type == "RESPONSE" && msg.data.find("\"responseCode\"") != std::string::npos) {
                    client->putMessageBack(msg);
                    continue;
                }
                
                // Convert message types to events
                if (msg.type == "GAME_START") {
                    eventQueue->push(GameEvent(EVENT_GAME_START, msg.data));
                } else if (msg.type == "QUESTION_INFO") {
                    eventQueue->push(GameEvent(EVENT_QUESTION_INFO, msg.data));
                } else if (msg.type == "GAME_END") {
                    eventQueue->push(GameEvent(EVENT_GAME_END, msg.data));
                } else if (msg.type == "LIFELINE_INFO") {
                    eventQueue->push(GameEvent(EVENT_LIFELINE_INFO, msg.data));
                } else if (msg.type == "FRIEND_REQUEST") {
                    eventQueue->push(GameEvent(EVENT_FRIEND_REQUEST, msg.data));
                } else if (msg.type == "FRIEND_REQUEST_ACCEPTED") {
                    eventQueue->push(GameEvent(EVENT_FRIEND_ACCEPTED, msg.data));
                } else if (msg.type == "NEW_MESSAGE") {
                    eventQueue->push(GameEvent(EVENT_NEW_MESSAGE, msg.data));
                }
            }
        }
    });
    notificationThread->detach();
    
    // Start event processing timer
    eventProcessTimer_->start();
}

void MainWindow::processGameEvents()
{
    if (!protocol_ || demoMode_) {
        return;
    }
    
    GameEventQueue* eventQueue = protocol_->getEventQueue();
    if (!eventQueue) {
        return;
    }
    
    GameEvent event;
    while (eventQueue->pop(event)) {
        std::cerr << "[DEBUG] Processing event type: " << event.type << std::endl;
        
        switch (event.type) {
            case EVENT_GAME_START: {
                int gameId = MillionaireGame::JsonUtils::extractInt(event.data, "gameId", 0);
                if (gameId > 0) {
                    protocol_->currentGameId = gameId;
                }
                gameState_->inGame = true;
                gameState_->onHome = false;
                gameState_->waitingForQuestion = true;
                gameState_->availableLifelines = {true, true, true};
                gameState_->totalScore = 0;
                gameState_->currentQuestionNumber = 0;
                onGameStart();
                break;
            }
            
            case EVENT_QUESTION_INFO: {
                std::string question = MillionaireGame::JsonUtils::extractString(event.data, "question");
                int questionNumber = MillionaireGame::JsonUtils::extractInt(event.data, "questionNumber", 0);
                int timeRemaining = MillionaireGame::JsonUtils::extractInt(event.data, "timeRemaining", 30);
                int prize = MillionaireGame::JsonUtils::extractInt(event.data, "prize", 0);
                int totalScore = MillionaireGame::JsonUtils::extractInt(event.data, "totalScore", 0);
                
                // Parse options
                QStringList options;
                // Simple JSON parsing for options array
                size_t optionsStart = event.data.find("\"options\"");
                if (optionsStart != std::string::npos) {
                    size_t arrayStart = event.data.find("[", optionsStart);
                    if (arrayStart != std::string::npos) {
                        size_t arrayEnd = event.data.find("]", arrayStart);
                        if (arrayEnd != std::string::npos) {
                            std::string optionsArray = event.data.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                            size_t pos = 0;
                            while (pos < optionsArray.length()) {
                                size_t textStart = optionsArray.find("\"text\"", pos);
                                if (textStart == std::string::npos) break;
                                
                                size_t colonPos = optionsArray.find(":", textStart);
                                if (colonPos == std::string::npos) break;
                                
                                size_t quoteStart = optionsArray.find("\"", colonPos);
                                if (quoteStart == std::string::npos) break;
                                
                                size_t quoteEnd = optionsArray.find("\"", quoteStart + 1);
                                if (quoteEnd == std::string::npos) break;
                                
                                std::string text = optionsArray.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                                options.append(QString::fromStdString(text));
                                
                                pos = quoteEnd + 1;
                            }
                        }
                    }
                }
                
                gameState_->question = question;
                gameState_->currentQuestionNumber = questionNumber;
                gameState_->timeRemaining = 30;
                gameState_->currentPrize = prize;
                gameState_->totalScore = totalScore;
                gameState_->waitingForQuestion = false;
                gameState_->revealActive = true;
                gameState_->answersRevealed = 0;
                
                protocol_->currentQuestionNumber = questionNumber;
                
                // Update game screen
                gameScreen_->updateQuestion(QString::fromStdString(question), options, questionNumber);
                gameScreen_->updatePrize(prize);
                gameScreen_->updateTimer(30);
                gameScreen_->resetForNewQuestion();
                
                break;
            }
            
            case EVENT_GAME_END: {
                std::string status = MillionaireGame::JsonUtils::extractString(event.data, "status");
                long long finalPrize = MillionaireGame::JsonUtils::extractInt(event.data, "finalPrize", 0);
                int totalScore = MillionaireGame::JsonUtils::extractInt(event.data, "totalScore", 0);
                bool isWinner = MillionaireGame::JsonUtils::extractBool(event.data, "isWinner", false);
                
                gameState_->finalPrize = finalPrize;
                gameState_->totalScore = totalScore;
                gameState_->inGame = false;
                
                onGameEnd();
                resultScreen_->showResult(finalPrize, totalScore, isWinner);
                onShowResult();
                break;
            }
            
            case EVENT_LIFELINE_INFO: {
                std::string lifelineType = MillionaireGame::JsonUtils::extractString(event.data, "lifelineType");
                int timeRemaining = MillionaireGame::JsonUtils::extractInt(event.data, "timeRemaining", gameState_->timeRemaining);
                
                if (lifelineType == "5050") {
                    gameState_->availableLifelines[0] = false;
                    // Parse remainingOptions
                    QList<int> remaining;
                    size_t pos = event.data.find("\"remainingOptions\"");
                    if (pos != std::string::npos) {
                        pos = event.data.find('[', pos);
                        if (pos != std::string::npos) {
                            size_t end = event.data.find(']', pos);
                            if (end != std::string::npos) {
                                std::string arrayStr = event.data.substr(pos + 1, end - pos - 1);
                                std::stringstream ss(arrayStr);
                                std::string token;
                                while (std::getline(ss, token, ',')) {
                                    token.erase(0, token.find_first_not_of(" \t"));
                                    token.erase(token.find_last_not_of(" \t") + 1);
                                    if (!token.empty()) {
                                        remaining.append(std::stoi(token));
                                    }
                                }
                            }
                        }
                    }
                    gameScreen_->updateLifeline5050(remaining);
                } else if (lifelineType == "PHONE") {
                    gameState_->availableLifelines[1] = false;
                    std::string suggestion = MillionaireGame::JsonUtils::extractString(event.data, "suggestion");
                    gameScreen_->updateLifelinePhone(QString::fromStdString(suggestion));
                } else if (lifelineType == "AUDIENCE") {
                    gameState_->availableLifelines[2] = false;
                    QMap<QChar, int> poll;
                    // Parse poll object
                    size_t pos = event.data.find("\"poll\"");
                    if (pos != std::string::npos) {
                        pos = event.data.find('{', pos);
                        if (pos != std::string::npos) {
                            int braceCount = 0;
                            size_t end = pos;
                            for (size_t i = pos; i < event.data.length(); i++) {
                                if (event.data[i] == '{') braceCount++;
                                if (event.data[i] == '}') {
                                    braceCount--;
                                    if (braceCount == 0) {
                                        end = i;
                                        break;
                                    }
                                }
                            }
                            if (end > pos) {
                                std::string pollStr = event.data.substr(pos + 1, end - pos - 1);
                                std::stringstream ss(pollStr);
                                std::string token;
                                while (std::getline(ss, token, ',')) {
                                    size_t colonPos = token.find(':');
                                    if (colonPos != std::string::npos) {
                                        std::string key = token.substr(0, colonPos);
                                        key.erase(0, key.find_first_not_of(" \t\""));
                                        key.erase(key.find_last_not_of(" \t\"") + 1);
                                        std::string value = token.substr(colonPos + 1);
                                        value.erase(0, value.find_first_not_of(" \t"));
                                        value.erase(value.find_last_not_of(" \t") + 1);
                                        if (key.length() == 1 && key[0] >= 'A' && key[0] <= 'D') {
                                            poll[key[0]] = std::stoi(value);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    gameScreen_->updateLifelineAudience(poll);
                }
                
                gameState_->timeRemaining = timeRemaining;
                gameScreen_->updateTimer(timeRemaining);
                gameScreen_->hideLifelineLoading();
                
                break;
            }
            
            case EVENT_FRIEND_REQUEST: {
                if (friendsScreen_) {
                    friendsScreen_->handleFriendRequestNotification(event.data);
                }
                break;
            }
            
            case EVENT_FRIEND_ACCEPTED: {
                if (friendsScreen_) {
                    friendsScreen_->handleFriendAcceptedNotification(event.data);
                }
                break;
            }
            
            case EVENT_NEW_MESSAGE: {
                if (friendsScreen_) {
                    friendsScreen_->handleNewMessageNotification(event.data);
                }
                break;
            }
            
            default:
                break;
        }
    }
}

void MainWindow::onLoginSuccess(const QString& username, const QString& role)
{
    std::cerr << "[DEBUG] onLoginSuccess called - username: " << username.toStdString() 
              << ", role: " << role.toStdString() << std::endl;
    
    stackedWidget_->setCurrentWidget(homeScreen_);
    homeScreen_->setUsername(username);
    homeScreen_->setUserRole(role);
    
    // Store username and role
    gameState_->username = username.toStdString();
    userRole_ = role;
    gameState_->loggedIn = true;
    gameState_->onHome = true;
}

void MainWindow::onGameStart()
{
    stackedWidget_->setCurrentWidget(gameScreen_);
    gameState_->inGame = true;
    gameState_->onHome = false;
    gameScreen_->setGameState(gameState_.get());
    // Reset lifelines when starting a new game session
    gameScreen_->resetLifelines();
}

void MainWindow::onGameEnd()
{
    gameState_->inGame = false;
}

void MainWindow::onShowResult()
{
    stackedWidget_->setCurrentWidget(resultScreen_);
}

void MainWindow::onBackToHome()
{
    stackedWidget_->setCurrentWidget(homeScreen_);
    gameState_->onHome = true;
    gameState_->inGame = false;
    // Reset lifelines when returning to home (prepare for next game)
    gameScreen_->resetLifelines();
}

void MainWindow::onAdminPanelClicked()
{
    stackedWidget_->setCurrentWidget(adminPanelScreen_);
    // Load users tab by default
    adminPanelScreen_->loadUsers(1);
}

void MainWindow::onFriendsClicked()
{
    stackedWidget_->setCurrentWidget(friendsScreen_);
    friendsScreen_->refreshData();
}

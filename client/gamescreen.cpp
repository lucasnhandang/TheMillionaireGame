#include "gamescreen.h"
#include "ui_gamescreen.h"  // Generated from gamescreen.ui
#include "protocol_handler.h"
#include "game_event.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QPainter>
#include <QFont>
#include <QMap>
#include <QList>
#include <QChar>
#include <cmath>

GameScreen::GameScreen(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GameScreen)
    , protocol_(nullptr)
    , gameState_(nullptr)
    , demoMode_(false)
    , currentQuestionNumber_(0)
    , selectedAnswer_(-1)
    , timeRemaining_(30)
    , timerRunning_(false)
    , answersRevealed_(0)
    , totalScore_(0)
    , lifeline5050Available_(true)
    , lifelinePhoneAvailable_(true)
    , lifelineAudienceAvailable_(true)
    , lifelineProcessing_(false)
{
    ui->setupUi(this);  // Load UI from .ui file
    setupUI();
}

GameScreen::~GameScreen()
{
    delete ui;
}

void GameScreen::setupUI()
{
    // Get widget pointers from UI
    // Prize labels (for highlighting)
    prizeLabel1_ = findChild<QLabel*>("prizeLabel1");
    prizeLabel2_ = findChild<QLabel*>("prizeLabel2");
    prizeLabel3_ = findChild<QLabel*>("prizeLabel3");
    prizeLabel4_ = findChild<QLabel*>("prizeLabel4");
    prizeLabel5_ = findChild<QLabel*>("prizeLabel5");
    prizeLabel6_ = findChild<QLabel*>("prizeLabel6");
    prizeLabel7_ = findChild<QLabel*>("prizeLabel7");
    prizeLabel8_ = findChild<QLabel*>("prizeLabel8");
    prizeLabel9_ = findChild<QLabel*>("prizeLabel9");
    prizeLabel10_ = findChild<QLabel*>("prizeLabel10");
    prizeLabel11_ = findChild<QLabel*>("prizeLabel11");
    prizeLabel12_ = findChild<QLabel*>("prizeLabel12");
    prizeLabel13_ = findChild<QLabel*>("prizeLabel13");
    prizeLabel14_ = findChild<QLabel*>("prizeLabel14");
    prizeLabel15_ = findChild<QLabel*>("prizeLabel15");
    
    // Store in list for easy access
    prizeLabels_ = {
        prizeLabel1_, prizeLabel2_, prizeLabel3_, prizeLabel4_, prizeLabel5_,
        prizeLabel6_, prizeLabel7_, prizeLabel8_, prizeLabel9_, prizeLabel10_,
        prizeLabel11_, prizeLabel12_, prizeLabel13_, prizeLabel14_, prizeLabel15_
    };
    
    // Main widgets
    questionLabel_ = ui->questionLabel;
    timerLabel_ = ui->timerLabel;
    scoreLabel_ = ui->scoreLabel;  // BONUS: Score display
    
    // Buttons
    walkAwayButton_ = ui->walkAwayButton;
    answerButtonA_ = ui->answerButtonA;
    answerButtonB_ = ui->answerButtonB;
    answerButtonC_ = ui->answerButtonC;
    answerButtonD_ = ui->answerButtonD;
    answerButtons_ = {answerButtonA_, answerButtonB_, answerButtonC_, answerButtonD_};
    
    submitButton_ = ui->submitButton;
    
    // Lifelines - note: lifelinAskButton in UI maps to lifelineAudienceButton_
    lifeline5050Button_ = ui->lifeline5050Button;
    lifelinePhoneButton_ = ui->lifelinePhoneButton;
    lifelineAudienceButton_ = findChild<QPushButton*>("lifelinAskButton");  // Map UI name to code
    
    // Result labels
    lifelineResultLabel_ = ui->lifelineResultLabel;
    audiencePollWidget_ = ui->audiencePollWidget;
    
    // Setup connections
    setupConnections();
    
    // Setup initial visibility
    setupInitialVisibility();
    
    // Timers
    countdownTimer_ = new QTimer(this);
    countdownTimer_->setInterval(1000);
    connect(countdownTimer_, &QTimer::timeout, this, &GameScreen::onTimerTimeout);
    
    revealTimer_ = new QTimer(this);
    revealTimer_->setInterval(500);
    connect(revealTimer_, &QTimer::timeout, this, &GameScreen::onRevealTimeout);
    
    // Initialize score display
    if (scoreLabel_) {
        scoreLabel_->setText("Score: 0");
    }
}

void GameScreen::setupConnections()
{
    // Connect button signals
    connect(walkAwayButton_, &QPushButton::clicked, this, &GameScreen::onWalkAwayClicked);
    connect(submitButton_, &QPushButton::clicked, this, &GameScreen::onAnswerButtonClicked);
    
    // Connect lifeline buttons
    if (lifeline5050Button_) {
        connect(lifeline5050Button_, &QPushButton::clicked, this, &GameScreen::onLifeline5050Clicked);
    }
    if (lifelinePhoneButton_) {
        connect(lifelinePhoneButton_, &QPushButton::clicked, this, &GameScreen::onLifelinePhoneClicked);
    }
    if (lifelineAudienceButton_) {
        connect(lifelineAudienceButton_, &QPushButton::clicked, this, &GameScreen::onLifelineAudienceClicked);
    }
    
    // Connect answer buttons
    for (int i = 0; i < 4; i++) {
        if (answerButtons_[i]) {
            connect(answerButtons_[i], &QPushButton::clicked, this, [this, i]() {
                selectedAnswer_ = i;
                updateAnswerButtons();
            });
        }
    }
}

void GameScreen::setupInitialVisibility()
{
    // Hide answer buttons initially
    for (auto* btn : answerButtons_) {
        if (btn) {
            btn->hide();
            btn->setEnabled(false);
        }
    }
    
    // Hide lifeline result label
    if (lifelineResultLabel_) {
        lifelineResultLabel_->hide();
    }
    
    // Hide audience poll widget
    if (audiencePollWidget_) {
        audiencePollWidget_->hide();
    }
}

void GameScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void GameScreen::setGameState(GameState* gameState)
{
    gameState_ = gameState;
}

void GameScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

void GameScreen::updateQuestion(const QString& question, const QStringList& options, int questionNumber)
{
    currentQuestion_ = question;
    currentOptions_ = options;
    currentQuestionNumber_ = questionNumber;
    selectedAnswer_ = -1;
    
    if (questionLabel_) {
        questionLabel_->setText(question);
    }
    
    // Highlight prize ladder for current question
    highlightPrizeLadder(questionNumber);
    
    // Reset answer buttons
    answersRevealed_ = 0;
    for (int i = 0; i < 4; i++) {
        if (answerButtons_[i] && i < options.size()) {
            answerButtons_[i]->setText(QString("%1. %2").arg(QChar('A' + i)).arg(options[i]));
            answerButtons_[i]->setEnabled(false);
            answerButtons_[i]->hide();
        }
    }
    
    // Start reveal animation
    revealTimer_->start();
}

void GameScreen::updateTimer(int seconds)
{
    timeRemaining_ = seconds;
    if (timerLabel_) {
        timerLabel_->setText(QString::number(seconds));
        
        // Update color based on time
        if (seconds <= 10) {
            timerLabel_->setStyleSheet(
                "background-color: #001844;"
                "color: #F44336;"
                "font-size: 24px;"
                "font-weight: bold;"
                "padding: 10px 20px;"
                "border-radius: 30px;"
                "min-width: 20px;"
            );
        } else if (seconds <= 20) {
            timerLabel_->setStyleSheet(
                "background-color: #001844;"
                "color: #FF9800;"
                "font-size: 24px;"
                "font-weight: bold;"
                "padding: 10px 20px;"
                "border-radius: 30px;"
                "min-width: 20px;"
            );
        } else {
            timerLabel_->setStyleSheet(
                "background-color: #001844;"
                "color: white;"
                "font-size: 24px;"
                "font-weight: bold;"
                "padding: 10px 20px;"
                "border-radius: 30px;"
                "min-width: 20px;"
            );
        }
    }
}

void GameScreen::highlightPrizeLadder(int questionNumber)
{
    // Reset all labels to default style
    for (int i = 0; i < 15; i++) {
        QLabel* label = prizeLabels_[i];
        if (!label) continue;
        
        int questionNum = i + 1;  // Q1 to Q15
        bool isCheckpoint = (questionNum == 5 || questionNum == 10 || questionNum == 15);
        
        if (questionNum == questionNumber) {
            // Highlight current question with gradient (bôi vàng)
            label->setStyleSheet(
                "color: #FFD700;"
                "font-size: 20px;"
                "padding: 3px;"
                "font-weight: bold;"
                "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                "    stop:0 rgba(255, 215, 0, 0.3),"
                "    stop:1 rgba(255, 215, 0, 0.1));"
                "border: 1px solid #FFD700;"
                "border-radius: 5px;"
            );
        } else if (isCheckpoint) {
            // Checkpoint style (vàng mặc định)
            label->setStyleSheet(
                "color: #FFD700;"
                "font-size: 20px;"
                "padding: 3px;"
                "font-weight: bold;"
                "background: transparent;"
            );
        } else {
            // Default style (trắng)
            label->setStyleSheet(
                "color: white;"
                "font-size: 20px;"
                "padding: 3px;"
                "background: transparent;"
            );
        }
    }
}

void GameScreen::updateLifeline5050(const QList<int>& remainingIndices)
{
    for (int i = 0; i < 4; i++) {
        if (answerButtons_[i] && !remainingIndices.contains(i)) {
            answerButtons_[i]->setEnabled(false);
            answerButtons_[i]->setStyleSheet(
                "QPushButton {"
                "  background-color: #333333;"
                "  color: #888888;"
                "  font-size: 16px;"
                "  padding: 15px 20px;"
                "  border-radius: 8px;"
                "  text-align: left;"
                "  min-height: 60px;"
                "}"
            );
        }
    }
    lifeline5050Available_ = false;
    if (lifeline5050Button_) {
        lifeline5050Button_->setEnabled(false);
    }
    if (lifelineResultLabel_) {
        lifelineResultLabel_->setText("50/50: Two wrong answers eliminated!");
        lifelineResultLabel_->show();
    }
}

void GameScreen::updateLifelinePhone(const QString& suggestion)
{
    lifelinePhoneAvailable_ = false;
    if (lifelinePhoneButton_) {
        lifelinePhoneButton_->setEnabled(false);
    }
    if (lifelineResultLabel_) {
        lifelineResultLabel_->setText(QString("Friend says: %1").arg(suggestion));
        lifelineResultLabel_->show();
    }
}

void GameScreen::updateLifelineAudience(const QMap<QChar, int>& poll)
{
    lifelineAudienceAvailable_ = false;
    if (lifelineAudienceButton_) {
        lifelineAudienceButton_->setEnabled(false);
    }
    
    QString pollText = "Audience Poll Results:\n";
    QList<QChar> labels = {'A', 'B', 'C', 'D'};
    for (QChar label : labels) {
        int percent = poll.value(label, 0);
        pollText += QString("%1: %2%\n").arg(label).arg(percent);
    }
    
    if (lifelineResultLabel_) {
        lifelineResultLabel_->setText(pollText);
        lifelineResultLabel_->show();
    }
}

void GameScreen::showLifelineLoading(const QString& message)
{
    lifelineProcessing_ = true;
    if (lifelineResultLabel_) {
        lifelineResultLabel_->setText(message);
        lifelineResultLabel_->show();
    }
}

void GameScreen::hideLifelineLoading()
{
    lifelineProcessing_ = false;
}

void GameScreen::resetForNewQuestion()
{
    if (lifelineResultLabel_) {
        lifelineResultLabel_->hide();
    }
    selectedAnswer_ = -1;
    updateAnswerButtons();
}

void GameScreen::resetLifelines()
{
    // Reset lifeline state
    lifeline5050Available_ = true;
    lifelinePhoneAvailable_ = true;
    lifelineAudienceAvailable_ = true;
    lifelineProcessing_ = false;
    lifelineType_.clear();
    
    // Reset lifeline UI
    if (lifeline5050Button_) {
        lifeline5050Button_->setEnabled(true);
    }
    if (lifelinePhoneButton_) {
        lifelinePhoneButton_->setEnabled(true);
    }
    if (lifelineAudienceButton_) {
        lifelineAudienceButton_->setEnabled(true);
    }
    
    // Reset button styles to active state (if they have icons, don't override)
    // The UI file already has styles, so we just need to enable them
    
    // Hide lifeline result
    if (lifelineResultLabel_) {
        lifelineResultLabel_->hide();
        lifelineResultLabel_->clear();
    }
}

void GameScreen::updateAnswerButtons()
{
    for (int i = 0; i < 4; i++) {
        if (!answerButtons_[i]) continue;
        
        QString style;
        if (selectedAnswer_ == i) {
            style = 
                "QPushButton {"
                "  background-color: #4CAF50;"
                "  color: white;"
                "  font-size: 24px;"
                "  font-weight: bold;"
                "  padding: 15px 30px;"
                "  border-radius: 10px;"
                "  min-height: 20px;"
                "  text-align: left;"
                "  background: transparent;"
                "}"
                "QPushButton:hover {"
                "  background-color: #45A049;"
                "}";
        } else {
            style = 
                "QPushButton {"
                "  background-color: #1E88E5;"
                "  color: white;"
                "  font-size: 24px;"
                "  font-weight: bold;"
                "  padding: 15px 30px;"
                "  border-radius: 10px;"
                "  min-height: 20px;"
                "  text-align: left;"
                "  background: transparent;"
                "}"
                "QPushButton:hover {"
                "  background-color: #1976D2;"
                "}"
                "QPushButton:disabled {"
                "  background-color: #555555;"
                "  color: #888888;"
                "}";
        }
        answerButtons_[i]->setStyleSheet(style);
    }
}

void GameScreen::startTimer()
{
    if (!timerRunning_) {
        timerRunning_ = true;
        countdownTimer_->start();
    }
}

void GameScreen::stopTimer()
{
    timerRunning_ = false;
    countdownTimer_->stop();
}

void GameScreen::onTimerTimeout()
{
    if (timeRemaining_ > 0) {
        timeRemaining_--;
        updateTimer(timeRemaining_);
        
        // BONUS: Add remaining time to score (score increases as time passes)
        // Score = sum of remaining time for each question
        // This is calculated when answer is submitted, but we can also show current potential score
    } else {
        stopTimer();
        emit gameEnded();
    }
}

void GameScreen::updateScore(int score)
{
    totalScore_ = score;
    if (scoreLabel_) {
        scoreLabel_->setText(QString("Score: %1").arg(score));
    }
}

void GameScreen::onRevealTimeout()
{
    if (answersRevealed_ < 4 && answersRevealed_ < currentOptions_.size()) {
        if (answerButtons_[answersRevealed_]) {
            answerButtons_[answersRevealed_]->show();
            answerButtons_[answersRevealed_]->setEnabled(true);
        }
        answersRevealed_++;
        
        if (answersRevealed_ >= 4 || answersRevealed_ >= currentOptions_.size()) {
            revealTimer_->stop();
            startTimer();
        }
    }
}

void GameScreen::onAnswerButtonClicked()
{
    if (selectedAnswer_ >= 0 && selectedAnswer_ < 4) {
        emit answerSubmitted(selectedAnswer_);
    }
}

void GameScreen::onLifeline5050Clicked()
{
    if (demoMode_) {
        // Demo mode
        QList<int> remaining = {0, 2};
        updateLifeline5050(remaining);
    } else if (protocol_) {
        showLifelineLoading("Eliminating 2 wrong answers...");
        protocol_->useLifeline("5050");
    }
}

void GameScreen::onLifelinePhoneClicked()
{
    if (demoMode_) {
        // Demo mode
        updateLifelinePhone("I think it's A!");
    } else if (protocol_) {
        showLifelineLoading("Calling...");
        protocol_->useLifeline("PHONE");
    }
}

void GameScreen::onLifelineAudienceClicked()
{
    if (demoMode_) {
        // Demo mode
        QMap<QChar, int> poll;
        poll['A'] = 65;
        poll['B'] = 15;
        poll['C'] = 10;
        poll['D'] = 10;
        updateLifelineAudience(poll);
    } else if (protocol_) {
        showLifelineLoading("Surveying audience...");
        protocol_->useLifeline("AUDIENCE");
    }
}

void GameScreen::onWalkAwayClicked()
{
    if (demoMode_) {
        emit gameEnded();
    } else if (protocol_) {
        protocol_->giveUp();
        emit gameEnded();
    }
}

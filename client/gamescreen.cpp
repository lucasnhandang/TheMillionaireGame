#include "gamescreen.h"
#include "protocol_handler.h"
#include "game_event.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QProgressBar>
#include <QPainter>
#include <QFont>
#include <QMap>
#include <QList>
#include <QChar>
#include <cmath>

GameScreen::GameScreen(QWidget *parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , gameState_(nullptr)
    , demoMode_(false)
    , currentQuestionNumber_(0)
    , selectedAnswer_(-1)
    , timeRemaining_(30)
    , timerRunning_(false)
    , answersRevealed_(0)
    , lifeline5050Available_(true)
    , lifelinePhoneAvailable_(true)
    , lifelineAudienceAvailable_(true)
    , lifelineProcessing_(false)
{
    setupUI();
}

void GameScreen::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Left panel - Prize ladder
    QWidget* prizePanel = new QWidget(this);
    prizePanel->setFixedWidth(200);
    prizePanel->setStyleSheet("background-color: #1A1A2E; border-radius: 10px; padding: 10px;");
    QVBoxLayout* prizeLayout = new QVBoxLayout(prizePanel);
    
    QLabel* prizeTitle = new QLabel("Prize Ladder", this);
    prizeTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: white;");
    prizeTitle->setAlignment(Qt::AlignCenter);
    prizeLayout->addWidget(prizeTitle);
    
    prizeLadderWidget_ = new QWidget(this);
    prizeLadderWidget_->setStyleSheet("background-color: transparent;");
    QVBoxLayout* ladderLayout = new QVBoxLayout(prizeLadderWidget_);
    ladderLayout->setSpacing(5);
    
    const int PRIZE_LADDER[15] = {
        1000000, 2000000, 3000000, 5000000, 10000000,
        20000000, 30000000, 50000000, 100000000, 200000000,
        300000000, 500000000, 1000000000, 2000000000, 1000000000
    };
    
    for (int i = 14; i >= 0; i--) {
        QLabel* prizeLabel = new QLabel(QString("Q%1: %2 VND").arg(i + 1).arg(PRIZE_LADDER[i]), this);
        bool isCheckpoint = (i == 4 || i == 9 || i == 14);
        QString color = isCheckpoint ? "#FFD700" : "white";
        prizeLabel->setStyleSheet(QString("color: %1; font-size: 12px; padding: 3px;").arg(color));
        ladderLayout->addWidget(prizeLabel);
    }
    
    prizeLayout->addWidget(prizeLadderWidget_);
    mainLayout->addWidget(prizePanel);
    
    // Center panel - Question and answers
    QWidget* questionPanel = new QWidget(this);
    questionPanel->setStyleSheet("background-color: #16213E; border-radius: 10px; padding: 20px;");
    QVBoxLayout* questionLayout = new QVBoxLayout(questionPanel);
    
    // Top bar with lifelines and timer
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    
    walkAwayButton_ = new QPushButton("WALK AWAY", this);
    walkAwayButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #F44336;"
        "  color: white;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 10px 20px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #D32F2F; }"
    );
    connect(walkAwayButton_, &QPushButton::clicked, this, &GameScreen::onWalkAwayClicked);
    topBarLayout->addWidget(walkAwayButton_);
    
    topBarLayout->addStretch();
    
    // Progress bar and question number
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 15);
    progressBar_->setValue(0);
    progressBar_->setStyleSheet(
        "QProgressBar {"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 5px;"
        "  text-align: center;"
        "  height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #1E88E5;"
        "}"
    );
    topBarLayout->addWidget(progressBar_);
    
    questionNumberLabel_ = new QLabel("Q1", this);
    questionNumberLabel_->setStyleSheet(
        "background-color: #1E88E5;"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px 15px;"
        "border-radius: 15px;"
    );
    questionNumberLabel_->setAlignment(Qt::AlignCenter);
    topBarLayout->addWidget(questionNumberLabel_);
    
    topBarLayout->addStretch();
    
    // Timer
    timerLabel_ = new QLabel("30", this);
    timerLabel_->setStyleSheet(
        "background-color: #4CAF50;"
        "color: white;"
        "font-size: 24px;"
        "font-weight: bold;"
        "padding: 10px 20px;"
        "border-radius: 20px;"
        "min-width: 60px;"
    );
    timerLabel_->setAlignment(Qt::AlignCenter);
    topBarLayout->addWidget(timerLabel_);
    
    // Lifelines
    lifeline5050Button_ = new QPushButton("50:50", this);
    lifeline5050Button_->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF9800;"
        "  color: white;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  padding: 8px 15px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:disabled { background-color: #555555; color: #888888; }"
    );
    connect(lifeline5050Button_, &QPushButton::clicked, this, &GameScreen::onLifeline5050Clicked);
    topBarLayout->addWidget(lifeline5050Button_);
    
    lifelinePhoneButton_ = new QPushButton("Phone", this);
    lifelinePhoneButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF9800;"
        "  color: white;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  padding: 8px 15px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:disabled { background-color: #555555; color: #888888; }"
    );
    connect(lifelinePhoneButton_, &QPushButton::clicked, this, &GameScreen::onLifelinePhoneClicked);
    topBarLayout->addWidget(lifelinePhoneButton_);
    
    lifelineAudienceButton_ = new QPushButton("Audience", this);
    lifelineAudienceButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF9800;"
        "  color: white;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  padding: 8px 15px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #F57C00; }"
        "QPushButton:disabled { background-color: #555555; color: #888888; }"
    );
    connect(lifelineAudienceButton_, &QPushButton::clicked, this, &GameScreen::onLifelineAudienceClicked);
    topBarLayout->addWidget(lifelineAudienceButton_);
    
    questionLayout->addLayout(topBarLayout);
    questionLayout->addSpacing(20);
    
    // Question label
    questionLabel_ = new QLabel("Waiting for question...", this);
    questionLabel_->setStyleSheet(
        "background-color: #0F3460;"
        "color: white;"
        "font-size: 20px;"
        "padding: 20px;"
        "border-radius: 10px;"
        "min-height: 100px;"
    );
    questionLabel_->setAlignment(Qt::AlignCenter);
    questionLabel_->setWordWrap(true);
    questionLayout->addWidget(questionLabel_);
    
    questionLayout->addSpacing(20);
    
    // Answer buttons
    answerButtonA_ = new QPushButton("A.", this);
    answerButtonB_ = new QPushButton("B.", this);
    answerButtonC_ = new QPushButton("C.", this);
    answerButtonD_ = new QPushButton("D.", this);
    
    answerButtons_ = {answerButtonA_, answerButtonB_, answerButtonC_, answerButtonD_};
    
    QString answerButtonStyle = 
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 15px 20px;"
        "  border-radius: 8px;"
        "  text-align: left;"
        "  min-height: 60px;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #555555; color: #888888; }";
    
    for (int i = 0; i < 4; i++) {
        answerButtons_[i]->setStyleSheet(answerButtonStyle);
        answerButtons_[i]->setEnabled(false);
        answerButtons_[i]->setVisible(false);
        connect(answerButtons_[i], &QPushButton::clicked, this, [this, i]() {
            selectedAnswer_ = i;
            updateAnswerButtons();
        });
        questionLayout->addWidget(answerButtons_[i]);
    }
    
    questionLayout->addSpacing(20);
    
    // Submit button
    QPushButton* submitButton = new QPushButton("Submit Answer", this);
    submitButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  padding: 15px 50px;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover { background-color: #45A049; }"
        "QPushButton:disabled { background-color: #555555; }"
    );
    connect(submitButton, &QPushButton::clicked, this, &GameScreen::onAnswerButtonClicked);
    questionLayout->addWidget(submitButton, 0, Qt::AlignCenter);
    
    questionLayout->addSpacing(20);
    
    // Lifeline result label
    lifelineResultLabel_ = new QLabel(this);
    lifelineResultLabel_->setStyleSheet("color: #FFD700; font-size: 16px;");
    lifelineResultLabel_->setAlignment(Qt::AlignCenter);
    lifelineResultLabel_->setWordWrap(true);
    lifelineResultLabel_->setVisible(false);
    questionLayout->addWidget(lifelineResultLabel_);
    
    // Prize label at bottom
    prizeLabel_ = new QLabel("0 VND", this);
    prizeLabel_->setStyleSheet(
        "color: #FFD700;"
        "font-size: 24px;"
        "font-weight: bold;"
    );
    prizeLabel_->setAlignment(Qt::AlignCenter);
    questionLayout->addWidget(prizeLabel_);
    
    mainLayout->addWidget(questionPanel, 1);
    
    // Timers
    countdownTimer_ = new QTimer(this);
    countdownTimer_->setInterval(1000);
    connect(countdownTimer_, &QTimer::timeout, this, &GameScreen::onTimerTimeout);
    
    revealTimer_ = new QTimer(this);
    revealTimer_->setInterval(500);
    connect(revealTimer_, &QTimer::timeout, this, &GameScreen::onRevealTimeout);
    
    setStyleSheet("background-color: #0D1B2A; color: white;");
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
    
    questionLabel_->setText(question);
    questionNumberLabel_->setText(QString("Q%1").arg(questionNumber));
    progressBar_->setValue(questionNumber);
    
    // Reset answer buttons
    answersRevealed_ = 0;
    for (int i = 0; i < 4; i++) {
        if (i < options.size()) {
            answerButtons_[i]->setText(QString("%1. %2").arg(QChar('A' + i)).arg(options[i]));
            answerButtons_[i]->setEnabled(false);
            answerButtons_[i]->setVisible(false);
        }
    }
    
    // Start reveal animation
    revealTimer_->start();
}

void GameScreen::updateTimer(int seconds)
{
    timeRemaining_ = seconds;
    timerLabel_->setText(QString::number(seconds));
    
    // Update color based on time
    if (seconds <= 10) {
        timerLabel_->setStyleSheet(
            "background-color: #F44336;"
            "color: white;"
            "font-size: 24px;"
            "font-weight: bold;"
            "padding: 10px 20px;"
            "border-radius: 20px;"
            "min-width: 60px;"
        );
    } else if (seconds <= 20) {
        timerLabel_->setStyleSheet(
            "background-color: #FF9800;"
            "color: white;"
            "font-size: 24px;"
            "font-weight: bold;"
            "padding: 10px 20px;"
            "border-radius: 20px;"
            "min-width: 60px;"
        );
    } else {
        timerLabel_->setStyleSheet(
            "background-color: #4CAF50;"
            "color: white;"
            "font-size: 24px;"
            "font-weight: bold;"
            "padding: 10px 20px;"
            "border-radius: 20px;"
            "min-width: 60px;"
        );
    }
}

void GameScreen::updatePrize(int prize)
{
    prizeLabel_->setText(QString("%1 VND").arg(prize));
}

void GameScreen::updateLifeline5050(const QList<int>& remainingIndices)
{
    for (int i = 0; i < 4; i++) {
        if (!remainingIndices.contains(i)) {
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
    lifeline5050Button_->setEnabled(false);
    lifelineResultLabel_->setText("50/50: Two wrong answers eliminated!");
    lifelineResultLabel_->setVisible(true);
}

void GameScreen::updateLifelinePhone(const QString& suggestion)
{
    lifelinePhoneAvailable_ = false;
    lifelinePhoneButton_->setEnabled(false);
    lifelineResultLabel_->setText(QString("Friend says: %1").arg(suggestion));
    lifelineResultLabel_->setVisible(true);
}

void GameScreen::updateLifelineAudience(const QMap<QChar, int>& poll)
{
    lifelineAudienceAvailable_ = false;
    lifelineAudienceButton_->setEnabled(false);
    
    QString pollText = "Audience Poll Results:\n";
    QList<QChar> labels = {'A', 'B', 'C', 'D'};
    for (QChar label : labels) {
        int percent = poll.value(label, 0);
        pollText += QString("%1: %2%\n").arg(label).arg(percent);
    }
    
    lifelineResultLabel_->setText(pollText);
    lifelineResultLabel_->setVisible(true);
}

void GameScreen::showLifelineLoading(const QString& message)
{
    lifelineProcessing_ = true;
    lifelineResultLabel_->setText(message);
    lifelineResultLabel_->setVisible(true);
}

void GameScreen::hideLifelineLoading()
{
    lifelineProcessing_ = false;
}

void GameScreen::resetForNewQuestion()
{
    lifelineResultLabel_->setVisible(false);
    selectedAnswer_ = -1;
    updateAnswerButtons();
}

void GameScreen::updateAnswerButtons()
{
    for (int i = 0; i < 4; i++) {
        QString style;
        if (selectedAnswer_ == i) {
            style = 
                "QPushButton {"
                "  background-color: #4CAF50;"
                "  color: white;"
                "  font-size: 16px;"
                "  padding: 15px 20px;"
                "  border-radius: 8px;"
                "  text-align: left;"
                "  min-height: 60px;"
                "}"
                "QPushButton:hover { background-color: #45A049; }";
        } else {
            style = 
                "QPushButton {"
                "  background-color: #1E88E5;"
                "  color: white;"
                "  font-size: 16px;"
                "  padding: 15px 20px;"
                "  border-radius: 8px;"
                "  text-align: left;"
                "  min-height: 60px;"
                "}"
                "QPushButton:hover { background-color: #1976D2; }"
                "QPushButton:disabled { background-color: #555555; color: #888888; }";
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
    } else {
        stopTimer();
        emit gameEnded();
    }
}

void GameScreen::onRevealTimeout()
{
    if (answersRevealed_ < 4 && answersRevealed_ < currentOptions_.size()) {
        answerButtons_[answersRevealed_]->setVisible(true);
        answerButtons_[answersRevealed_]->setEnabled(true);
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

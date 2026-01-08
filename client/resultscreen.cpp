#include "resultscreen.h"
#include "gamestate.h"  // For PRIZE_LADDER
#include <QVBoxLayout>
#include <QHBoxLayout>

ResultScreen::ResultScreen(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ResultScreen::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    titleLabel_ = new QLabel("Game Result", this);
    titleLabel_->setStyleSheet("font-size: 36px; font-weight: bold; color: white;");
    titleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel_);
    
    mainLayout->addSpacing(30);
    
    prizeLabel_ = new QLabel("Total Winnings:", this);
    prizeLabel_->setStyleSheet("font-size: 24px; color: white;");
    prizeLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(prizeLabel_);
    
    scoreLabel_ = new QLabel("Total Score:", this);
    scoreLabel_->setStyleSheet("font-size: 24px; color: white;");
    scoreLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(scoreLabel_);
    
    messageLabel_ = new QLabel(this);
    messageLabel_->setStyleSheet("font-size: 20px; font-weight: bold;");
    messageLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(messageLabel_);
    
    mainLayout->addSpacing(50);
    
    backToHomeButton_ = new QPushButton("Back to Home", this);
    backToHomeButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 18px;"
        "  padding: 15px 50px;"
        "  border-radius: 10px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
    );
    connect(backToHomeButton_, &QPushButton::clicked, this, &ResultScreen::onBackToHomeClicked);
    mainLayout->addWidget(backToHomeButton_, 0, Qt::AlignCenter);
    
    setStyleSheet("background-color: #0D1B2A;");
}

QString formatCurrency(long long amount) {
    // Format number with commas and $ sign
    if (amount == 0) {
        return "$0";
    }
    
    QString formatted = QString::number(amount);
    
    // Add commas for thousands (from right to left)
    for (int i = formatted.length() - 3; i > 0; i -= 3) {
        formatted.insert(i, ',');
    }
    
    return QString("$%1").arg(formatted);
}

void ResultScreen::showResult(long long finalPrize, int totalScore, bool isWinner, int questionNumber, bool isWalkAway)
{
    // Calculate winning according to new rules (all values in USD)
    long long calculatedWinning = 0;
    
    if (isWinner) {
        // Winner gets the final prize (Q15 = $1,000,000)
        // Use PRIZE_LADDER to ensure consistency with UI display
        calculatedWinning = GameState::PRIZE_LADDER[14];  // Q15 index = 14, value = 1,000,000
    } else if (isWalkAway) {
        // Walk away: winning = prize of previous question
        // questionNumber is the current question player is on when they walk away
        // So winning = prize of questionNumber - 1 (the question they just completed)
        if (questionNumber > 1 && questionNumber <= 15) {
            // Use prize from previous question (questionNumber - 1 means index questionNumber - 2)
            calculatedWinning = GameState::PRIZE_LADDER[questionNumber - 2];
        } else if (questionNumber == 1) {
            // If on Q1 and walk away (before answering Q1), winning = 0
            calculatedWinning = 0;
        } else {
            // Fallback: use finalPrize (may need conversion if server sends in different currency)
            calculatedWinning = finalPrize;
        }
    } else {
        // Wrong answer: winning based on question number
        if (questionNumber >= 1 && questionNumber <= 5) {
            // Wrong answer in questions 1-5: $0
            calculatedWinning = 0;
        } else if (questionNumber >= 6 && questionNumber <= 10) {
            // Wrong answer in questions 6-10: $1,000 (checkpoint Q5)
            calculatedWinning = GameState::PRIZE_LADDER[4];  // Q5 index = 4, value = 1000
        } else if (questionNumber >= 11 && questionNumber <= 15) {
            // Wrong answer in questions 11-15: $32,000 (checkpoint Q10)
            calculatedWinning = GameState::PRIZE_LADDER[9];  // Q10 index = 9, value = 32000
        } else {
            // Fallback: use finalPrize
            calculatedWinning = finalPrize;
        }
    }
    
    // Format and display in USD
    prizeLabel_->setText(QString("Total Winnings: %1").arg(formatCurrency(calculatedWinning)));
    scoreLabel_->setText(QString("Total Score: %1 points").arg(totalScore));
    
    if (isWinner) {
        messageLabel_->setText("🎉 Congratulations! You WON!");
        messageLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    } else {
        messageLabel_->setText("Game Over!");
        messageLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: #F44336;");
    }
}

void ResultScreen::onBackToHomeClicked()
{
    emit backToHomeClicked();
}

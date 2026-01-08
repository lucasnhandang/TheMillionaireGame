#include "resultscreen.h"
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

void ResultScreen::showResult(long long finalPrize, int totalScore, bool isWinner)
{
    prizeLabel_->setText(QString("Total Winnings: %1 VND").arg(finalPrize));
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

#include "result_page.h"
#include <QFont>

namespace MillionaireGame {

ResultPage::ResultPage(long long finalPrize, int finalQuestion, bool isWinner, QWidget* parent)
    : QWidget(parent) {
    setupUI();
    
    // Set result message
    if (isWinner) {
        result_label_->setText("🎉 CHÚC MỪNG! BẠN ĐÃ THẮNG! 🎉");
        result_label_->setStyleSheet("color: #00ff00; font-size: 32px; font-weight: bold;");
    } else {
        result_label_->setText("Game Over");
        result_label_->setStyleSheet("color: #ff0000; font-size: 32px; font-weight: bold;");
    }
    
    // Set prize
    QString prize_text;
    formatPrize(finalPrize, prize_text);
    prize_label_->setText("Giải thưởng: " + prize_text);
    
    // Set question number
    question_label_->setText(QString("Bạn đã trả lời đúng %1/15 câu hỏi").arg(finalQuestion));
}

void ResultPage::setupUI() {
    setWindowTitle("Who Wants to Be a Millionaire - Result");
    resize(800, 600);
    
    setStyleSheet(
        "QWidget { background-color: #1a1a2e; color: #eee; }"
        "QLabel { color: #eee; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 10px; padding: 15px 30px; font-size: 18px; font-weight: bold; "
        "min-width: 200px; min-height: 50px; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        "QPushButton:pressed { background-color: #e94560; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(30);
    layout->setContentsMargins(50, 50, 50, 50);
    
    layout->addStretch();
    
    result_label_ = new QLabel(this);
    result_label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(result_label_);
    
    prize_label_ = new QLabel(this);
    prize_label_->setAlignment(Qt::AlignCenter);
    QFont prize_font = prize_label_->font();
    prize_font.setPointSize(24);
    prize_font.setBold(true);
    prize_label_->setFont(prize_font);
    prize_label_->setStyleSheet("color: #e94560;");
    layout->addWidget(prize_label_);
    
    question_label_ = new QLabel(this);
    question_label_->setAlignment(Qt::AlignCenter);
    QFont question_font = question_label_->font();
    question_font.setPointSize(18);
    question_label_->setFont(question_font);
    layout->addWidget(question_label_);
    
    layout->addStretch();
    
    back_button_ = new QPushButton("Back to Menu", this);
    connect(back_button_, &QPushButton::clicked, this, &ResultPage::onBackToMenuClicked);
    layout->addWidget(back_button_);
    
    play_again_button_ = new QPushButton("Play Again", this);
    connect(play_again_button_, &QPushButton::clicked, this, &ResultPage::onPlayAgainClicked);
    layout->addWidget(play_again_button_);
    
    layout->addStretch();
}

void ResultPage::onBackToMenuClicked() {
    emit backToMenuRequested();
}

void ResultPage::onPlayAgainClicked() {
    emit playAgainRequested();
}

void ResultPage::formatPrize(long long prize, QString& formatted) {
    if (prize >= 1000000000) {
        formatted = QString::number(prize / 1000000000.0, 'f', 1) + " tỷ VND";
    } else if (prize >= 1000000) {
        formatted = QString::number(prize / 1000000.0, 'f', 1) + " triệu VND";
    } else if (prize >= 1000) {
        formatted = QString::number(prize / 1000.0, 'f', 0) + " nghìn VND";
    } else {
        formatted = QString::number(prize) + " VND";
    }
}

} // namespace MillionaireGame


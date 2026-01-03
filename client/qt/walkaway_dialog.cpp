#include "walkaway_dialog.h"
#include <QFont>

namespace MillionaireGame {

WalkAwayDialog::WalkAwayDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
}

void WalkAwayDialog::setupUI() {
    setWindowTitle("Walk Away");
    setModal(true);
    resize(400, 200);
    
    setStyleSheet(
        "QDialog { background-color: #1a1a2e; }"
        "QLabel { color: #eee; font-size: 18px; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 10px; padding: 15px 30px; font-size: 16px; font-weight: bold; "
        "min-width: 100px; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        "QPushButton:pressed { background-color: #e94560; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 30, 30, 30);
    
    QLabel* question_label = new QLabel("Are you sure?", this);
    question_label->setAlignment(Qt::AlignCenter);
    QFont font = question_label->font();
    font.setPointSize(20);
    font.setBold(true);
    question_label->setFont(font);
    question_label->setStyleSheet("color: #e94560;");
    layout->addWidget(question_label);
    
    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->setSpacing(20);
    
    yes_button_ = new QPushButton("Yes", this);
    yes_button_->setMinimumHeight(50);
    connect(yes_button_, &QPushButton::clicked, this, &WalkAwayDialog::onYesClicked);
    button_layout->addWidget(yes_button_);
    
    no_button_ = new QPushButton("No", this);
    no_button_->setMinimumHeight(50);
    connect(no_button_, &QPushButton::clicked, this, &WalkAwayDialog::onNoClicked);
    button_layout->addWidget(no_button_);
    
    layout->addLayout(button_layout);
}

void WalkAwayDialog::onYesClicked() {
    emit confirmed();
    accept();
}

void WalkAwayDialog::onNoClicked() {
    emit cancelled();
    reject();
}

} // namespace MillionaireGame


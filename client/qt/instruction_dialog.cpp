#include "instruction_dialog.h"
#include <QFont>

namespace MillionaireGame {

InstructionDialog::InstructionDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
}

void InstructionDialog::setupUI() {
    setWindowTitle("Game Instructions");
    setModal(true);
    resize(700, 600);
    
    setStyleSheet(
        "QDialog { background-color: #1a1a2e; }"
        "QLabel { color: #eee; font-size: 14px; }"
        "QTextEdit { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 15px; color: #eee; font-size: 14px; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 10px 30px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
    );
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 30, 30, 30);
    
    QLabel* title = new QLabel("HƯỚNG DẪN CHƠI GAME", this);
    QFont title_font = title->font();
    title_font.setPointSize(20);
    title_font.setBold(true);
    title->setFont(title_font);
    title->setStyleSheet("color: #e94560;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    
    instruction_text_ = new QTextEdit(this);
    instruction_text_->setReadOnly(true);
    instruction_text_->setHtml(
        "<h3 style='color: #e94560;'>Luật chơi:</h3>"
        "<p>1. Bạn sẽ trả lời 15 câu hỏi, từ dễ đến khó.</p>"
        "<p>2. Mỗi câu hỏi có 4 đáp án, chỉ có 1 đáp án đúng.</p>"
        "<p>3. Bạn có 30 giây để trả lời mỗi câu hỏi.</p>"
        "<p>4. Trả lời đúng sẽ tiếp tục câu hỏi tiếp theo.</p>"
        "<p>5. Trả lời sai sẽ kết thúc game và nhận số tiền ở câu hỏi trước đó.</p>"
        "<br>"
        "<h3 style='color: #e94560;'>Lifelines (Trợ giúp):</h3>"
        "<p><b>50:50:</b> Loại bỏ 2 đáp án sai, còn lại 2 đáp án.</p>"
        "<p><b>Phone a Friend:</b> Gọi điện cho bạn để được gợi ý.</p>"
        "<p><b>Ask the Audience:</b> Hỏi ý kiến khán giả trường quay.</p>"
        "<br>"
        "<h3 style='color: #e94560;'>Các mốc an toàn:</h3>"
        "<p>Bạn sẽ được đảm bảo số tiền ở các câu hỏi: 1, 5, 10, 15.</p>"
        "<br>"
        "<h3 style='color: #e94560;'>Walk Away:</h3>"
        "<p>Bạn có thể dừng cuộc chơi bất cứ lúc nào và nhận số tiền ở câu hỏi trước đó.</p>"
    );
    layout->addWidget(instruction_text_);
    
    close_button_ = new QPushButton("X", this);
    close_button_->setMaximumSize(40, 40);
    close_button_->setStyleSheet(
        "QPushButton { font-size: 20px; border-radius: 20px; }"
    );
    connect(close_button_, &QPushButton::clicked, this, &InstructionDialog::onCloseButtonClicked);
    layout->addWidget(close_button_, 0, Qt::AlignRight);
}

void InstructionDialog::onCloseButtonClicked() {
    accept();
}

} // namespace MillionaireGame


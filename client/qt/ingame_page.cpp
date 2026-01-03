#include "ingame_page.h"
#include "network_thread.h"
#include "walkaway_dialog.h"
#include "../src/protocol_handler.h"
#include "../src/utils/json_parser.h"
#include <QApplication>
#include <QFont>
#include <QMessageBox>
#include <QThread>
#include <QStringList>
#include <cmath>

namespace MillionaireGame {

// Prize amounts for each level (in VND for example, or use your currency)
const long long PRIZE_AMOUNTS[15] = {
    100000,      // Level 1
    200000,      // Level 2
    300000,      // Level 3
    500000,      // Level 4
    1000000,     // Level 5 (safe haven)
    2000000,     // Level 6
    4000000,     // Level 7
    8000000,     // Level 8
    16000000,    // Level 9
    32000000,    // Level 10 (safe haven)
    64000000,    // Level 11
    125000000,   // Level 12
    250000000,   // Level 13
    500000000,   // Level 14
    1000000000   // Level 15 (1 million)
};

InGamePage::InGamePage(NetworkThread* networkThread, const QString& authToken, QWidget* parent)
    : QWidget(parent), network_thread_(networkThread), auth_token_(authToken),
      current_game_id_(0), current_question_number_(0), selected_answer_(-1),
      answer_submitted_(false), timer_paused_(false), lifeline_active_(false),
      time_remaining_(30), time_limit_(30), walkaway_dialog_(nullptr) {
    
    available_lifelines_ = {true, true, true}; // All lifelines available initially
    
    setupUI();
    setupPrizeTree();
    setupLifelineDisplay();
    
    // Connect network signals
    if (network_thread_) {
        connect(network_thread_, &NetworkThread::questionReceived,
                this, &InGamePage::onQuestionReceived);
        connect(network_thread_, &NetworkThread::lifelineInfoReceived,
                this, &InGamePage::onLifelineInfoReceived);
        connect(network_thread_, &NetworkThread::answerResponseReceived,
                this, &InGamePage::onAnswerResponseReceived);
        connect(network_thread_, &NetworkThread::gameEndReceived,
                this, &InGamePage::onGameEndReceived);
    }
    
    // Setup timers
    game_timer_ = new QTimer(this);
    connect(game_timer_, &QTimer::timeout, this, &InGamePage::onTimerTimeout);
    
    answer_verification_timer_ = new QTimer(this);
    answer_verification_timer_->setSingleShot(true);
    connect(answer_verification_timer_, &QTimer::timeout, this, &InGamePage::onAnswerVerificationTimeout);
    
    // Start game
    if (network_thread_ && network_thread_->getProtocolHandler()) {
        QThread* start_thread = QThread::create([this]() {
            ProtocolHandler* protocol = network_thread_->getProtocolHandler();
            auto response = protocol->startGame(auth_token_.toStdString(), false);
            if (response.success) {
                // Wait for QUESTION_INFO notification
            }
        });
        connect(start_thread, &QThread::finished, start_thread, &QThread::deleteLater);
        start_thread->start();
    }
}

InGamePage::~InGamePage() {
    stopTimer();
    if (walkaway_dialog_) {
        delete walkaway_dialog_;
    }
}

void InGamePage::setupUI() {
    setWindowTitle("Who Wants to Be a Millionaire - Game");
    resize(1400, 900);
    
    setStyleSheet(
        "QWidget { background-color: #0a0e27; color: #eee; }"
        "QLabel { color: #eee; }"
        "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
        "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        "QPushButton:pressed { background-color: #e94560; }"
        "QPushButton:disabled { background-color: #0a0a0a; color: #666; border-color: #333; }"
        "QProgressBar { border: 2px solid #0f3460; border-radius: 10px; "
        "background-color: #16213e; text-align: center; }"
        "QProgressBar::chunk { background-color: #e94560; border-radius: 8px; }"
    );
    
    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->setSpacing(20);
    main_layout->setContentsMargins(20, 20, 20, 20);
    
    // Left side - Question and Answers
    QVBoxLayout* left_layout = new QVBoxLayout();
    left_layout->setSpacing(15);
    
    // Question label
    question_label_ = new QLabel("Đang tải câu hỏi...", this);
    question_label_->setWordWrap(true);
    question_label_->setAlignment(Qt::AlignCenter);
    QFont question_font = question_label_->font();
    question_font.setPointSize(18);
    question_font.setBold(true);
    question_label_->setFont(question_font);
    question_label_->setStyleSheet(
        "QLabel { background-color: #16213e; border: 3px solid #0f3460; "
        "border-radius: 10px; padding: 20px; min-height: 100px; }"
    );
    left_layout->addWidget(question_label_);
    
    // Answer buttons
    QGridLayout* answer_layout = new QGridLayout();
    answer_layout->setSpacing(10);
    
    for (int i = 0; i < 4; i++) {
        answer_buttons_[i] = new QPushButton(this);
        answer_buttons_[i]->setMinimumHeight(80);
        QFont btn_font = answer_buttons_[i]->font();
        btn_font.setPointSize(16);
        answer_buttons_[i]->setFont(btn_font);
        
        int row = i / 2;
        int col = i % 2;
        answer_layout->addWidget(answer_buttons_[i], row, col);
        
        connect(answer_buttons_[i], &QPushButton::clicked, [this, i]() {
            onAnswerButtonClicked(i);
        });
    }
    left_layout->addLayout(answer_layout);
    
    // Timer and question info
    QHBoxLayout* info_layout = new QHBoxLayout();
    
    question_number_label_ = new QLabel("Câu hỏi: 0/15", this);
    question_number_label_->setStyleSheet("font-size: 16px; font-weight: bold; color: #e94560;");
    info_layout->addWidget(question_number_label_);
    
    timer_label_ = new QLabel("30", this);
    QFont timer_font = timer_label_->font();
    timer_font.setPointSize(24);
    timer_font.setBold(true);
    timer_label_->setFont(timer_font);
    timer_label_->setAlignment(Qt::AlignCenter);
    timer_label_->setStyleSheet(
        "QLabel { background-color: #16213e; border: 3px solid #e94560; "
        "border-radius: 50px; min-width: 80px; min-height: 80px; }"
    );
    info_layout->addWidget(timer_label_);
    
    prize_label_ = new QLabel("Giải thưởng: 0 VND", this);
    prize_label_->setStyleSheet("font-size: 16px; font-weight: bold; color: #e94560;");
    info_layout->addWidget(prize_label_);
    
    left_layout->addLayout(info_layout);
    
    // Lifelines
    QHBoxLayout* lifeline_layout = new QHBoxLayout();
    lifeline_layout->setSpacing(10);
    
    lifeline_5050_button_ = new QPushButton("50:50", this);
    lifeline_5050_button_->setMinimumHeight(50);
    connect(lifeline_5050_button_, &QPushButton::clicked, this, &InGamePage::onLifeline5050Clicked);
    lifeline_layout->addWidget(lifeline_5050_button_);
    
    lifeline_call_button_ = new QPushButton("📞 Phone", this);
    lifeline_call_button_->setMinimumHeight(50);
    connect(lifeline_call_button_, &QPushButton::clicked, this, &InGamePage::onLifelineCallClicked);
    lifeline_layout->addWidget(lifeline_call_button_);
    
    lifeline_ask_button_ = new QPushButton("👥 Ask", this);
    lifeline_ask_button_->setMinimumHeight(50);
    connect(lifeline_ask_button_, &QPushButton::clicked, this, &InGamePage::onLifelineAskClicked);
    lifeline_layout->addWidget(lifeline_ask_button_);
    
    left_layout->addLayout(lifeline_layout);
    
    // Lifeline display area
    lifeline_stack_ = new QStackedWidget(this);
    lifeline_stack_->setMaximumHeight(200);
    left_layout->addWidget(lifeline_stack_);
    
    // Walk Away button (bottom right)
    walkaway_button_ = new QPushButton("Walk Away", this);
    walkaway_button_->setMinimumHeight(50);
    walkaway_button_->setStyleSheet(
        "QPushButton { background-color: #8b0000; border-color: #ff0000; }"
        "QPushButton:hover { background-color: #a00000; }"
    );
    connect(walkaway_button_, &QPushButton::clicked, this, &InGamePage::onWalkAwayClicked);
    left_layout->addWidget(walkaway_button_, 0, Qt::AlignRight);
    
    main_layout->addLayout(left_layout, 2);
    
    // Right side - Prize Tree
    prize_tree_widget_ = new QWidget(this);
    prize_tree_widget_->setStyleSheet("background-color: #16213e; border: 2px solid #0f3460; border-radius: 10px;");
    QVBoxLayout* prize_layout = new QVBoxLayout(prize_tree_widget_);
    prize_layout->setSpacing(5);
    prize_layout->setContentsMargins(15, 15, 15, 15);
    
    QLabel* prize_title = new QLabel("PRIZE TREE", prize_tree_widget_);
    prize_title->setAlignment(Qt::AlignCenter);
    QFont title_font = prize_title->font();
    title_font.setPointSize(18);
    title_font.setBold(true);
    prize_title->setFont(title_font);
    prize_title->setStyleSheet("color: #e94560;");
    prize_layout->addWidget(prize_title);
    
    // Create prize labels (from level 15 down to 1)
    for (int i = 14; i >= 0; i--) {
        QString prize_text;
        formatPrize(PRIZE_AMOUNTS[i], prize_text);
        prize_labels_[i] = new QLabel(QString::number(i + 1) + ": " + prize_text, prize_tree_widget_);
        prize_labels_[i]->setAlignment(Qt::AlignCenter);
        prize_labels_[i]->setStyleSheet(
            "QLabel { padding: 8px; border-radius: 5px; background-color: #0a0e27; }"
        );
        prize_layout->addWidget(prize_labels_[i]);
    }
    
    prize_layout->addStretch();
    main_layout->addWidget(prize_tree_widget_, 1);
}

void InGamePage::setupPrizeTree() {
    // Already set up in setupUI
}

void InGamePage::setupLifelineDisplay() {
    // 50:50 widget
    lifeline_5050_widget_ = new QWidget();
    QVBoxLayout* layout_5050 = new QVBoxLayout(lifeline_5050_widget_);
    lifeline_5050_info_label_ = new QLabel("50:50 - 2 đáp án sai đã bị loại", lifeline_5050_widget_);
    lifeline_5050_info_label_->setAlignment(Qt::AlignCenter);
    lifeline_5050_info_label_->setStyleSheet("font-size: 18px; color: #ffa500; padding: 20px;");
    layout_5050->addWidget(lifeline_5050_info_label_);
    lifeline_stack_->addWidget(lifeline_5050_widget_);
    
    // Phone widget
    lifeline_call_widget_ = new QWidget();
    QVBoxLayout* layout_call = new QVBoxLayout(lifeline_call_widget_);
    lifeline_call_info_text_ = new QTextEdit(lifeline_call_widget_);
    lifeline_call_info_text_->setReadOnly(true);
    lifeline_call_info_text_->setStyleSheet(
        "QTextEdit { background-color: #0a0e27; border: 2px solid #0f3460; "
        "border-radius: 5px; padding: 15px; font-size: 16px; }"
    );
    layout_call->addWidget(lifeline_call_info_text_);
    lifeline_stack_->addWidget(lifeline_call_widget_);
    
    // Ask widget
    lifeline_ask_widget_ = new QWidget();
    QVBoxLayout* layout_ask = new QVBoxLayout(lifeline_ask_widget_);
    QLabel* ask_title = new QLabel("Khán giả trường quay:", lifeline_ask_widget_);
    ask_title->setAlignment(Qt::AlignCenter);
    ask_title->setStyleSheet("font-size: 18px; color: #e94560; font-weight: bold;");
    layout_ask->addWidget(ask_title);
    
    lifeline_ask_chart_widget_ = new QWidget(lifeline_ask_widget_);
    layout_ask->addWidget(lifeline_ask_chart_widget_);
    lifeline_stack_->addWidget(lifeline_ask_widget_);
    
    lifeline_stack_->setCurrentIndex(0); // Hide by default
    lifeline_stack_->hide();
}

void InGamePage::updatePrizeTree(int currentLevel) {
    for (int i = 0; i < 15; i++) {
        QString style;
        if (i + 1 == currentLevel) {
            style = "QLabel { padding: 8px; border-radius: 5px; "
                   "background-color: #e94560; color: #fff; font-weight: bold; }";
        } else if (i + 1 < currentLevel) {
            style = "QLabel { padding: 8px; border-radius: 5px; "
                   "background-color: #0f3460; color: #fff; }";
        } else {
            style = "QLabel { padding: 8px; border-radius: 5px; "
                   "background-color: #0a0e27; color: #aaa; }";
        }
        prize_labels_[i]->setStyleSheet(style);
    }
}

void InGamePage::displayQuestion(const ProtocolHandler::QuestionInfo& question) {
    current_question_ = question;
    current_question_number_ = question.questionNumber;
    current_game_id_ = question.gameId;
    time_remaining_ = question.timeRemaining > 0 ? question.timeRemaining : question.timeLimit;
    time_limit_ = question.timeLimit;
    
    // Update question text
    question_label_->setText(QString::fromStdString(question.question));
    
    // Update answer buttons
    QString labels[] = {"A", "B", "C", "D"};
    for (int i = 0; i < 4 && i < (int)question.optionTexts.size(); i++) {
        QString text = labels[i] + ": " + QString::fromStdString(question.optionTexts[i]);
        answer_buttons_[i]->setText(text);
        answer_buttons_[i]->setEnabled(true);
        answer_buttons_[i]->setStyleSheet(
            "QPushButton { background-color: #16213e; border: 2px solid #0f3460; "
            "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
            "QPushButton:hover { background-color: #0f3460; border-color: #e94560; }"
        );
    }
    
    // Update question number and prize
    question_number_label_->setText(QString("Câu hỏi: %1/15").arg(question.questionNumber));
    QString prize_text;
    formatPrize(question.prize, prize_text);
    prize_label_->setText("Giải thưởng: " + prize_text);
    
    // Update prize tree
    updatePrizeTree(question.questionNumber);
    
    // Reset state
    answer_submitted_ = false;
    selected_answer_ = -1;
    enableAnswerButtons(true);
    
    // Start timer
    startTimer();
}

void InGamePage::onAnswerButtonClicked(int answerIndex) {
    if (answer_submitted_ || timer_paused_) {
        return;
    }
    
    selected_answer_ = answerIndex;
    answer_submitted_ = true;
    
    // Highlight selected answer in yellow
    answer_buttons_[answerIndex]->setStyleSheet(
        "QPushButton { background-color: #ffd700; color: #000; border: 3px solid #ffa500; "
        "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
    );
    
    // Disable all buttons
    enableAnswerButtons(false);
    
    // Pause timer
    pauseTimer();
    
    // Wait 5 seconds then submit
    answer_verification_timer_->start(5000);
}

void InGamePage::onAnswerVerificationTimeout() {
    submitAnswer(selected_answer_);
}

void InGamePage::submitAnswer(int answerIndex) {
    if (!network_thread_ || !network_thread_->getProtocolHandler()) {
        return;
    }
    
    QThread* answer_thread = QThread::create([this, answerIndex]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->answerQuestion(
            auth_token_.toStdString(),
            current_game_id_,
            current_question_number_,
            answerIndex
        );
        
        // The response will come via answerResponseReceived signal
    });
    connect(answer_thread, &QThread::finished, answer_thread, &QThread::deleteLater);
    answer_thread->start();
}

void InGamePage::onAnswerResponseReceived(const QString& answerJson) {
    // Parse response
    std::string json = answerJson.toStdString();
    
    // Try to extract from data field first
    size_t dataPos = json.find("\"data\":");
    std::string data = json;
    if (dataPos != std::string::npos) {
        data = json.substr(dataPos);
    }
    
    bool correct = JsonParser::extractBool(data, "correct");
    int correctAnswer = JsonParser::extractInt(data, "correctAnswer");
    bool gameOver = JsonParser::extractBool(data, "gameOver");
    
    showAnswerResult(correct, correctAnswer);
    
    if (gameOver) {
        // Wait for GAME_END notification or emit directly
        long long finalPrize = JsonParser::extractInt(data, "finalPrize");
        int finalQuestion = JsonParser::extractInt(data, "questionNumber");
        bool isWinner = JsonParser::extractBool(data, "isWinner");
        emit gameEnded(finalPrize, finalQuestion, isWinner);
    }
}

void InGamePage::showAnswerResult(bool correct, int correctAnswer) {
    stopTimer();
    
    if (correct) {
        // Flash selected answer green for 2 seconds
        answer_buttons_[selected_answer_]->setStyleSheet(
            "QPushButton { background-color: #00ff00; color: #000; border: 3px solid #00aa00; "
            "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
        );
        
        // Wait 2 seconds then continue to next question
        QTimer::singleShot(2000, this, [this]() {
            // Next question will come via questionReceived signal
        });
    } else {
        // Flash selected answer red and correct answer green
        answer_buttons_[selected_answer_]->setStyleSheet(
            "QPushButton { background-color: #ff0000; color: #fff; border: 3px solid #aa0000; "
            "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
        );
        
        if (correctAnswer >= 0 && correctAnswer < 4) {
            answer_buttons_[correctAnswer]->setStyleSheet(
                "QPushButton { background-color: #00ff00; color: #000; border: 3px solid #00aa00; "
                "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
            );
        }
        
        // Wait 2 seconds then show result page
        QTimer::singleShot(2000, this, [this]() {
            // Emit gameEnded signal
            long long finalPrize = (current_question_number_ > 1) ? 
                PRIZE_AMOUNTS[current_question_number_ - 2] : 0;
            emit gameEnded(finalPrize, current_question_number_ - 1, false);
        });
    }
}

void InGamePage::onTimerTimeout() {
    if (timer_paused_ || lifeline_active_) {
        return;
    }
    
    time_remaining_--;
    updateTimerDisplay();
    
    if (time_remaining_ <= 0) {
        stopTimer();
        // Time's up - show correct answer and go to result
        if (current_question_.correct_answer >= 0 && current_question_.correct_answer < 4) {
            answer_buttons_[current_question_.correct_answer]->setStyleSheet(
                "QPushButton { background-color: #00ff00; color: #000; border: 3px solid #00aa00; "
                "border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; }"
            );
        }
        
        QTimer::singleShot(2000, this, [this]() {
            long long finalPrize = (current_question_number_ > 1) ? 
                PRIZE_AMOUNTS[current_question_number_ - 2] : 0;
            emit gameEnded(finalPrize, current_question_number_ - 1, false);
        });
    }
}

void InGamePage::updateTimerDisplay() {
    timer_label_->setText(QString::number(time_remaining_));
    
    // Change color based on time remaining
    if (time_remaining_ <= 10) {
        timer_label_->setStyleSheet(
            "QLabel { background-color: #ff0000; border: 3px solid #aa0000; "
            "border-radius: 50px; min-width: 80px; min-height: 80px; color: #fff; }"
        );
    } else if (time_remaining_ <= 20) {
        timer_label_->setStyleSheet(
            "QLabel { background-color: #ffa500; border: 3px solid #ff8800; "
            "border-radius: 50px; min-width: 80px; min-height: 80px; color: #000; }"
        );
    } else {
        timer_label_->setStyleSheet(
            "QLabel { background-color: #16213e; border: 3px solid #e94560; "
            "border-radius: 50px; min-width: 80px; min-height: 80px; color: #eee; }"
        );
    }
}

void InGamePage::startTimer() {
    time_remaining_ = time_limit_;
    updateTimerDisplay();
    if (!timer_paused_ && !lifeline_active_) {
        game_timer_->start(1000); // Update every second
    }
}

void InGamePage::pauseTimer() {
    timer_paused_ = true;
    game_timer_->stop();
}

void InGamePage::resumeTimer() {
    timer_paused_ = false;
    if (!lifeline_active_) {
        game_timer_->start(1000);
    }
}

void InGamePage::stopTimer() {
    game_timer_->stop();
    timer_paused_ = false;
}

void InGamePage::onLifeline5050Clicked() {
    if (!available_lifelines_[0] || answer_submitted_ || !network_thread_) {
        return;
    }
    
    pauseTimer();
    
    QThread* lifeline_thread = QThread::create([this]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->useLifeline(
            auth_token_.toStdString(),
            current_game_id_,
            current_question_number_,
            "5050"
        );
    });
    connect(lifeline_thread, &QThread::finished, lifeline_thread, &QThread::deleteLater);
    lifeline_thread->start();
}

void InGamePage::onLifelineCallClicked() {
    if (!available_lifelines_[1] || answer_submitted_ || !network_thread_) {
        return;
    }
    
    pauseTimer();
    
    QThread* lifeline_thread = QThread::create([this]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->useLifeline(
            auth_token_.toStdString(),
            current_game_id_,
            current_question_number_,
            "PHONE"
        );
    });
    connect(lifeline_thread, &QThread::finished, lifeline_thread, &QThread::deleteLater);
    lifeline_thread->start();
}

void InGamePage::onLifelineAskClicked() {
    if (!available_lifelines_[2] || answer_submitted_ || !network_thread_) {
        return;
    }
    
    pauseTimer();
    
    QThread* lifeline_thread = QThread::create([this]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->useLifeline(
            auth_token_.toStdString(),
            current_game_id_,
            current_question_number_,
            "AUDIENCE"
        );
    });
    connect(lifeline_thread, &QThread::finished, lifeline_thread, &QThread::deleteLater);
    lifeline_thread->start();
}

void InGamePage::onLifelineInfoReceived(const QString& lifelineJson) {
    // Parse lifeline info and display
    std::string json = lifelineJson.toStdString();
    std::string type = JsonParser::extractString(json, "lifelineType");
    
    // Get lifeline data from question (should be in database)
    // For now, we'll parse from the notification
    
    lifeline_active_ = true;
    lifeline_stack_->show();
    
    if (type == "5050") {
        // Parse which options to remove
        // This should come from database lifeline_5050_info field
        displayLifeline5050(""); // Will be filled from database
        lifeline_stack_->setCurrentWidget(lifeline_5050_widget_);
    } else if (type == "PHONE") {
        displayLifelineCall(""); // Will be filled from database
        lifeline_stack_->setCurrentWidget(lifeline_call_widget_);
    } else if (type == "AUDIENCE") {
        displayLifelineAsk(""); // Will be filled from database
        lifeline_stack_->setCurrentWidget(lifeline_ask_widget_);
    }
    
    // Resume timer after user closes lifeline display
    // (User clicks somewhere or after a delay)
    QTimer::singleShot(5000, this, [this]() {
        hideLifelineDisplay();
        resumeTimer();
    });
}

void InGamePage::displayLifeline5050(const QString& info) {
    // Parse "1,2" format and disable those answer buttons
    QStringList indices = info.split(",");
    for (const QString& idxStr : indices) {
        bool ok;
        int idx = idxStr.trimmed().toInt(&ok);
        if (ok && idx >= 0 && idx < 4) {
            answer_buttons_[idx]->setEnabled(false);
            answer_buttons_[idx]->setStyleSheet(
                "QPushButton { background-color: #333; color: #666; border: 2px solid #222; "
                "border-radius: 8px; padding: 15px; font-size: 16px; }"
            );
        }
    }
    lifeline_5050_info_label_->setText("50:50 - 2 đáp án sai đã bị loại bỏ");
}

void InGamePage::displayLifelineCall(const QString& info) {
    lifeline_call_info_text_->setPlainText(info);
}

void InGamePage::displayLifelineAsk(const QString& info) {
    // Parse "10088002" format (10% A, 8% B, 80% C, 2% D)
    if (info.length() == 8) {
        int percents[4];
        for (int i = 0; i < 4; i++) {
            percents[i] = info.mid(i * 2, 2).toInt();
        }
        
        // Create a simple bar chart
        QVBoxLayout* chart_layout = new QVBoxLayout(lifeline_ask_chart_widget_);
        QString labels[] = {"A", "B", "C", "D"};
        for (int i = 0; i < 4; i++) {
            QHBoxLayout* bar_layout = new QHBoxLayout();
            QLabel* label = new QLabel(labels[i] + ":", lifeline_ask_chart_widget_);
            label->setMinimumWidth(30);
            bar_layout->addWidget(label);
            
            QProgressBar* bar = new QProgressBar(lifeline_ask_chart_widget_);
            bar->setValue(percents[i]);
            bar->setMaximum(100);
            bar->setTextVisible(true);
            bar->setFormat(QString::number(percents[i]) + "%");
            bar_layout->addWidget(bar);
            
            chart_layout->addLayout(bar_layout);
        }
    }
}

void InGamePage::hideLifelineDisplay() {
    lifeline_active_ = false;
    lifeline_stack_->hide();
}

void InGamePage::onQuestionReceived(const QString& questionJson) {
    std::string json = questionJson.toStdString();
    ProtocolHandler::QuestionInfo question = network_thread_->getProtocolHandler()->parseQuestionInfo(json);
    displayQuestion(question);
}

void InGamePage::onGameEndReceived(const QString& gameEndJson) {
    std::string json = gameEndJson.toStdString();
    bool isWinner = JsonParser::extractBool(json, "isWinner");
    long long finalPrize = JsonParser::extractInt(json, "finalPrize");
    int finalQuestion = JsonParser::extractInt(json, "finalQuestionNumber");
    
    emit gameEnded(finalPrize, finalQuestion, isWinner);
}

void InGamePage::onWalkAwayClicked() {
    pauseTimer();
    
    if (!walkaway_dialog_) {
        walkaway_dialog_ = new WalkAwayDialog(this);
        connect(walkaway_dialog_, &WalkAwayDialog::confirmed,
                this, &InGamePage::onWalkAwayConfirmed);
        connect(walkaway_dialog_, &WalkAwayDialog::cancelled,
                this, &InGamePage::onWalkAwayCancelled);
    }
    
    walkaway_dialog_->exec();
}

void InGamePage::onWalkAwayConfirmed() {
    if (!network_thread_ || !network_thread_->getProtocolHandler()) {
        return;
    }
    
    QThread* giveup_thread = QThread::create([this]() {
        ProtocolHandler* protocol = network_thread_->getProtocolHandler();
        auto response = protocol->giveUp(
            auth_token_.toStdString(),
            current_game_id_,
            current_question_number_
        );
        
        // Emit gameEnded with previous question's prize
        long long finalPrize = (current_question_number_ > 1) ? 
            PRIZE_AMOUNTS[current_question_number_ - 2] : 0;
        QMetaObject::invokeMethod(this, "gameEnded", Qt::QueuedConnection,
            Q_ARG(long long, finalPrize),
            Q_ARG(int, current_question_number_ - 1),
            Q_ARG(bool, false));
    });
    connect(giveup_thread, &QThread::finished, giveup_thread, &QThread::deleteLater);
    giveup_thread->start();
}

void InGamePage::onWalkAwayCancelled() {
    resumeTimer();
}

void InGamePage::formatPrize(long long prize, QString& formatted) {
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

void InGamePage::enableAnswerButtons(bool enable) {
    for (int i = 0; i < 4; i++) {
        answer_buttons_[i]->setEnabled(enable);
    }
}

void InGamePage::updateLifelineButtons() {
    lifeline_5050_button_->setEnabled(available_lifelines_[0] && !answer_submitted_);
    lifeline_call_button_->setEnabled(available_lifelines_[1] && !answer_submitted_);
    lifeline_ask_button_->setEnabled(available_lifelines_[2] && !answer_submitted_);
}

} // namespace MillionaireGame


#ifndef INGAME_PAGE_H
#define INGAME_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTextEdit>
#include "../src/protocol_handler.h"

namespace MillionaireGame {

class NetworkThread;
class WalkAwayDialog;

/**
 * InGame page - Main gameplay interface
 */
class InGamePage : public QWidget {
    Q_OBJECT

public:
    explicit InGamePage(NetworkThread* networkThread, const QString& authToken, QWidget* parent = nullptr);
    ~InGamePage();

signals:
    void gameEnded(long long finalPrize, int finalQuestion, bool isWinner);

private slots:
    void onAnswerButtonClicked(int answerIndex);
    void onLifeline5050Clicked();
    void onLifelineCallClicked();
    void onLifelineAskClicked();
    void onWalkAwayClicked();
    void onWalkAwayConfirmed();
    void onWalkAwayCancelled();
    void onTimerTimeout();
    void onAnswerVerificationTimeout();
    void onQuestionReceived(const QString& questionJson);
    void onLifelineInfoReceived(const QString& lifelineJson);
    void onAnswerResponseReceived(const QString& answerJson);
    void onGameEndReceived(const QString& gameEndJson);

private:
    NetworkThread* network_thread_;
    QString auth_token_;
    int current_game_id_;
    int current_question_number_;
    int selected_answer_;
    bool answer_submitted_;
    bool timer_paused_;
    bool lifeline_active_;
    
    // UI Components
    QLabel* question_label_;
    QPushButton* answer_buttons_[4];
    QLabel* prize_label_;
    QLabel* question_number_label_;
    QLabel* timer_label_;
    QProgressBar* timer_progress_;
    QPushButton* lifeline_5050_button_;
    QPushButton* lifeline_call_button_;
    QPushButton* lifeline_ask_button_;
    QPushButton* walkaway_button_;
    
    // Prize tree (right side)
    QWidget* prize_tree_widget_;
    QLabel* prize_labels_[15];
    
    // Lifeline display area
    QStackedWidget* lifeline_stack_;
    QWidget* lifeline_5050_widget_;
    QWidget* lifeline_call_widget_;
    QWidget* lifeline_ask_widget_;
    QLabel* lifeline_5050_info_label_;
    QTextEdit* lifeline_call_info_text_;
    QWidget* lifeline_ask_chart_widget_;
    
    // Timers
    QTimer* game_timer_;
    QTimer* answer_verification_timer_;
    int time_remaining_;
    int time_limit_;
    
    // Dialogs
    WalkAwayDialog* walkaway_dialog_;
    
    // Question data
    ProtocolHandler::QuestionInfo current_question_;
    std::vector<bool> available_lifelines_; // [5050, call, ask]
    
    void setupUI();
    void setupPrizeTree();
    void setupLifelineDisplay();
    void updatePrizeTree(int currentLevel);
    void displayQuestion(const ProtocolHandler::QuestionInfo& question);
    void displayLifeline5050(const QString& info); // "1,2" format
    void displayLifelineCall(const QString& info);
    void displayLifelineAsk(const QString& info); // "10088002" format
    void hideLifelineDisplay();
    void startTimer();
    void pauseTimer();
    void resumeTimer();
    void stopTimer();
    void updateTimerDisplay();
    void submitAnswer(int answerIndex);
    void showAnswerResult(bool correct, int correctAnswer);
    void formatPrize(long long prize, QString& formatted);
    void enableAnswerButtons(bool enable);
    void updateLifelineButtons();
};

} // namespace MillionaireGame

#endif // INGAME_PAGE_H


#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTimer>
#include <QTime>
#include <QMap>
#include <QList>
#include <QChar>
#include <QStringList>

class ProtocolHandler;
class GameState;

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget *parent = nullptr);

    void setProtocolHandler(ProtocolHandler* protocol);
    void setGameState(GameState* gameState);
    void setDemoMode(bool demoMode);
    
    void updateQuestion(const QString& question, const QStringList& options, int questionNumber);
    void updateTimer(int seconds);
    void updatePrize(int prize);
    void updateLifeline5050(const QList<int>& remainingIndices);
    void updateLifelinePhone(const QString& suggestion);
    void updateLifelineAudience(const QMap<QChar, int>& poll);
    void showLifelineLoading(const QString& message);
    void hideLifelineLoading();
    void resetForNewQuestion();
    void resetLifelines();  // Reset all lifelines for new game session

signals:
    void answerSubmitted(int answerIndex);
    void lifelineUsed(const QString& lifelineType);
    void walkAwayClicked();
    void gameEnded();

private slots:
    void onAnswerButtonClicked();
    void onLifeline5050Clicked();
    void onLifelinePhoneClicked();
    void onLifelineAudienceClicked();
    void onWalkAwayClicked();
    void onTimerTimeout();
    void onRevealTimeout();

private:
    void setupUI();
    void setupPrizeLadder();
    void updateAnswerButtons();
    void updateLifelineButtons();
    void startTimer();
    void stopTimer();
    void revealNextOption();

    // UI Components
    QLabel* prizeLadderLabel_;
    QWidget* prizeLadderWidget_;
    QLabel* questionLabel_;
    QLabel* timerLabel_;
    QProgressBar* progressBar_;
    QLabel* prizeLabel_;
    QLabel* questionNumberLabel_;
    
    QPushButton* answerButtonA_;
    QPushButton* answerButtonB_;
    QPushButton* answerButtonC_;
    QPushButton* answerButtonD_;
    QList<QPushButton*> answerButtons_;
    
    QPushButton* lifeline5050Button_;
    QPushButton* lifelinePhoneButton_;
    QPushButton* lifelineAudienceButton_;
    QPushButton* walkAwayButton_;
    
    QLabel* lifelineResultLabel_;
    QWidget* audiencePollWidget_;
    
    // State
    ProtocolHandler* protocol_;
    GameState* gameState_;
    bool demoMode_;
    
    QString currentQuestion_;
    QStringList currentOptions_;
    int currentQuestionNumber_;
    int selectedAnswer_;
    int timeRemaining_;
    bool timerRunning_;
    
    QTimer* countdownTimer_;
    QTimer* revealTimer_;
    int answersRevealed_;
    
    // Lifelines
    bool lifeline5050Available_;
    bool lifelinePhoneAvailable_;
    bool lifelineAudienceAvailable_;
    bool lifelineProcessing_;
    QString lifelineType_;
};

#endif // GAMESCREEN_H

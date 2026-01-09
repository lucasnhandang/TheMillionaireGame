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

// Forward declaration for UI class (generated from .ui file)
namespace Ui {
    class GameScreen;
}

class ProtocolHandler;
class GameState;

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget *parent = nullptr);
    ~GameScreen();

    void setProtocolHandler(ProtocolHandler* protocol);
    void setGameState(GameState* gameState);
    void setDemoMode(bool demoMode);
    
    void updateQuestion(const QString& question, const QStringList& options, int questionNumber);
    void updateTimer(int seconds);
    void updateScore(int score);  // BONUS: Update score display
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
    void saveGameClicked();
    void gameEnded();

private slots:
    void onAnswerButtonClicked();
    void onLifeline5050Clicked();
    void onLifelinePhoneClicked();
    void onLifelineAudienceClicked();
    void onWalkAwayClicked();
    void onSaveGameClicked();
    void onTimerTimeout();
    void onRevealTimeout();

private:
    void setupUI();
    void setupConnections();  // Connect signals and slots
    void setupInitialVisibility();  // Set initial visibility for widgets
    void setupPrizeLadder();
    void loadLifelineIcons();  // Load lifeline button icons
    void updateAnswerButtons();
    void updateLifelineButtons();
    void startTimer();
    void stopTimer();
    void revealNextOption();
    void highlightPrizeLadder(int questionNumber);  // Highlight current question in prize ladder

    Ui::GameScreen* ui;  // UI loaded from .ui file

    // UI Components (loaded from UI)
    QLabel* prizeLabel1_;
    QLabel* prizeLabel2_;
    QLabel* prizeLabel3_;
    QLabel* prizeLabel4_;
    QLabel* prizeLabel5_;
    QLabel* prizeLabel6_;
    QLabel* prizeLabel7_;
    QLabel* prizeLabel8_;
    QLabel* prizeLabel9_;
    QLabel* prizeLabel10_;
    QLabel* prizeLabel11_;
    QLabel* prizeLabel12_;
    QLabel* prizeLabel13_;
    QLabel* prizeLabel14_;
    QLabel* prizeLabel15_;
    QList<QLabel*> prizeLabels_;  // For easy access
    
    QLabel* questionLabel_;
    QLabel* timerLabel_;
    QLabel* scoreLabel_;  // BONUS: Score display
    
    QPushButton* answerButtonA_;
    QPushButton* answerButtonB_;
    QPushButton* answerButtonC_;
    QPushButton* answerButtonD_;
    QList<QPushButton*> answerButtons_;
    
    QPushButton* lifeline5050Button_;
    QPushButton* lifelinePhoneButton_;
    QPushButton* lifelineAudienceButton_;  // Maps to lifelinAskButton in UI
    QPushButton* walkAwayButton_;
    QPushButton* saveGameButton_;
    QPushButton* submitButton_;
    
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
    int totalScore_;  // BONUS: Track total score
    
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

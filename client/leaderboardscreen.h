#ifndef LEADERBOARDSCREEN_H
#define LEADERBOARDSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

class ProtocolHandler;

class LeaderboardScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LeaderboardScreen(QWidget *parent = nullptr);

    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);
    void refreshData();

signals:
    void backToHome();

private slots:
    void onBackToHomeClicked();
    void onModeChanged();
    void onRefreshClicked();

private:
    void setupUI();
    void loadLeaderboard();
    QString formatCurrency(long long amount);
    void updateTable();

    ProtocolHandler* protocol_;
    bool demoMode_;
    
    // UI Components
    QLabel* titleLabel_;
    QPushButton* backButton_;
    QPushButton* refreshButton_;
    QPushButton* globalModeButton_;
    QPushButton* friendModeButton_;
    QTableWidget* tableWidget_;
    
    // State
    QString currentMode_;  // "global" or "friend"
    struct LeaderboardEntry {
        QString username;
        long long totalWinning;
        int totalPoints;
        int rank;
    };
    QList<LeaderboardEntry> entries_;
};

#endif // LEADERBOARDSCREEN_H

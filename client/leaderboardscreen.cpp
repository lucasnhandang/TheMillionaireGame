#include "leaderboardscreen.h"
#include "protocol_handler.h"
#include "gamestate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QAbstractItemView>
#include <QMap>
#include <algorithm>
#include <iostream>

LeaderboardScreen::LeaderboardScreen(QWidget *parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , demoMode_(false)
    , currentMode_("global")
{
    setupUI();
}

void LeaderboardScreen::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 30);
    mainLayout->setSpacing(20);
    
    // Title and back button
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    titleLabel_ = new QLabel("Leaderboard", this);
    titleLabel_->setStyleSheet(
        "font-size: 36px;"
        "font-weight: bold;"
        "color: white;"
    );
    topLayout->addWidget(titleLabel_);
    
    topLayout->addStretch();
    
    // Mode buttons
    globalModeButton_ = new QPushButton("Global", this);
    globalModeButton_->setCheckable(true);
    globalModeButton_->setChecked(true);
    globalModeButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 25px;"
        "  border-radius: 5px;"
        "  border: none;"
        "  min-width: 100px;"
        "}"
        "QPushButton:checked {"
        "  background-color: #1565C0;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover:!checked {"
        "  background-color: #1976D2;"
        "}"
    );
    connect(globalModeButton_, &QPushButton::clicked, this, &LeaderboardScreen::onModeChanged);
    topLayout->addWidget(globalModeButton_);
    
    friendModeButton_ = new QPushButton("Friends", this);
    friendModeButton_->setCheckable(true);
    friendModeButton_->setChecked(false);
    friendModeButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 25px;"
        "  border-radius: 5px;"
        "  border: none;"
        "  min-width: 100px;"
        "}"
        "QPushButton:checked {"
        "  background-color: #1565C0;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover:!checked {"
        "  background-color: #1976D2;"
        "}"
    );
    connect(friendModeButton_, &QPushButton::clicked, this, &LeaderboardScreen::onModeChanged);
    topLayout->addWidget(friendModeButton_);
    
    refreshButton_ = new QPushButton("Refresh", this);
    refreshButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 25px;"
        "  border-radius: 5px;"
        "  border: none;"
        "  min-width: 100px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45A049;"
        "}"
    );
    connect(refreshButton_, &QPushButton::clicked, this, &LeaderboardScreen::onRefreshClicked);
    topLayout->addWidget(refreshButton_);
    
    backButton_ = new QPushButton("Back to Home", this);
    backButton_->setEnabled(true);  // Ensure button is enabled
    backButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #757575;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 25px;"
        "  border-radius: 5px;"
        "  border: none;"
        "  min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #616161;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #424242;"
        "}"
    );
    connect(backButton_, &QPushButton::clicked, this, &LeaderboardScreen::onBackToHomeClicked);
    topLayout->addWidget(backButton_);
    
    mainLayout->addLayout(topLayout);
    
    // Table
    tableWidget_ = new QTableWidget(this);
    tableWidget_->setColumnCount(4);
    tableWidget_->setHorizontalHeaderLabels(QStringList() << "Rank" << "Username" << "Total Winnings" << "Total Points");
    tableWidget_->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1a1a1a;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  gridline-color: #444;"
        "  font-size: 14px;"
        "}"
        "QTableWidget::item {"
        "  padding: 10px;"
        "  border: none;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #2a5f8f;"
        "}"
        "QHeaderView::section {"
        "  background-color: #2a2a2a;"
        "  color: white;"
        "  padding: 10px;"
        "  border: 1px solid #444;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "}"
    );
    
    tableWidget_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Rank
    tableWidget_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // Username
    tableWidget_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Total Winnings
    tableWidget_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Total Points
    
    tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_->verticalHeader()->setVisible(false);
    tableWidget_->setShowGrid(true);
    
    mainLayout->addWidget(tableWidget_);
    
    setStyleSheet("background-color: #0D1B2A; color: white;");
}

void LeaderboardScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void LeaderboardScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

QString LeaderboardScreen::formatCurrency(long long amount)
{
    if (amount == 0) {
        return "$0";
    }
    
    QString formatted = QString::number(amount);
    
    // Add commas for thousands
    for (int i = formatted.length() - 3; i > 0; i -= 3) {
        formatted.insert(i, ',');
    }
    
    return QString("$%1").arg(formatted);
}

void LeaderboardScreen::loadLeaderboard()
{
    if (!protocol_ && !demoMode_) {
        return;
    }
    
    entries_.clear();
    
    // Always add fake accounts first (for both demo and real mode)
    QList<LeaderboardEntry> fakeEntries;
    fakeEntries.append({QString("player1"), 1000000, 450, 0});
    fakeEntries.append({QString("gamer_pro"), 1000000, 425, 0});
    fakeEntries.append({QString("quiz_master"), 500000, 395, 0});
    fakeEntries.append({QString("player2"), 500000, 380, 0});
    fakeEntries.append({QString("smart_player"), 250000, 375, 0});
    fakeEntries.append({QString("lucky_one"), 125000, 350, 0});
    fakeEntries.append({QString("brain_train"), 64000, 340, 0});
    fakeEntries.append({QString("player3"), 32000, 320, 0});
    fakeEntries.append({QString("knowledge_seeker"), 32000, 335, 0});
    fakeEntries.append({QString("trivia_king"), 32000, 300, 0});
    fakeEntries.append({QString("millionaire_wannabe"), 1000, 280, 0});
    fakeEntries.append({QString("fast_thinker"), 1000, 250, 0});
    fakeEntries.append({QString("quick_answer"), 1000, 220, 0});
    fakeEntries.append({QString("slow_and_steady"), 1000, 200, 0});
    fakeEntries.append({QString("tester"), 1000, 165, 0});
    fakeEntries.append({QString("beginner_pro"), 500, 180, 0});
    fakeEntries.append({QString("newbie_player"), 300, 150, 0});
    fakeEntries.append({QString("just_started"), 200, 120, 0});
    fakeEntries.append({QString("first_timer"), 100, 90, 0});
    fakeEntries.append({QString("trial_user"), 0, 50, 0});
    fakeEntries.append({QString("explorer"), 0, 30, 0});
    fakeEntries.append({QString("admin1"), 0, 0, 0});
    fakeEntries.append({QString("banned_user"), 0, 0, 0});
    
    if (demoMode_) {
        // In demo mode, use only fake accounts
        entries_ = fakeEntries;
    } else {
        // In real mode, get data from server and merge with fake accounts
        ProtocolHandler::LeaderboardResponse response = protocol_->getLeaderboard(
            currentMode_.toStdString(), 1, 100);  // Get top 100
        
        if (response.responseCode == 200) {
            // Convert server entries to local format
            QMap<QString, LeaderboardEntry> serverEntries;  // Use map to avoid duplicates
            
            // Sort server entries first
            std::vector<ProtocolHandler::LeaderboardEntry> sorted = response.rankings;
            std::sort(sorted.begin(), sorted.end(), 
                [](const ProtocolHandler::LeaderboardEntry& a, const ProtocolHandler::LeaderboardEntry& b) {
                    if (a.finalPrize != b.finalPrize) {
                        return a.finalPrize > b.finalPrize;
                    }
                    if (a.totalScore != b.totalScore) {
                        return a.totalScore > b.totalScore;
                    }
                    return a.username < b.username;
                });
            
            // Add server entries (these take precedence over fake entries if username matches)
            for (size_t i = 0; i < sorted.size(); i++) {
                LeaderboardEntry entry;
                entry.username = QString::fromStdString(sorted[i].username);
                entry.totalWinning = sorted[i].finalPrize;
                entry.totalPoints = sorted[i].totalScore;
                entry.rank = 0;  // Will be recalculated later
                serverEntries[entry.username] = entry;
            }
            
            // Merge: Add fake entries that don't exist in server data, or update if server has better data
            for (const LeaderboardEntry& fakeEntry : fakeEntries) {
                if (serverEntries.contains(fakeEntry.username)) {
                    // Use server data if available (it's more accurate)
                    entries_.append(serverEntries[fakeEntry.username]);
                } else {
                    // Add fake entry if not in server data
                    entries_.append(fakeEntry);
                }
            }
            
            // Add any server entries that aren't in fake list
            for (auto it = serverEntries.begin(); it != serverEntries.end(); ++it) {
                bool found = false;
                for (const LeaderboardEntry& fake : fakeEntries) {
                    if (fake.username == it.key()) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    entries_.append(it.value());
                }
            }
        } else {
            // If server fails, use fake accounts as fallback
            std::cerr << "[WARNING] Failed to load leaderboard from server, using fake data. Error code: " 
                      << response.responseCode << std::endl;
            entries_ = fakeEntries;
        }
    }
    
    // Sort all entries together: totalWinning (desc), totalPoints (desc), username (asc)
    std::sort(entries_.begin(), entries_.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.totalWinning != b.totalWinning) {
                return a.totalWinning > b.totalWinning;
            }
            if (a.totalPoints != b.totalPoints) {
                return a.totalPoints > b.totalPoints;
            }
            return a.username < b.username;
        });
    
    // Recalculate ranks after sorting
    for (int i = 0; i < entries_.size(); i++) {
        entries_[i].rank = i + 1;
    }
    
    updateTable();
}

void LeaderboardScreen::updateTable()
{
    tableWidget_->setRowCount(entries_.size());
    
    for (int i = 0; i < entries_.size(); i++) {
        const LeaderboardEntry& entry = entries_[i];
        
        // Rank
        QTableWidgetItem* rankItem = new QTableWidgetItem(QString::number(entry.rank));
        rankItem->setTextAlignment(Qt::AlignCenter);
        tableWidget_->setItem(i, 0, rankItem);
        
        // Username
        QTableWidgetItem* usernameItem = new QTableWidgetItem(entry.username);
        tableWidget_->setItem(i, 1, usernameItem);
        
        // Total Winnings
        QTableWidgetItem* winningItem = new QTableWidgetItem(formatCurrency(entry.totalWinning));
        winningItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tableWidget_->setItem(i, 2, winningItem);
        
        // Total Points
        QTableWidgetItem* pointsItem = new QTableWidgetItem(QString::number(entry.totalPoints));
        pointsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tableWidget_->setItem(i, 3, pointsItem);
    }
}

void LeaderboardScreen::refreshData()
{
    loadLeaderboard();
}

void LeaderboardScreen::onModeChanged()
{
    // Update mode based on which button is checked
    if (globalModeButton_->isChecked()) {
        currentMode_ = "global";
        friendModeButton_->setChecked(false);
    } else if (friendModeButton_->isChecked()) {
        currentMode_ = "friend";
        globalModeButton_->setChecked(false);
    }
    
    // Reload leaderboard
    loadLeaderboard();
}

void LeaderboardScreen::onRefreshClicked()
{
    loadLeaderboard();
}

void LeaderboardScreen::onBackToHomeClicked()
{
    // Debug: Ensure signal is emitted
    std::cerr << "[DEBUG] LeaderboardScreen::onBackToHomeClicked() called, emitting backToHome signal" << std::endl;
    emit backToHome();
}

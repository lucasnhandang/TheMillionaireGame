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
    
    if (demoMode_) {
        // Demo data
        entries_.append({QString("player1"), 1000000, 450, 1});
        entries_.append({QString("player2"), 500000, 380, 2});
        entries_.append({QString("player3"), 32000, 320, 3});
    } else {
        // Get leaderboard from server
        ProtocolHandler::LeaderboardResponse response = protocol_->getLeaderboard(
            currentMode_.toStdString(), 1, 100);  // Get top 100
        
        if (response.responseCode == 200) {
            // Sort by: totalWinning (desc), totalPoints (desc), username (asc)
            std::vector<ProtocolHandler::LeaderboardEntry> sorted = response.rankings;
            std::sort(sorted.begin(), sorted.end(), 
                [](const ProtocolHandler::LeaderboardEntry& a, const ProtocolHandler::LeaderboardEntry& b) {
                    // Primary: totalWinning (finalPrize) descending
                    if (a.finalPrize != b.finalPrize) {
                        return a.finalPrize > b.finalPrize;
                    }
                    // Secondary: totalScore descending
                    if (a.totalScore != b.totalScore) {
                        return a.totalScore > b.totalScore;
                    }
                    // Tertiary: username ascending (alphabetical)
                    return a.username < b.username;
                });
            
            // Convert to local entries
            for (size_t i = 0; i < sorted.size(); i++) {
                LeaderboardEntry entry;
                entry.username = QString::fromStdString(sorted[i].username);
                entry.totalWinning = sorted[i].finalPrize;
                entry.totalPoints = sorted[i].totalScore;
                entry.rank = i + 1;  // Recalculate rank based on sorted order
                entries_.append(entry);
            }
        } else {
            QMessageBox::warning(this, "Error", 
                QString("Failed to load leaderboard. Error code: %1").arg(response.responseCode));
        }
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
    emit backToHome();
}

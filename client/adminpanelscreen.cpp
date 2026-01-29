#include "adminpanelscreen.h"
#include "protocol_handler.h"
#include "addquestiondialog.h"
#include "editquestiondialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

AdminPanelScreen::AdminPanelScreen(QWidget *parent)
    : QWidget(parent)
    , protocol_(nullptr)
    , demoMode_(false)
    , currentUsersPage_(1)
    , totalUsersPages_(1)
    , totalUsers_(0)
    , currentQuestionsPage_(1)
    , totalQuestionsPages_(1)
    , totalQuestions_(0)
{
    setupUI();
}

void AdminPanelScreen::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);
    
    // Title
    QLabel* titleLabel = new QLabel("Admin Panel", this);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: white;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Tab widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 2px solid #444;"
        "  background-color: #2b2b2b;"
        "  border-radius: 5px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #3b3b3b;"
        "  color: white;"
        "  padding: 10px 30px;"
        "  margin-right: 2px;"
        "  border-top-left-radius: 5px;"
        "  border-top-right-radius: 5px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #1E88E5;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: #555;"
        "}"
    );
    
    setupUsersTab();
    setupQuestionsTab();
    
    tabWidget_->addTab(usersTab_, "Users");
    tabWidget_->addTab(questionsTab_, "Questions");
    
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) onUsersTabSelected();
        else if (index == 1) onQuestionsTabSelected();
    });
    
    mainLayout->addWidget(tabWidget_);
    
    // Back button
    backButton_ = new QPushButton("Back to Home", this);
    backButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #666;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 40px;"
        "  border-radius: 8px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #777;"
        "}"
    );
    connect(backButton_, &QPushButton::clicked, this, &AdminPanelScreen::onBackClicked);
    
    QHBoxLayout* backLayout = new QHBoxLayout();
    backLayout->addStretch();
    backLayout->addWidget(backButton_);
    backLayout->addStretch();
    mainLayout->addLayout(backLayout);
}

void AdminPanelScreen::setupUsersTab()
{
    usersTab_ = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(usersTab_);
    layout->setSpacing(15);
    
    // Table
    usersTable_ = new QTableWidget(0, 7, usersTab_);
    usersTable_->setHorizontalHeaderLabels({"Username", "Role", "Status", "Total Games", "Highest Prize", "", ""});
    usersTable_->horizontalHeader()->setStretchLastSection(false);
    usersTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    usersTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    usersTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    usersTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    usersTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    usersTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    usersTable_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    usersTable_->setColumnWidth(1, 80);
    usersTable_->setColumnWidth(2, 90);
    usersTable_->setColumnWidth(3, 110);
    usersTable_->setColumnWidth(4, 130);
    usersTable_->setColumnWidth(5, 100);
    usersTable_->setColumnWidth(6, 80);
    usersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    usersTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersTable_->setAlternatingRowColors(false);
    usersTable_->verticalHeader()->setVisible(false);
    usersTable_->verticalHeader()->setDefaultSectionSize(50);
    usersTable_->setShowGrid(true);
    usersTable_->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1a1a2e;"
        "  color: #eee;"
        "  gridline-color: #333;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "}"
        "QTableWidget::item {"
        "  padding: 12px 8px;"
        "  border-bottom: 1px solid #333;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #0f3460;"
        "}"
        "QHeaderView::section {"
        "  background-color: #16213e;"
        "  color: #00d4ff;"
        "  padding: 12px 8px;"
        "  border: none;"
        "  border-bottom: 2px solid #00d4ff;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
    );
    layout->addWidget(usersTable_);
    
    // Pagination controls
    QHBoxLayout* paginationLayout = new QHBoxLayout();
    paginationLayout->addStretch();
    
    usersPrevButton_ = new QPushButton("◀ Previous", usersTab_);
    usersPrevButton_->setStyleSheet(
        "QPushButton { background-color: #1E88E5; color: white; padding: 8px 20px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(usersPrevButton_, &QPushButton::clicked, this, &AdminPanelScreen::onUsersPrevPage);
    paginationLayout->addWidget(usersPrevButton_);
    
    usersPageLabel_ = new QLabel("Page 1 of 1", usersTab_);
    usersPageLabel_->setStyleSheet("color: white; font-size: 14px; padding: 0 20px;");
    paginationLayout->addWidget(usersPageLabel_);
    
    usersNextButton_ = new QPushButton("Next ▶", usersTab_);
    usersNextButton_->setStyleSheet(
        "QPushButton { background-color: #1E88E5; color: white; padding: 8px 20px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(usersNextButton_, &QPushButton::clicked, this, &AdminPanelScreen::onUsersNextPage);
    paginationLayout->addWidget(usersNextButton_);
    
    paginationLayout->addStretch();
    layout->addLayout(paginationLayout);
}

void AdminPanelScreen::setupQuestionsTab()
{
    questionsTab_ = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(questionsTab_);
    layout->setSpacing(15);
    
    // Add Question button
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addStretch();
    
    addQuestionButton_ = new QPushButton("➕ Add New Question", questionsTab_);
    addQuestionButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 16px;"
        "  padding: 10px 30px;"
        "  border-radius: 8px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45A049;"
        "}"
    );
    connect(addQuestionButton_, &QPushButton::clicked, this, &AdminPanelScreen::onAddQuestion);
    topLayout->addWidget(addQuestionButton_);
    
    layout->addLayout(topLayout);
    
    // Table
    questionsTable_ = new QTableWidget(0, 5, questionsTab_);
    questionsTable_->setHorizontalHeaderLabels({"ID", "Question", "Level", "", ""});
    questionsTable_->horizontalHeader()->setStretchLastSection(false);
    questionsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    questionsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    questionsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    questionsTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    questionsTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    questionsTable_->setColumnWidth(0, 60);
    questionsTable_->setColumnWidth(2, 70);
    questionsTable_->setColumnWidth(3, 90);
    questionsTable_->setColumnWidth(4, 90);
    questionsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    questionsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    questionsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    questionsTable_->setAlternatingRowColors(false);
    questionsTable_->verticalHeader()->setVisible(false);
    questionsTable_->verticalHeader()->setDefaultSectionSize(50);
    questionsTable_->setShowGrid(true);
    questionsTable_->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1a1a2e;"
        "  color: #eee;"
        "  gridline-color: #333;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "}"
        "QTableWidget::item {"
        "  padding: 12px 8px;"
        "  border-bottom: 1px solid #333;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #0f3460;"
        "}"
        "QHeaderView::section {"
        "  background-color: #16213e;"
        "  color: #00d4ff;"
        "  padding: 12px 8px;"
        "  border: none;"
        "  border-bottom: 2px solid #00d4ff;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
    );
    layout->addWidget(questionsTable_);
    
    // Pagination controls
    QHBoxLayout* paginationLayout = new QHBoxLayout();
    paginationLayout->addStretch();
    
    questionsPrevButton_ = new QPushButton("◀ Previous", questionsTab_);
    questionsPrevButton_->setStyleSheet(
        "QPushButton { background-color: #1E88E5; color: white; padding: 8px 20px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(questionsPrevButton_, &QPushButton::clicked, this, &AdminPanelScreen::onQuestionsPrevPage);
    paginationLayout->addWidget(questionsPrevButton_);
    
    questionsPageLabel_ = new QLabel("Page 1 of 1", questionsTab_);
    questionsPageLabel_->setStyleSheet("color: white; font-size: 14px; padding: 0 20px;");
    paginationLayout->addWidget(questionsPageLabel_);
    
    questionsNextButton_ = new QPushButton("Next ▶", questionsTab_);
    questionsNextButton_->setStyleSheet(
        "QPushButton { background-color: #1E88E5; color: white; padding: 8px 20px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:disabled { background-color: #555; }"
    );
    connect(questionsNextButton_, &QPushButton::clicked, this, &AdminPanelScreen::onQuestionsNextPage);
    paginationLayout->addWidget(questionsNextButton_);
    
    paginationLayout->addStretch();
    layout->addLayout(paginationLayout);
}

void AdminPanelScreen::setProtocolHandler(ProtocolHandler* protocol)
{
    protocol_ = protocol;
}

void AdminPanelScreen::setDemoMode(bool demoMode)
{
    demoMode_ = demoMode;
}

void AdminPanelScreen::loadUsers(int page)
{
    currentUsersPage_ = page;
    
    if (demoMode_) {
        // Mock data for demo mode
        totalUsers_ = 25;
        totalUsersPages_ = 3;
        updateUsersTable();
        updateUsersPagination();
        return;
    }
    
    if (!protocol_) {
        usersTable_->setRowCount(1);
        usersTable_->setItem(0, 0, new QTableWidgetItem("Error: Not connected"));
        return;
    }
    
    // Call protocol to get users
    ProtocolHandler::ViewUsersResponse response = protocol_->viewUsers(page, 10);
    
    if (response.responseCode != 200) {
        usersTable_->setRowCount(1);
        usersTable_->setItem(0, 0, new QTableWidgetItem(QString("Error: Code %1").arg(response.responseCode)));
        return;
    }
    
    // Update pagination info
    totalUsers_ = response.total;
    totalUsersPages_ = (totalUsers_ + 9) / 10;  // Ceiling division
    
    // Clear and populate table
    usersTable_->setRowCount(0);
    for (const auto& user : response.users) {
        int row = usersTable_->rowCount();
        usersTable_->insertRow(row);
        usersTable_->setRowHeight(row, 50);
        
        QTableWidgetItem* usernameItem = new QTableWidgetItem(QString::fromStdString(user.username));
        usernameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        usersTable_->setItem(row, 0, usernameItem);
        
        QTableWidgetItem* roleItem = new QTableWidgetItem(QString::fromStdString(user.role));
        roleItem->setTextAlignment(Qt::AlignCenter);
        usersTable_->setItem(row, 1, roleItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(user.isBanned ? "Banned" : "Active");
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (user.isBanned) {
            statusItem->setForeground(QBrush(QColor("#ff6b6b")));
        } else {
            statusItem->setForeground(QBrush(QColor("#51cf66")));
        }
        usersTable_->setItem(row, 2, statusItem);
        
        QTableWidgetItem* gamesItem = new QTableWidgetItem(QString::number(user.totalGames));
        gamesItem->setTextAlignment(Qt::AlignCenter);
        usersTable_->setItem(row, 3, gamesItem);
        
        QTableWidgetItem* prizeItem = new QTableWidgetItem(QString::number(user.highestPrize));
        prizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        usersTable_->setItem(row, 4, prizeItem);
        
        // Action buttons with better styling
        bool isAdmin = (user.role == "admin");
        QPushButton* actionBtn = new QPushButton(isAdmin ? "Revoke" : "Promote", usersTable_);
        actionBtn->setMinimumSize(85, 32);
        actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #e67e22;"
            "  color: white;"
            "  font-size: 12px;"
            "  font-weight: bold;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  border: none;"
            "}"
            "QPushButton:hover { background-color: #d35400; }"
        );
        QString username = QString::fromStdString(user.username);
        connect(actionBtn, &QPushButton::clicked, this, [this, username, isAdmin]() {
            if (isAdmin) onRevokeAdmin(username);
            else onPromoteUser(username);
        });
        usersTable_->setCellWidget(row, 5, actionBtn);
        
        QPushButton* banBtn = new QPushButton("Ban", usersTable_);
        banBtn->setMinimumSize(70, 32);
        banBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #e74c3c;"
            "  color: white;"
            "  font-size: 12px;"
            "  font-weight: bold;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  border: none;"
            "}"
            "QPushButton:hover { background-color: #c0392b; }"
            "QPushButton:disabled { background-color: #555; color: #888; }"
        );
        connect(banBtn, &QPushButton::clicked, this, [this, username]() {
            onBanUser(username);
        });
        if (user.isBanned) banBtn->setEnabled(false);
        usersTable_->setCellWidget(row, 6, banBtn);
    }
    
    updateUsersPagination();
}

void AdminPanelScreen::loadQuestions(int page)
{
    currentQuestionsPage_ = page;
    
    if (demoMode_) {
        // Mock data for demo mode
        totalQuestions_ = 50;
        totalQuestionsPages_ = 5;
        updateQuestionsTable();
        updateQuestionsPagination();
        return;
    }
    
    if (!protocol_) {
        questionsTable_->setRowCount(1);
        questionsTable_->setItem(0, 0, new QTableWidgetItem("Error: Not connected"));
        return;
    }
    
    // Call protocol to get questions
    ProtocolHandler::ViewQuestionsResponse response = protocol_->viewQuestions(page, 10);
    
    if (response.responseCode != 200) {
        questionsTable_->setRowCount(1);
        questionsTable_->setItem(0, 0, new QTableWidgetItem(QString("Error: Code %1").arg(response.responseCode)));
        return;
    }
    
    // Update pagination info
    totalQuestions_ = response.total;
    totalQuestionsPages_ = (totalQuestions_ + 9) / 10;  // Ceiling division
    
    // Clear and populate table
    questionsTable_->setRowCount(0);
    for (const auto& question : response.questions) {
        int row = questionsTable_->rowCount();
        questionsTable_->insertRow(row);
        questionsTable_->setRowHeight(row, 50);
        
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(question.questionId));
        idItem->setTextAlignment(Qt::AlignCenter);
        questionsTable_->setItem(row, 0, idItem);
        
        // Truncate long questions for table display
        QString questionPreview = QString::fromStdString(question.question);
        if (questionPreview.length() > 80) {
            questionPreview = questionPreview.left(77) + "...";
        }
        QTableWidgetItem* questionItem = new QTableWidgetItem(questionPreview);
        questionItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        questionsTable_->setItem(row, 1, questionItem);
        
        QTableWidgetItem* levelItem = new QTableWidgetItem(QString::number(question.level));
        levelItem->setTextAlignment(Qt::AlignCenter);
        questionsTable_->setItem(row, 2, levelItem);
        
        // Edit button
        int questionId = question.questionId;
        QPushButton* editBtn = new QPushButton("Edit", questionsTable_);
        editBtn->setMinimumSize(75, 32);
        editBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #3498db;"
            "  color: white;"
            "  font-size: 12px;"
            "  font-weight: bold;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  border: none;"
            "}"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        connect(editBtn, &QPushButton::clicked, this, [this, questionId]() {
            onEditQuestion(questionId);
        });
        questionsTable_->setCellWidget(row, 3, editBtn);
        
        // Delete button
        QPushButton* deleteBtn = new QPushButton("Delete", questionsTable_);
        deleteBtn->setMinimumSize(75, 32);
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #e74c3c;"
            "  color: white;"
            "  font-size: 12px;"
            "  font-weight: bold;"
            "  padding: 6px 12px;"
            "  border-radius: 6px;"
            "  border: none;"
            "}"
            "QPushButton:hover { background-color: #c0392b; }"
        );
        connect(deleteBtn, &QPushButton::clicked, this, [this, questionId]() {
            onDeleteQuestion(questionId);
        });
        questionsTable_->setCellWidget(row, 4, deleteBtn);
    }
    
    updateQuestionsPagination();
}

void AdminPanelScreen::updateUsersTable()
{
    // Clear existing rows
    usersTable_->setRowCount(0);
    
    if (demoMode_) {
        // Add mock users for demo
        for (int i = 0; i < 10; i++) {
            int row = usersTable_->rowCount();
            usersTable_->insertRow(row);
            usersTable_->setRowHeight(row, 50);
            
            QTableWidgetItem* usernameItem = new QTableWidgetItem(QString("user%1").arg(i + 1));
            usernameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            usersTable_->setItem(row, 0, usernameItem);
            
            QTableWidgetItem* roleItem = new QTableWidgetItem(i == 0 ? "admin" : "user");
            roleItem->setTextAlignment(Qt::AlignCenter);
            usersTable_->setItem(row, 1, roleItem);
            
            QTableWidgetItem* statusItem = new QTableWidgetItem(i == 5 ? "Banned" : "Active");
            statusItem->setTextAlignment(Qt::AlignCenter);
            if (i == 5) {
                statusItem->setForeground(QBrush(QColor("#ff6b6b")));
            } else {
                statusItem->setForeground(QBrush(QColor("#51cf66")));
            }
            usersTable_->setItem(row, 2, statusItem);
            
            QTableWidgetItem* gamesItem = new QTableWidgetItem(QString::number(i * 3));
            gamesItem->setTextAlignment(Qt::AlignCenter);
            usersTable_->setItem(row, 3, gamesItem);
            
            QTableWidgetItem* prizeItem = new QTableWidgetItem(QString::number(i * 1000000));
            prizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            usersTable_->setItem(row, 4, prizeItem);
            
            // Action buttons
            QPushButton* actionBtn = new QPushButton(i == 0 ? "Revoke" : "Promote", usersTable_);
            actionBtn->setMinimumSize(85, 32);
            actionBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #e67e22;"
                "  color: white;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 6px;"
                "  border: none;"
                "}"
                "QPushButton:hover { background-color: #d35400; }"
            );
            QString username = QString("user%1").arg(i + 1);
            bool isAdmin = (i == 0);
            connect(actionBtn, &QPushButton::clicked, this, [this, username, isAdmin]() {
                if (isAdmin) onRevokeAdmin(username);
                else onPromoteUser(username);
            });
            usersTable_->setCellWidget(row, 5, actionBtn);
            
            QPushButton* banBtn = new QPushButton("Ban", usersTable_);
            banBtn->setMinimumSize(70, 32);
            banBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #e74c3c;"
                "  color: white;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 6px;"
                "  border: none;"
                "}"
                "QPushButton:hover { background-color: #c0392b; }"
                "QPushButton:disabled { background-color: #555; color: #888; }"
            );
            connect(banBtn, &QPushButton::clicked, this, [this, username]() {
                onBanUser(username);
            });
            if (i == 5) banBtn->setEnabled(false); // Already banned
            usersTable_->setCellWidget(row, 6, banBtn);
        }
    }
}

void AdminPanelScreen::updateQuestionsTable()
{
    // Clear existing rows
    questionsTable_->setRowCount(0);
    
    if (demoMode_) {
        // Add mock questions for demo
        for (int i = 0; i < 10; i++) {
            int row = questionsTable_->rowCount();
            questionsTable_->insertRow(row);
            questionsTable_->setRowHeight(row, 50);
            
            int questionId = (currentQuestionsPage_ - 1) * 10 + i + 1;
            
            QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(questionId));
            idItem->setTextAlignment(Qt::AlignCenter);
            questionsTable_->setItem(row, 0, idItem);
            
            QTableWidgetItem* questionItem = new QTableWidgetItem(QString("Sample question %1...").arg(questionId));
            questionItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            questionsTable_->setItem(row, 1, questionItem);
            
            QTableWidgetItem* levelItem = new QTableWidgetItem(QString::number((i % 3)));
            levelItem->setTextAlignment(Qt::AlignCenter);
            questionsTable_->setItem(row, 2, levelItem);
            
            // Edit button
            QPushButton* editBtn = new QPushButton("Edit", questionsTable_);
            editBtn->setMinimumSize(75, 32);
            editBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #3498db;"
                "  color: white;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 6px;"
                "  border: none;"
                "}"
                "QPushButton:hover { background-color: #2980b9; }"
            );
            connect(editBtn, &QPushButton::clicked, this, [this, questionId]() {
                onEditQuestion(questionId);
            });
            questionsTable_->setCellWidget(row, 3, editBtn);
            
            // Delete button
            QPushButton* deleteBtn = new QPushButton("Delete", questionsTable_);
            deleteBtn->setMinimumSize(75, 32);
            deleteBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #e74c3c;"
                "  color: white;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 6px;"
                "  border: none;"
                "}"
                "QPushButton:hover { background-color: #c0392b; }"
            );
            connect(deleteBtn, &QPushButton::clicked, this, [this, questionId]() {
                onDeleteQuestion(questionId);
            });
            questionsTable_->setCellWidget(row, 4, deleteBtn);
        }
    }
}

void AdminPanelScreen::updateUsersPagination()
{
    usersPageLabel_->setText(QString("Page %1 of %2 (%3 total users)")
                             .arg(currentUsersPage_)
                             .arg(totalUsersPages_)
                             .arg(totalUsers_));
    usersPrevButton_->setEnabled(currentUsersPage_ > 1);
    usersNextButton_->setEnabled(currentUsersPage_ < totalUsersPages_);
}

void AdminPanelScreen::updateQuestionsPagination()
{
    questionsPageLabel_->setText(QString("Page %1 of %2 (%3 total questions)")
                                 .arg(currentQuestionsPage_)
                                 .arg(totalQuestionsPages_)
                                 .arg(totalQuestions_));
    questionsPrevButton_->setEnabled(currentQuestionsPage_ > 1);
    questionsNextButton_->setEnabled(currentQuestionsPage_ < totalQuestionsPages_);
}

void AdminPanelScreen::styleMessageBox(QMessageBox& msgBox, const QString& buttonColor)
{
    msgBox.setStyleSheet(
        "QMessageBox {"
        "  background-color: #1a1a1a;"
        "  color: white;"
        "  border: none;"
        "}"
        "QMessageBox QLabel {"
        "  color: white;"
        "  font-size: 14px;"
        "}"
    );
    
    // Style all buttons
    QAbstractButton* okBtn = msgBox.button(QMessageBox::Ok);
    if (okBtn) {
        okBtn->setStyleSheet(
            QString(
                "QPushButton {"
                "  background-color: %1;"
                "  color: white;"
                "  border: none;"
                "  padding: 10px 20px;"
                "  border-radius: 5px;"
                "  min-width: 100px;"
                "  font-size: 14px;"
                "  font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "  background-color: %2;"
                "}"
                "QPushButton:pressed {"
                "  background-color: %3;"
                "}"
            ).arg(buttonColor)
            .arg(buttonColor == "#4CAF50" ? "#45A049" : (buttonColor == "#d32f2f" ? "#e53935" : "#1976D2"))
            .arg(buttonColor == "#4CAF50" ? "#388E3C" : (buttonColor == "#d32f2f" ? "#b71c1c" : "#1565C0"))
        );
    }
    
    QAbstractButton* yesBtn = msgBox.button(QMessageBox::Yes);
    if (yesBtn) {
        yesBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #1E88E5;"
            "  color: white;"
            "  border: none;"
            "  padding: 6px 20px;"
            "  border-radius: 5px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #1976D2;"
            "}"
        );
    }
    
    QAbstractButton* noBtn = msgBox.button(QMessageBox::No);
    if (noBtn) {
        noBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #555555;"
            "  color: white;"
            "  border: none;"
            "  padding: 6px 20px;"
            "  border-radius: 5px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #666666;"
            "}"
        );
    }
}

void AdminPanelScreen::onBackClicked()
{
    emit backToHome();
}

void AdminPanelScreen::onUsersTabSelected()
{
    loadUsers(currentUsersPage_);
}

void AdminPanelScreen::onQuestionsTabSelected()
{
    loadQuestions(currentQuestionsPage_);
}

void AdminPanelScreen::onUsersPrevPage()
{
    if (currentUsersPage_ > 1) {
        loadUsers(currentUsersPage_ - 1);
    }
}

void AdminPanelScreen::onUsersNextPage()
{
    if (currentUsersPage_ < totalUsersPages_) {
        loadUsers(currentUsersPage_ + 1);
    }
}

void AdminPanelScreen::onPromoteUser(const QString& username)
{
    // Custom styled confirmation dialog to ensure text is visible on dark theme
    QMessageBox promoteBox(this);
    promoteBox.setIcon(QMessageBox::Question);
    promoteBox.setWindowTitle("Promote User");
    promoteBox.setText(QString("Promote %1 to admin?").arg(username));
    promoteBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    promoteBox.setDefaultButton(QMessageBox::No);
    styleMessageBox(promoteBox);
    QMessageBox::StandardButton reply =
        static_cast<QMessageBox::StandardButton>(promoteBox.exec());
    if (reply != QMessageBox::Yes)
        return;
    
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    int code = protocol_->promoteUser(username.toStdString());
    if (code == 200) {
        QMessageBox successBox(this);
        successBox.setWindowTitle("Success");
        successBox.setText(QString("%1 promoted to admin successfully").arg(username));
        successBox.setIcon(QMessageBox::Information);
        successBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(successBox, "#4CAF50");
        successBox.exec();
        loadUsers(currentUsersPage_);  // Reload current page
    } else if (code == 403) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Access forbidden - you are not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 404) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("User not found");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 409) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("User is already an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText(QString("Failed to promote user (code %1)").arg(code));
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    }
}

void AdminPanelScreen::onRevokeAdmin(const QString& username)
{
    // Custom styled confirmation dialog to ensure text is visible on dark theme
    QMessageBox revokeBox(this);
    revokeBox.setIcon(QMessageBox::Question);
    revokeBox.setWindowTitle("Revoke Admin");
    revokeBox.setText(QString("Revoke admin rights from %1?").arg(username));
    revokeBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    revokeBox.setDefaultButton(QMessageBox::No);
    styleMessageBox(revokeBox);
    QMessageBox::StandardButton reply =
        static_cast<QMessageBox::StandardButton>(revokeBox.exec());
    if (reply != QMessageBox::Yes)
        return;
    
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    int code = protocol_->revokeAdmin(username.toStdString());
    if (code == 200) {
        QMessageBox successBox(this);
        successBox.setWindowTitle("Success");
        successBox.setText(QString("Admin rights revoked from %1").arg(username));
        successBox.setIcon(QMessageBox::Information);
        successBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(successBox, "#4CAF50");
        successBox.exec();
        loadUsers(currentUsersPage_);  // Reload current page
    } else if (code == 403) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Access forbidden - you are not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 404) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("User not found");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 409) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("User is not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 422) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Cannot revoke your own admin rights");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText(QString("Failed to revoke admin (code %1)").arg(code));
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    }
}

void AdminPanelScreen::onBanUser(const QString& username)
{
    // Custom, themed input dialog for ban reason
    QInputDialog reasonDialog(this);
    reasonDialog.setWindowTitle("Ban User");
    reasonDialog.setLabelText(QString("Enter reason for banning %1:").arg(username));
    reasonDialog.setInputMode(QInputDialog::TextInput);
    reasonDialog.setTextValue("");
    reasonDialog.setStyleSheet(
        "QInputDialog {"
        "  background-color: #0D1B2A;"
        "  color: white;"
        "}"
        "QLabel {"
        "  color: white;"
        "}"
        "QLineEdit {"
        "  background-color: #1a1a2e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  border-radius: 4px;"
        "}"
        "QPushButton {"
        "  background-color: #1E88E5;"
        "  color: white;"
        "  padding: 6px 20px;"
        "  border-radius: 5px;"
        "  border: none;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
    );
    if (reasonDialog.exec() != QDialog::Accepted)
        return;
    QString reason = reasonDialog.textValue().trimmed();
    if (reason.isEmpty())
        return;
    
    // Custom styled confirmation dialog to ensure text is visible on dark theme
    QMessageBox banBox(this);
    banBox.setIcon(QMessageBox::Question);
    banBox.setWindowTitle("Confirm Ban");
    banBox.setText(QString("Ban user %1?").arg(username));
    banBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    banBox.setDefaultButton(QMessageBox::No);
    styleMessageBox(banBox);
    
    // Override Yes button with red color for ban
    QAbstractButton* yesBtn = banBox.button(QMessageBox::Yes);
    if (yesBtn) {
        yesBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #E53935;"
            "  color: white;"
            "  border: none;"
            "  padding: 6px 20px;"
            "  border-radius: 5px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #C62828;"
            "}"
        );
    }
    QMessageBox::StandardButton reply =
        static_cast<QMessageBox::StandardButton>(banBox.exec());
    if (reply != QMessageBox::Yes)
        return;
    
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    int code = protocol_->banUser(username.toStdString(), reason.toStdString());
    if (code == 200) {
        QMessageBox successBox(this);
        successBox.setWindowTitle("Success");
        successBox.setText(QString("User %1 banned successfully").arg(username));
        successBox.setIcon(QMessageBox::Information);
        successBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(successBox, "#4CAF50");
        successBox.exec();
        loadUsers(currentUsersPage_);  // Reload current page
    } else if (code == 403) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Access forbidden - you are not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 404) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("User not found");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 422) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Cannot ban yourself");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText(QString("Failed to ban user (code %1)").arg(code));
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    }
}

void AdminPanelScreen::onQuestionsPrevPage()
{
    if (currentQuestionsPage_ > 1) {
        loadQuestions(currentQuestionsPage_ - 1);
    }
}

void AdminPanelScreen::onQuestionsNextPage()
{
    if (currentQuestionsPage_ < totalQuestionsPages_) {
        loadQuestions(currentQuestionsPage_ + 1);
    }
}

void AdminPanelScreen::onAddQuestion()
{
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    AddQuestionDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Build request
        ProtocolHandler::AddQuestionRequest req;
        req.question = dialog.getQuestionText().toStdString();
        
        QStringList options = dialog.getOptions();
        for (const QString& opt : options) {
            req.options.push_back(opt.toStdString());
        }
        
        req.correctAnswer = dialog.getCorrectAnswer();
        req.level = dialog.getLevel();
        req.lifeline_5050_info = dialog.getLifeline5050Info().toStdString();
        req.lifeline_ask_info = dialog.getLifelineAskInfo().toStdString();
        req.lifeline_call_info = dialog.getLifelineCallInfo().toStdString();
        
        // Send request
        ProtocolHandler::AddQuestionResponse response = protocol_->addQuestion(req);
        
        if (response.responseCode == 200) {
            QMessageBox successBox(this);
            successBox.setWindowTitle("Success");
            successBox.setText("Question added successfully");
            successBox.setInformativeText(QString("Question ID: %1").arg(response.questionId));
            successBox.setIcon(QMessageBox::Information);
            successBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(successBox, "#4CAF50");
            successBox.exec();
            loadQuestions(currentQuestionsPage_);  // Reload current page
        } else if (response.responseCode == 403) {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Access forbidden");
            errorBox.setInformativeText("You are not an admin");
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        } else if (response.responseCode == 422) {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Invalid data");
            errorBox.setInformativeText(QString::fromStdString(response.message));
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        } else {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Failed to add question");
            errorBox.setInformativeText(QString("Error code: %1").arg(response.responseCode));
            errorBox.setIcon(QMessageBox::Critical);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        }
    }
}

void AdminPanelScreen::onEditQuestion(int questionId)
{
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    // Fetch full question details from server
    ProtocolHandler::QuestionDetail detail = protocol_->getQuestionDetail(questionId);
    
    if (detail.responseCode == 404) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Question not found (it may have been deleted)");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        loadQuestions(currentQuestionsPage_);
        return;
    } else if (detail.responseCode == 403) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Access forbidden - you are not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    } else if (detail.responseCode != 200) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText(QString("Failed to load question details (code %1)").arg(detail.responseCode));
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    // Open edit dialog pre-populated with current data
    EditQuestionDialog dialog(questionId, this);
    
    QStringList options;
    if (detail.options.size() >= 4) {
        options << QString::fromStdString(detail.options[0])
                << QString::fromStdString(detail.options[1])
                << QString::fromStdString(detail.options[2])
                << QString::fromStdString(detail.options[3]);
    } else {
        options << "" << "" << "" << "";
    }
    
    dialog.setQuestionData(QString::fromStdString(detail.question),
                           options,
                           detail.correctAnswer,
                           detail.level,
                           QString::fromStdString(detail.lifeline_5050_info),
                           QString::fromStdString(detail.lifeline_ask_info),
                           QString::fromStdString(detail.lifeline_call_info));
    
    if (dialog.exec() == QDialog::Accepted) {
        // Gather updated data
        std::string newQuestion = dialog.getQuestionText().toStdString();
        QStringList newOptions = dialog.getOptions();
        std::vector<std::string> optionsVec;
        for (const QString& opt : newOptions) {
            optionsVec.push_back(opt.toStdString());
        }
        int correctAnswer = dialog.getCorrectAnswer();
        
        // Get lifeline data
        QString lifeline5050 = dialog.getLifeline5050Info();
        QString lifelineAsk = dialog.getLifelineAskInfo();
        QString lifelineCall = dialog.getLifelineCallInfo();
        
        int code = protocol_->changeQuestion(questionId, newQuestion, optionsVec, correctAnswer,
                                            lifeline5050.toStdString(),
                                            lifelineAsk.toStdString(),
                                            lifelineCall.toStdString());
        if (code == 200) {
            QMessageBox successBox(this);
            successBox.setWindowTitle("Success");
            successBox.setText("Question updated successfully");
            successBox.setIcon(QMessageBox::Information);
            successBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(successBox, "#4CAF50");
            successBox.exec();
            loadQuestions(currentQuestionsPage_);
        } else if (code == 404) {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Question not found");
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
            loadQuestions(currentQuestionsPage_);
        } else if (code == 422) {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Invalid question data");
            errorBox.setInformativeText("Check correct answer or options");
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        } else if (code == 403) {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText("Access forbidden - you are not an admin");
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        } else {
            QMessageBox errorBox(this);
            errorBox.setWindowTitle("Error");
            errorBox.setText(QString("Failed to update question (code %1)").arg(code));
            errorBox.setIcon(QMessageBox::Warning);
            errorBox.setStandardButtons(QMessageBox::Ok);
            styleMessageBox(errorBox, "#d32f2f");
            errorBox.exec();
        }
    }
}

void AdminPanelScreen::onDeleteQuestion(int questionId)
{
    // Custom styled confirmation dialog to ensure text is visible on dark theme
    QMessageBox deleteBox(this);
    deleteBox.setIcon(QMessageBox::Question);
    deleteBox.setWindowTitle("Delete Question");
    deleteBox.setText(QString("Delete question ID: %1?\n"
                              "This action cannot be undone.").arg(questionId));
    deleteBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    deleteBox.setDefaultButton(QMessageBox::No);
    styleMessageBox(deleteBox);
    
    // Override Yes button with red color for delete
    QAbstractButton* yesBtn = deleteBox.button(QMessageBox::Yes);
    if (yesBtn) {
        yesBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #E53935;"
            "  color: white;"
            "  border: none;"
            "  padding: 6px 20px;"
            "  border-radius: 5px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #C62828;"
            "}"
        );
    }
    QMessageBox::StandardButton reply =
        static_cast<QMessageBox::StandardButton>(deleteBox.exec());
    if (reply != QMessageBox::Yes)
        return;
    
    if (!protocol_) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Not connected to server");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
        return;
    }
    
    int code = protocol_->deleteQuestion(questionId);
    if (code == 200) {
        QMessageBox successBox(this);
        successBox.setWindowTitle("Success");
        successBox.setText(QString("Question %1 deleted successfully").arg(questionId));
        successBox.setIcon(QMessageBox::Information);
        successBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(successBox, "#4CAF50");
        successBox.exec();
        loadQuestions(currentQuestionsPage_);  // Reload current page
    } else if (code == 403) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Access forbidden - you are not an admin");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else if (code == 404) {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText("Question not found");
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    } else {
        QMessageBox errorBox(this);
        errorBox.setWindowTitle("Error");
        errorBox.setText(QString("Failed to delete question (code %1)").arg(code));
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setStandardButtons(QMessageBox::Ok);
        styleMessageBox(errorBox, "#d32f2f");
        errorBox.exec();
    }
}


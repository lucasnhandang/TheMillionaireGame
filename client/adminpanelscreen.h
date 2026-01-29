#ifndef ADMINPANELSCREEN_H
#define ADMINPANELSCREEN_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

class ProtocolHandler;

class AdminPanelScreen : public QWidget
{
    Q_OBJECT

public:
    explicit AdminPanelScreen(QWidget *parent = nullptr);

    void setProtocolHandler(ProtocolHandler* protocol);
    void setDemoMode(bool demoMode);
    void loadUsers(int page = 1);
    void loadQuestions(int page = 1);

signals:
    void backToHome();

private slots:
    void onBackClicked();
    void onUsersTabSelected();
    void onQuestionsTabSelected();
    
    // User management slots
    void onUsersPrevPage();
    void onUsersNextPage();
    void onPromoteUser(const QString& username);
    void onRevokeAdmin(const QString& username);
    void onBanUser(const QString& username);
    
    // Question management slots
    void onQuestionsPrevPage();
    void onQuestionsNextPage();
    void onAddQuestion();
    void onEditQuestion(int questionId);
    void onDeleteQuestion(int questionId);

private:
    void setupUI();
    void setupUsersTab();
    void setupQuestionsTab();
    void updateUsersTable();
    void updateQuestionsTable();
    void updateUsersPagination();
    void updateQuestionsPagination();
    void styleMessageBox(QMessageBox& msgBox, const QString& buttonColor = "#4CAF50");
    
    QTabWidget* tabWidget_;
    
    // Users tab widgets
    QWidget* usersTab_;
    QTableWidget* usersTable_;
    QPushButton* usersPrevButton_;
    QPushButton* usersNextButton_;
    QLabel* usersPageLabel_;
    int currentUsersPage_;
    int totalUsersPages_;
    int totalUsers_;
    
    // Questions tab widgets
    QWidget* questionsTab_;
    QTableWidget* questionsTable_;
    QPushButton* addQuestionButton_;
    QPushButton* questionsPrevButton_;
    QPushButton* questionsNextButton_;
    QLabel* questionsPageLabel_;
    int currentQuestionsPage_;
    int totalQuestionsPages_;
    int totalQuestions_;
    
    // Back button
    QPushButton* backButton_;
    
    ProtocolHandler* protocol_;
    bool demoMode_;
};

#endif // ADMINPANELSCREEN_H


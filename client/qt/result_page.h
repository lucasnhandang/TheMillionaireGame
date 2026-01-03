#ifndef RESULT_PAGE_H
#define RESULT_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

namespace MillionaireGame {

/**
 * Result page - Shows game result
 */
class ResultPage : public QWidget {
    Q_OBJECT

public:
    explicit ResultPage(long long finalPrize, int finalQuestion, bool isWinner, QWidget* parent = nullptr);

signals:
    void backToMenuRequested();
    void playAgainRequested();

private slots:
    void onBackToMenuClicked();
    void onPlayAgainClicked();

private:
    QLabel* result_label_;
    QLabel* prize_label_;
    QLabel* question_label_;
    QPushButton* back_button_;
    QPushButton* play_again_button_;
    
    void setupUI();
    void formatPrize(long long prize, QString& formatted);
};

} // namespace MillionaireGame

#endif // RESULT_PAGE_H


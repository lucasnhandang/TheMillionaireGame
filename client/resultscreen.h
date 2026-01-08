#ifndef RESULTSCREEN_H
#define RESULTSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ResultScreen : public QWidget
{
    Q_OBJECT

public:
    explicit ResultScreen(QWidget *parent = nullptr);

    void showResult(long long finalPrize, int totalScore, bool isWinner);

signals:
    void backToHomeClicked();

private slots:
    void onBackToHomeClicked();

private:
    void setupUI();

    QLabel* titleLabel_;
    QLabel* prizeLabel_;
    QLabel* scoreLabel_;
    QLabel* messageLabel_;
    QPushButton* backToHomeButton_;
};

#endif // RESULTSCREEN_H

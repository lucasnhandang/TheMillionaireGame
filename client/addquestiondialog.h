#ifndef ADDQUESTIONDIALOG_H
#define ADDQUESTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

class AddQuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddQuestionDialog(QWidget *parent = nullptr);
    
    QString getQuestionText() const;
    QStringList getOptions() const;
    int getCorrectAnswer() const;
    int getLevel() const;
    QString getLifeline5050Info() const;
    QString getLifelineAskInfo() const;
    QString getLifelineCallInfo() const;

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    bool validateInputs();
    
    QTextEdit* questionEdit_;
    QLineEdit* optionAEdit_;
    QLineEdit* optionBEdit_;
    QLineEdit* optionCEdit_;
    QLineEdit* optionDEdit_;
    QRadioButton* answerARadio_;
    QRadioButton* answerBRadio_;
    QRadioButton* answerCRadio_;
    QRadioButton* answerDRadio_;
    QComboBox* levelCombo_;
    
    // Lifeline 50/50: 4 checkboxes
    QCheckBox* lifeline5050A_;
    QCheckBox* lifeline5050B_;
    QCheckBox* lifeline5050C_;
    QCheckBox* lifeline5050D_;
    
    // Lifeline Ask Audience: 4 percentage inputs
    QSpinBox* lifelineAskA_;
    QSpinBox* lifelineAskB_;
    QSpinBox* lifelineAskC_;
    QSpinBox* lifelineAskD_;
    
    // Lifeline Phone Call: percentage and option
    QSpinBox* lifelineCallPercent_;
    QComboBox* lifelineCallOption_;
    
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
    
private slots:
    void on5050CheckboxChanged();
};

#endif // ADDQUESTIONDIALOG_H


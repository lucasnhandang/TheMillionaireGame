#ifndef EDITQUESTIONDIALOG_H
#define EDITQUESTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

class EditQuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditQuestionDialog(int questionId, QWidget *parent = nullptr);
    
    void setQuestionData(const QString& question, const QStringList& options, 
                        int correctAnswer, int level,
                        const QString& lifeline5050 = QString(),
                        const QString& lifelineAsk = QString(),
                        const QString& lifelineCall = QString());
    
    QString getQuestionText() const;
    QStringList getOptions() const;
    int getCorrectAnswer() const;
    int getLevel() const;
    QString getLifeline5050Info() const;
    QString getLifelineAskInfo() const;
    QString getLifelineCallInfo() const;
    int getQuestionId() const { return questionId_; }

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    bool validateInputs();
    
    int questionId_;
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
    
private:
    void parseLifeline5050(const QString& jsonStr);
    void parseLifelineAsk(const QString& jsonStr);
    void parseLifelineCall(const QString& text);
};

#endif // EDITQUESTIONDIALOG_H


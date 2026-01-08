#ifndef EDITQUESTIONDIALOG_H
#define EDITQUESTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>

class EditQuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditQuestionDialog(int questionId, QWidget *parent = nullptr);
    
    void setQuestionData(const QString& question, const QStringList& options, 
                        int correctAnswer, int level);
    
    QString getQuestionText() const;
    QStringList getOptions() const;
    int getCorrectAnswer() const;
    int getLevel() const;
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
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
};

#endif // EDITQUESTIONDIALOG_H


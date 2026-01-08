#ifndef ADDQUESTIONDIALOG_H
#define ADDQUESTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>

class AddQuestionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddQuestionDialog(QWidget *parent = nullptr);
    
    QString getQuestionText() const;
    QStringList getOptions() const;
    int getCorrectAnswer() const;
    int getLevel() const;

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
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
};

#endif // ADDQUESTIONDIALOG_H


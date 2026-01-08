#include "editquestiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QButtonGroup>
#include <QMessageBox>

EditQuestionDialog::EditQuestionDialog(int questionId, QWidget *parent)
    : QDialog(parent)
    , questionId_(questionId)
{
    setupUI();
    setWindowTitle(QString("Edit Question #%1").arg(questionId));
    setMinimumWidth(600);
    setMinimumHeight(500);
    
    // Apply dark theme consistent with the main app and ensure text is visible
    setStyleSheet(
        "QDialog {"
        "  background-color: #0D1B2A;"
        "  color: white;"
        "}"
        "QLabel {"
        "  color: white;"
        "}"
        "QGroupBox {"
        "  color: #00D4FF;"
        "  border: 1px solid #333;"
        "  margin-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px 0 5px;"
        "}"
        "QLineEdit, QTextEdit, QComboBox {"
        "  background-color: #1a1a2e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  border-radius: 4px;"
        "}"
        "QRadioButton {"
        "  color: white;"
        "}"
        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 9px;"
        "  border: 2px solid #00D4FF;"
        "  background-color: transparent;"
        "}"
        "QRadioButton::indicator:checked {"
        "  background-color: #00D4FF;"
        "  border-color: #00B0FF;"
        "}"
    );
}

void EditQuestionDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel* titleLabel = new QLabel(QString("Edit Question #%1").arg(questionId_), this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    mainLayout->addWidget(titleLabel);
    
    // Question text
    QLabel* questionLabel = new QLabel("Question Text:", this);
    questionLabel->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(questionLabel);
    
    questionEdit_ = new QTextEdit(this);
    questionEdit_->setPlaceholderText("Enter the question...");
    questionEdit_->setMinimumHeight(80);
    mainLayout->addWidget(questionEdit_);
    
    // Options
    QGroupBox* optionsGroup = new QGroupBox("Answer Options", this);
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);
    
    optionAEdit_ = new QLineEdit(this);
    optionAEdit_->setPlaceholderText("Enter option A...");
    optionsLayout->addRow("A:", optionAEdit_);
    
    optionBEdit_ = new QLineEdit(this);
    optionBEdit_->setPlaceholderText("Enter option B...");
    optionsLayout->addRow("B:", optionBEdit_);
    
    optionCEdit_ = new QLineEdit(this);
    optionCEdit_->setPlaceholderText("Enter option C...");
    optionsLayout->addRow("C:", optionCEdit_);
    
    optionDEdit_ = new QLineEdit(this);
    optionDEdit_->setPlaceholderText("Enter option D...");
    optionsLayout->addRow("D:", optionDEdit_);
    
    mainLayout->addWidget(optionsGroup);
    
    // Correct answer
    QGroupBox* answerGroup = new QGroupBox("Correct Answer", this);
    QHBoxLayout* answerLayout = new QHBoxLayout(answerGroup);
    
    QButtonGroup* answerButtonGroup = new QButtonGroup(this);
    
    answerARadio_ = new QRadioButton("A", this);
    answerButtonGroup->addButton(answerARadio_, 0);
    answerLayout->addWidget(answerARadio_);
    
    answerBRadio_ = new QRadioButton("B", this);
    answerButtonGroup->addButton(answerBRadio_, 1);
    answerLayout->addWidget(answerBRadio_);
    
    answerCRadio_ = new QRadioButton("C", this);
    answerButtonGroup->addButton(answerCRadio_, 2);
    answerLayout->addWidget(answerCRadio_);
    
    answerDRadio_ = new QRadioButton("D", this);
    answerButtonGroup->addButton(answerDRadio_, 3);
    answerLayout->addWidget(answerDRadio_);
    
    answerARadio_->setChecked(true); // Default
    
    mainLayout->addWidget(answerGroup);
    
    // Level
    QHBoxLayout* levelLayout = new QHBoxLayout();
    QLabel* levelLabel = new QLabel("Difficulty Level:", this);
    levelLabel->setStyleSheet("font-weight: bold;");
    levelLayout->addWidget(levelLabel);
    
    levelCombo_ = new QComboBox(this);
    levelCombo_->addItem("Level 1 (Easy)", 0);
    levelCombo_->addItem("Level 2 (Medium)", 1);
    levelCombo_->addItem("Level 3 (Hard)", 2);
    levelLayout->addWidget(levelCombo_);
    levelLayout->addStretch();
    
    mainLayout->addLayout(levelLayout);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelButton_ = new QPushButton("Cancel", this);
    cancelButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #666;"
        "  color: white;"
        "  padding: 10px 30px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #777;"
        "}"
    );
    connect(cancelButton_, &QPushButton::clicked, this, &EditQuestionDialog::onCancelClicked);
    buttonLayout->addWidget(cancelButton_);
    
    saveButton_ = new QPushButton("Save Changes", this);
    saveButton_->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  padding: 10px 30px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45A049;"
        "}"
    );
    connect(saveButton_, &QPushButton::clicked, this, &EditQuestionDialog::onSaveClicked);
    buttonLayout->addWidget(saveButton_);
    
    mainLayout->addLayout(buttonLayout);
}

void EditQuestionDialog::setQuestionData(const QString& question, const QStringList& options, 
                                        int correctAnswer, int level)
{
    questionEdit_->setPlainText(question);
    
    if (options.size() >= 4) {
        optionAEdit_->setText(options[0]);
        optionBEdit_->setText(options[1]);
        optionCEdit_->setText(options[2]);
        optionDEdit_->setText(options[3]);
    }
    
    switch (correctAnswer) {
        case 0: answerARadio_->setChecked(true); break;
        case 1: answerBRadio_->setChecked(true); break;
        case 2: answerCRadio_->setChecked(true); break;
        case 3: answerDRadio_->setChecked(true); break;
    }
    
    levelCombo_->setCurrentIndex(level);
}

QString EditQuestionDialog::getQuestionText() const
{
    return questionEdit_->toPlainText().trimmed();
}

QStringList EditQuestionDialog::getOptions() const
{
    return QStringList() 
        << optionAEdit_->text().trimmed()
        << optionBEdit_->text().trimmed()
        << optionCEdit_->text().trimmed()
        << optionDEdit_->text().trimmed();
}

int EditQuestionDialog::getCorrectAnswer() const
{
    if (answerARadio_->isChecked()) return 0;
    if (answerBRadio_->isChecked()) return 1;
    if (answerCRadio_->isChecked()) return 2;
    if (answerDRadio_->isChecked()) return 3;
    return 0;
}

int EditQuestionDialog::getLevel() const
{
    return levelCombo_->currentData().toInt();
}

bool EditQuestionDialog::validateInputs()
{
    if (getQuestionText().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a question.");
        return false;
    }
    
    QStringList options = getOptions();
    for (int i = 0; i < options.size(); i++) {
        if (options[i].isEmpty()) {
            QMessageBox::warning(this, "Validation Error", 
                               QString("Please enter option %1.").arg(char('A' + i)));
            return false;
        }
    }
    
    return true;
}

void EditQuestionDialog::onSaveClicked()
{
    if (validateInputs()) {
        accept();
    }
}

void EditQuestionDialog::onCancelClicked()
{
    reject();
}


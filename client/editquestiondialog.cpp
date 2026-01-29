#include "editquestiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QButtonGroup>
#include <QMessageBox>
#include <QRegularExpression>

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
        "QCheckBox {"
        "  color: white;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 4px;"
        "  border: 2px solid #00D4FF;"
        "  background-color: transparent;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #00D4FF;"
        "  border-color: #00B0FF;"
        "}"
        "QSpinBox {"
        "  background-color: #1a1a2e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "QComboBox {"
        "  background-color: #1a1a2e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "  min-width: 60px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 4px solid transparent;"
        "  border-right: 4px solid transparent;"
        "  border-top: 6px solid #00D4FF;"
        "  width: 0;"
        "  height: 0;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #1a1a2e;"
        "  color: white;"
        "  border: 1px solid #444;"
        "  selection-background-color: #0f3460;"
        "  selection-color: white;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  padding: 5px;"
        "  min-height: 20px;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "  background-color: #0f3460;"
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
    
    // Lifelines section
    QGroupBox* lifelinesGroup = new QGroupBox("Lifeline Information", this);
    QVBoxLayout* lifelinesLayout = new QVBoxLayout(lifelinesGroup);
    
    // 50/50 Lifeline
    QLabel* label5050 = new QLabel("50/50 Lifeline (Select 2 possible correct answers):", this);
    label5050->setStyleSheet("font-weight: bold;");
    lifelinesLayout->addWidget(label5050);
    
    QHBoxLayout* layout5050 = new QHBoxLayout();
    layout5050->setSpacing(60);
    lifeline5050A_ = new QCheckBox("A", this);
    lifeline5050B_ = new QCheckBox("B", this);
    lifeline5050C_ = new QCheckBox("C", this);
    lifeline5050D_ = new QCheckBox("D", this);
    layout5050->addWidget(lifeline5050A_);
    layout5050->addWidget(lifeline5050B_);
    layout5050->addWidget(lifeline5050C_);
    layout5050->addWidget(lifeline5050D_);
    layout5050->addStretch();
    connect(lifeline5050A_, &QCheckBox::stateChanged, this, &EditQuestionDialog::on5050CheckboxChanged);
    connect(lifeline5050B_, &QCheckBox::stateChanged, this, &EditQuestionDialog::on5050CheckboxChanged);
    connect(lifeline5050C_, &QCheckBox::stateChanged, this, &EditQuestionDialog::on5050CheckboxChanged);
    connect(lifeline5050D_, &QCheckBox::stateChanged, this, &EditQuestionDialog::on5050CheckboxChanged);
    lifelinesLayout->addLayout(layout5050);
    
    // Ask Audience Lifeline
    QLabel* labelAsk = new QLabel("Ask Audience Lifeline (Enter percentages, must sum to 100):", this);
    labelAsk->setStyleSheet("font-weight: bold; margin-top: 10px;");
    lifelinesLayout->addWidget(labelAsk);
    
    QHBoxLayout* layoutAsk = new QHBoxLayout();
    layoutAsk->setSpacing(15);
    QLabel* labelAskA = new QLabel("A:", this);
    lifelineAskA_ = new QSpinBox(this);
    lifelineAskA_->setRange(0, 100);
    lifelineAskA_->setValue(25);
    lifelineAskA_->setMinimumWidth(80);
    QLabel* labelAskB = new QLabel("B:", this);
    lifelineAskB_ = new QSpinBox(this);
    lifelineAskB_->setRange(0, 100);
    lifelineAskB_->setValue(25);
    lifelineAskB_->setMinimumWidth(80);
    QLabel* labelAskC = new QLabel("C:", this);
    lifelineAskC_ = new QSpinBox(this);
    lifelineAskC_->setRange(0, 100);
    lifelineAskC_->setValue(25);
    lifelineAskC_->setMinimumWidth(80);
    QLabel* labelAskD = new QLabel("D:", this);
    lifelineAskD_ = new QSpinBox(this);
    lifelineAskD_->setRange(0, 100);
    lifelineAskD_->setValue(25);
    lifelineAskD_->setMinimumWidth(80);
    layoutAsk->addWidget(labelAskA);
    layoutAsk->addWidget(lifelineAskA_);
    layoutAsk->addWidget(labelAskB);
    layoutAsk->addWidget(lifelineAskB_);
    layoutAsk->addWidget(labelAskC);
    layoutAsk->addWidget(lifelineAskC_);
    layoutAsk->addWidget(labelAskD);
    layoutAsk->addWidget(lifelineAskD_);
    layoutAsk->addStretch();
    lifelinesLayout->addLayout(layoutAsk);
    
    // Phone Call Lifeline
    QLabel* labelCall = new QLabel("Phone Call Lifeline:", this);
    labelCall->setStyleSheet("font-weight: bold; margin-top: 10px;");
    lifelinesLayout->addWidget(labelCall);
    
    QHBoxLayout* layoutCall = new QHBoxLayout();
    layoutCall->setSpacing(10);
    QLabel* labelCallText = new QLabel("I'm", this);
    lifelineCallPercent_ = new QSpinBox(this);
    lifelineCallPercent_->setRange(0, 100);
    lifelineCallPercent_->setValue(80);
    lifelineCallPercent_->setSuffix("%");
    lifelineCallPercent_->setMinimumWidth(80);
    lifelineCallPercent_->setMinimumHeight(25);
    QLabel* labelCallText2 = new QLabel("sure it's", this);
    lifelineCallOption_ = new QComboBox(this);
    lifelineCallOption_->addItem("A", 0);
    lifelineCallOption_->addItem("B", 1);
    lifelineCallOption_->addItem("C", 2);
    lifelineCallOption_->addItem("D", 3);
    lifelineCallOption_->setMinimumWidth(60);
    lifelineCallOption_->setMinimumHeight(25);
    layoutCall->addWidget(labelCallText);
    layoutCall->addWidget(lifelineCallPercent_);
    layoutCall->addWidget(labelCallText2);
    layoutCall->addWidget(lifelineCallOption_);
    layoutCall->addStretch();
    lifelinesLayout->addLayout(layoutCall);
    
    mainLayout->addWidget(lifelinesGroup);
    
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
                                        int correctAnswer, int level,
                                        const QString& lifeline5050,
                                        const QString& lifelineAsk,
                                        const QString& lifelineCall)
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
    
    // Parse and set lifeline data
    parseLifeline5050(lifeline5050);
    parseLifelineAsk(lifelineAsk);
    parseLifelineCall(lifelineCall);
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

QString EditQuestionDialog::getLifeline5050Info() const
{
    // Get checked indices (0-3 for A-D)
    QList<int> checkedIndices;
    if (lifeline5050A_->isChecked()) checkedIndices.append(0);
    if (lifeline5050B_->isChecked()) checkedIndices.append(1);
    if (lifeline5050C_->isChecked()) checkedIndices.append(2);
    if (lifeline5050D_->isChecked()) checkedIndices.append(3);
    
    // Convert to JSON array format: [1,3]
    if (checkedIndices.size() == 2) {
        return QString("[%1,%2]").arg(checkedIndices[0]).arg(checkedIndices[1]);
    }
    return ""; // Invalid - must have exactly 2 checked
}

QString EditQuestionDialog::getLifelineAskInfo() const
{
    // Convert to JSON object format: {"A":10,"B":65,"C":15,"D":10}
    return QString("{\"A\":%1,\"B\":%2,\"C\":%3,\"D\":%4}")
           .arg(lifelineAskA_->value())
           .arg(lifelineAskB_->value())
           .arg(lifelineAskC_->value())
           .arg(lifelineAskD_->value());
}

QString EditQuestionDialog::getLifelineCallInfo() const
{
    // Convert to text format: I'm 80% sure it's B
    char option = 'A' + lifelineCallOption_->currentData().toInt();
    return QString("I'm %1% sure it's %2").arg(lifelineCallPercent_->value()).arg(option);
}

void EditQuestionDialog::on5050CheckboxChanged()
{
    // Ensure exactly 2 checkboxes are checked
    int checkedCount = 0;
    if (lifeline5050A_->isChecked()) checkedCount++;
    if (lifeline5050B_->isChecked()) checkedCount++;
    if (lifeline5050C_->isChecked()) checkedCount++;
    if (lifeline5050D_->isChecked()) checkedCount++;
    
    if (checkedCount > 2) {
        // Uncheck the last one that was checked
        QCheckBox* sender = qobject_cast<QCheckBox*>(this->sender());
        if (sender) {
            sender->setChecked(false);
        }
    }
}

void EditQuestionDialog::parseLifeline5050(const QString& jsonStr)
{
    // Parse JSON array format: [1,3] -> check indices 1 and 3
    if (jsonStr.isEmpty()) return;
    
    // Remove whitespace and brackets
    QString cleaned = jsonStr.trimmed();
    if (cleaned.startsWith('[') && cleaned.endsWith(']')) {
        cleaned = cleaned.mid(1, cleaned.length() - 2).trimmed();
    }
    
    // Split by comma and parse indices
    QStringList parts = cleaned.split(',');
    lifeline5050A_->setChecked(false);
    lifeline5050B_->setChecked(false);
    lifeline5050C_->setChecked(false);
    lifeline5050D_->setChecked(false);
    
    for (const QString& part : parts) {
        bool ok;
        int index = part.trimmed().toInt(&ok);
        if (ok && index >= 0 && index <= 3) {
            switch (index) {
                case 0: lifeline5050A_->setChecked(true); break;
                case 1: lifeline5050B_->setChecked(true); break;
                case 2: lifeline5050C_->setChecked(true); break;
                case 3: lifeline5050D_->setChecked(true); break;
            }
        }
    }
}

void EditQuestionDialog::parseLifelineAsk(const QString& jsonStr)
{
    // Parse JSON object format: {"A":10,"B":65,"C":15,"D":10}
    if (jsonStr.isEmpty()) {
        lifelineAskA_->setValue(25);
        lifelineAskB_->setValue(25);
        lifelineAskC_->setValue(25);
        lifelineAskD_->setValue(25);
        return;
    }
    
    // Use regex to extract values
    QRegularExpression reA("\"A\"\\s*:\\s*(\\d+)");
    QRegularExpression reB("\"B\"\\s*:\\s*(\\d+)");
    QRegularExpression reC("\"C\"\\s*:\\s*(\\d+)");
    QRegularExpression reD("\"D\"\\s*:\\s*(\\d+)");
    
    QRegularExpressionMatch matchA = reA.match(jsonStr);
    QRegularExpressionMatch matchB = reB.match(jsonStr);
    QRegularExpressionMatch matchC = reC.match(jsonStr);
    QRegularExpressionMatch matchD = reD.match(jsonStr);
    
    if (matchA.hasMatch()) {
        lifelineAskA_->setValue(matchA.captured(1).toInt());
    }
    if (matchB.hasMatch()) {
        lifelineAskB_->setValue(matchB.captured(1).toInt());
    }
    if (matchC.hasMatch()) {
        lifelineAskC_->setValue(matchC.captured(1).toInt());
    }
    if (matchD.hasMatch()) {
        lifelineAskD_->setValue(matchD.captured(1).toInt());
    }
}

void EditQuestionDialog::parseLifelineCall(const QString& text)
{
    // Parse text format: I'm 80% sure it's B
    if (text.isEmpty()) {
        lifelineCallPercent_->setValue(80);
        lifelineCallOption_->setCurrentIndex(0);
        return;
    }
    
    // Extract percentage
    QRegularExpression rePercent("(\\d+)%");
    QRegularExpressionMatch matchPercent = rePercent.match(text);
    if (matchPercent.hasMatch()) {
        lifelineCallPercent_->setValue(matchPercent.captured(1).toInt());
    }
    
    // Extract option (A, B, C, or D)
    QRegularExpression reOption("it'?s?\\s+([A-D])", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch matchOption = reOption.match(text);
    if (matchOption.hasMatch()) {
        QString option = matchOption.captured(1).toUpper();
        int index = option[0].toLatin1() - 'A';
        if (index >= 0 && index <= 3) {
            lifelineCallOption_->setCurrentIndex(index);
        }
    }
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
    
    // Validate 50/50 lifeline - must have exactly 2 checked
    int checkedCount = 0;
    if (lifeline5050A_->isChecked()) checkedCount++;
    if (lifeline5050B_->isChecked()) checkedCount++;
    if (lifeline5050C_->isChecked()) checkedCount++;
    if (lifeline5050D_->isChecked()) checkedCount++;
    if (checkedCount != 2) {
        QMessageBox::warning(this, "Validation Error", 
                           "Please select exactly 2 options for the 50/50 lifeline.");
        return false;
    }
    
    // Validate Ask Audience lifeline - percentages should sum to 100
    int totalPercent = lifelineAskA_->value() + lifelineAskB_->value() + 
                       lifelineAskC_->value() + lifelineAskD_->value();
    if (totalPercent != 100) {
        QMessageBox::warning(this, "Validation Error", 
                           QString("Ask Audience percentages must sum to 100 (currently %1).").arg(totalPercent));
        return false;
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


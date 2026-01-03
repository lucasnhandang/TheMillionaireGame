#ifndef INSTRUCTION_DIALOG_H
#define INSTRUCTION_DIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTextEdit>

namespace MillionaireGame {

/**
 * Instruction dialog - Shows game instructions
 */
class InstructionDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstructionDialog(QWidget* parent = nullptr);

private slots:
    void onCloseButtonClicked();

private:
    QPushButton* close_button_;
    QTextEdit* instruction_text_;
    
    void setupUI();
};

} // namespace MillionaireGame

#endif // INSTRUCTION_DIALOG_H


#ifndef WALKAWAY_DIALOG_H
#define WALKAWAY_DIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

namespace MillionaireGame {

/**
 * Walk Away confirmation dialog
 */
class WalkAwayDialog : public QDialog {
    Q_OBJECT

public:
    explicit WalkAwayDialog(QWidget* parent = nullptr);

signals:
    void confirmed();
    void cancelled();

private slots:
    void onYesClicked();
    void onNoClicked();

private:
    QPushButton* yes_button_;
    QPushButton* no_button_;
    
    void setupUI();
};

} // namespace MillionaireGame

#endif // WALKAWAY_DIALOG_H


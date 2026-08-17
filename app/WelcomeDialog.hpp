#pragma once

#include "proteus/ui/CanvasModel.hpp"

#include <QDialog>
#include <QString>

class QComboBox;
class QListWidget;

class WelcomeDialog final : public QDialog {
    Q_OBJECT

public:
    explicit WelcomeDialog(QWidget* parent = nullptr);

    bool opensExistingProject() const noexcept;
    QString selectedPath() const;
    proteus::CanvasSettings selectedCanvas() const;

private:
    void chooseOpenFile();
    void acceptNewProject();
    void openRecentProject();

    QComboBox* canvasSize_;
    QListWidget* recentFiles_;
    bool opensExisting_{false};
    QString selectedPath_;
};

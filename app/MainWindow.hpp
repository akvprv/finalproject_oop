#pragma once

#include "CanvasWidget.hpp"

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void createProject(proteus::CanvasSettings settings);
    bool openProject(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildActions();
    void buildLibraryDock();
    void buildActiveDock();
    void buildLogDock();
    void refreshCatalog();
    void refreshActiveComponents();
    void updatePreview();
    void updateHistoryActions();
    void newProject();
    void openProjectDialog();
    bool saveProject();
    bool saveProjectAs();
    bool saveProjectTo(const QString& path);
    bool confirmDiscardChanges();
    void exportImage();
    void rememberRecentFile(const QString& path);
    void showError(const QString& title, const QString& message);

    CanvasWidget* canvas_;
    proteus::ComponentCatalog catalog_;
    QLineEdit* searchBox_;
    QComboBox* categoryBox_;
    QListWidget* catalogList_;
    QLabel* preview_;
    QListWidget* activeList_;
    QPlainTextEdit* log_;
    QAction* undoAction_;
    QAction* redoAction_;
    QString currentPath_;
};

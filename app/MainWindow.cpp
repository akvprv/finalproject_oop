#include "MainWindow.hpp"

#include "SchematicPainter.hpp"
#include "WelcomeDialog.hpp"
#include "proteus/persistence/ProjectSerializer.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QVariant>
#include <QVBoxLayout>

#include <exception>
#include <filesystem>

namespace {

std::filesystem::path projectPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), canvas_(new CanvasWidget(this)),
      searchBox_(nullptr), categoryBox_(nullptr), catalogList_(nullptr),
      preview_(nullptr), activeList_(nullptr), log_(nullptr),
      undoAction_(nullptr), redoAction_(nullptr) {
    setWindowTitle("Proteus Circuit Studio");
    resize(1380, 840);
    setCentralWidget(canvas_);
    buildActions();
    buildLibraryDock();
    buildActiveDock();
    buildLogDock();
    connect(canvas_, &CanvasWidget::statusChanged,
            statusBar(), &QStatusBar::showMessage);
    connect(canvas_, &CanvasWidget::logMessage,
            log_, &QPlainTextEdit::appendPlainText);
    connect(canvas_, &CanvasWidget::circuitChanged,
            this, &MainWindow::refreshActiveComponents);
    connect(canvas_, &CanvasWidget::historyChanged,
            this, &MainWindow::updateHistoryActions);
    statusBar()->showMessage("Ready");
}

void MainWindow::createProject(proteus::CanvasSettings settings) {
    currentPath_.clear();
    canvas_->newDocument(settings);
    setWindowTitle("Untitled - Proteus Circuit Studio");
}

bool MainWindow::openProject(const QString& path) {
    try {
        auto document = proteus::ProjectSerializer::loadFile(projectPath(path));
        canvas_->loadDocument(std::move(document));
        canvas_->markSaved();
        currentPath_ = path;
        rememberRecentFile(path);
        setWindowTitle(QFileInfo(path).fileName() + " - Proteus Circuit Studio");
        log_->appendPlainText("Project opened: " + path);
        return true;
    } catch (const std::exception& error) {
        showError("Open Project", QString::fromUtf8(error.what()));
        return false;
    }
}

void MainWindow::buildActions() {
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* editMenu = menuBar()->addMenu("&Edit");
    auto* simulationMenu = menuBar()->addMenu("&Simulation");
    auto* toolBar = addToolBar("Main");
    toolBar->setMovable(false);

    auto* newAction = fileMenu->addAction("New Project");
    newAction->setShortcut(QKeySequence::New);
    auto* openAction = fileMenu->addAction("Open Project");
    openAction->setShortcut(QKeySequence::Open);
    auto* saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(QKeySequence::Save);
    auto* saveAsAction = fileMenu->addAction("Save As");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    auto* exportAction = fileMenu->addAction("Export PNG");
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction("Exit");

    undoAction_ = editMenu->addAction("Undo");
    undoAction_->setShortcut(QKeySequence::Undo);
    redoAction_ = editMenu->addAction("Redo");
    redoAction_->setShortcut(QKeySequence::Redo);
    editMenu->addSeparator();
    auto* selectionAction = editMenu->addAction("Selection Mode");
    selectionAction->setShortcut(Qt::Key_Escape);

    auto* runAction = simulationMenu->addAction("Run");
    runAction->setShortcut(Qt::Key_F5);
    auto* pauseAction = simulationMenu->addAction("Pause");
    pauseAction->setShortcut(Qt::Key_F6);
    auto* stopAction = simulationMenu->addAction("Stop");
    stopAction->setShortcut(Qt::Key_F7);
    auto* stepAction = simulationMenu->addAction("Step");
    stepAction->setShortcut(Qt::Key_F8);
    simulationMenu->addSeparator();
    auto* drcAction = simulationMenu->addAction("Run DRC");

    toolBar->addAction(newAction);
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addSeparator();
    toolBar->addAction(undoAction_);
    toolBar->addAction(redoAction_);
    toolBar->addSeparator();
    toolBar->addAction(runAction);
    toolBar->addAction(pauseAction);
    toolBar->addAction(stopAction);
    toolBar->addAction(stepAction);
    toolBar->addAction(drcAction);

    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProjectDialog);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(undoAction_, &QAction::triggered, canvas_, &CanvasWidget::undo);
    connect(redoAction_, &QAction::triggered, canvas_, &CanvasWidget::redo);
    connect(selectionAction, &QAction::triggered,
            canvas_, &CanvasWidget::clearPlacementType);
    connect(runAction, &QAction::triggered,
            canvas_, &CanvasWidget::runSimulation);
    connect(pauseAction, &QAction::triggered,
            canvas_, &CanvasWidget::pauseSimulation);
    connect(stopAction, &QAction::triggered,
            canvas_, &CanvasWidget::stopSimulation);
    connect(stepAction, &QAction::triggered,
            canvas_, &CanvasWidget::stepSimulation);
    connect(drcAction, &QAction::triggered,
            canvas_, &CanvasWidget::runDesignRuleCheck);
    updateHistoryActions();
}

void MainWindow::buildLibraryDock() {
    auto* dock = new QDockWidget("Component Library", this);
    auto* panel = new QWidget(dock);
    auto* layout = new QVBoxLayout(panel);
    searchBox_ = new QLineEdit(panel);
    searchBox_->setPlaceholderText("Search components");
    categoryBox_ = new QComboBox(panel);
    categoryBox_->addItem("All categories");
    for (const auto& category : catalog_.categories()) {
        categoryBox_->addItem(QString::fromStdString(category));
    }
    catalogList_ = new QListWidget(panel);
    preview_ = new QLabel("Select a component", panel);
    preview_->setAlignment(Qt::AlignCenter);
    preview_->setMinimumHeight(120);
    preview_->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(searchBox_);
    layout->addWidget(categoryBox_);
    layout->addWidget(catalogList_, 1);
    layout->addWidget(preview_);
    dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(searchBox_, &QLineEdit::textChanged,
            this, &MainWindow::refreshCatalog);
    connect(categoryBox_, &QComboBox::currentTextChanged,
            this, &MainWindow::refreshCatalog);
    connect(catalogList_, &QListWidget::currentRowChanged,
            this, &MainWindow::updatePreview);
    connect(catalogList_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
                canvas_->setPlacementType(item->data(Qt::UserRole).toString().toStdString());
            });
    refreshCatalog();
}

void MainWindow::buildActiveDock() {
    auto* dock = new QDockWidget("Active Components", this);
    auto* panel = new QWidget(dock);
    auto* layout = new QVBoxLayout(panel);
    activeList_ = new QListWidget(panel);
    auto* removeButton = new QPushButton("Remove selected component", panel);
    layout->addWidget(activeList_, 1);
    layout->addWidget(removeButton);
    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    connect(removeButton, &QPushButton::clicked, this, [this] {
        const auto* item = activeList_->currentItem();
        if (!item) {
            return;
        }
        canvas_->removeComponent(
            static_cast<proteus::ComponentId>(item->data(Qt::UserRole).toULongLong()));
    });
}

void MainWindow::buildLogDock() {
    auto* dock = new QDockWidget("Simulation Log", this);
    log_ = new QPlainTextEdit(dock);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(1000);
    dock->setWidget(log_);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::refreshCatalog() {
    const std::string text = searchBox_->text().toStdString();
    const std::string category = categoryBox_->currentIndex() == 0
        ? std::string{} : categoryBox_->currentText().toStdString();
    catalogList_->clear();
    for (const auto& entry : catalog_.search(text, category)) {
        auto* item = new QListWidgetItem(
            QString::fromStdString(entry.displayName), catalogList_);
        item->setData(Qt::UserRole, QString::fromStdString(entry.typeName));
        item->setToolTip(QString::fromStdString(entry.description));
    }
    if (catalogList_->count() > 0) {
        catalogList_->setCurrentRow(0);
    }
}

void MainWindow::refreshActiveComponents() {
    activeList_->clear();
    for (const auto id : canvas_->document().circuit.componentIds()) {
        const auto& component = canvas_->document().circuit.component(id);
        auto* item = new QListWidgetItem(
            QString("%1  |  %2")
                .arg(QString::fromStdString(component.label()),
                     QString::fromStdString(component.typeName())),
            activeList_);
        item->setData(Qt::UserRole,
                      QVariant::fromValue<qulonglong>(component.id()));
    }
}

void MainWindow::updatePreview() {
    const auto* item = catalogList_->currentItem();
    if (!item) {
        preview_->setText("Select a component");
        preview_->setPixmap({});
        return;
    }
    QPixmap image(260, 120);
    image.fill(QColor("#fafafa"));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    schematic::drawSymbol(
        painter,
        item->data(Qt::UserRole).toString(),
        QRectF(40.0, 18.0, 180.0, 72.0));
    painter.setPen(QColor("#455a64"));
    painter.drawText(QRectF(10.0, 92.0, 240.0, 22.0),
                     Qt::AlignCenter, item->text());
    preview_->setPixmap(image);
}

void MainWindow::updateHistoryActions() {
    if (undoAction_) {
        undoAction_->setEnabled(canvas_->canUndo());
    }
    if (redoAction_) {
        redoAction_->setEnabled(canvas_->canRedo());
    }
}

void MainWindow::newProject() {
    WelcomeDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (!confirmDiscardChanges()) {
        return;
    }
    if (dialog.opensExistingProject()) {
        openProject(dialog.selectedPath());
    } else {
        createProject(dialog.selectedCanvas());
    }
}

void MainWindow::openProjectDialog() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Project", {}, "Proteus Project (*.proteus.json *.json)");
    if (!path.isEmpty() && confirmDiscardChanges()) {
        openProject(path);
    }
}

bool MainWindow::saveProject() {
    if (currentPath_.isEmpty()) {
        return saveProjectAs();
    }
    return saveProjectTo(currentPath_);
}

bool MainWindow::saveProjectAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project", currentPath_, "Proteus Project (*.proteus.json)");
    if (path.isEmpty()) {
        return false;
    }
    if (!path.endsWith(".json", Qt::CaseInsensitive)) {
        path += ".proteus.json";
    }
    if (!saveProjectTo(path)) {
        return false;
    }
    currentPath_ = path;
    setWindowTitle(QFileInfo(path).fileName() + " - Proteus Circuit Studio");
    return true;
}

bool MainWindow::saveProjectTo(const QString& path) {
    try {
        proteus::ProjectSerializer::saveFile(
            canvas_->document(), projectPath(path));
        canvas_->markSaved();
        rememberRecentFile(path);
        log_->appendPlainText("Project saved: " + path);
        return true;
    } catch (const std::exception& error) {
        showError("Save Project", QString::fromUtf8(error.what()));
        return false;
    }
}

bool MainWindow::confirmDiscardChanges() {
    if (!canvas_->isModified()) {
        return true;
    }
    const auto choice = QMessageBox::warning(
        this,
        "Unsaved changes",
        "The current project has unsaved changes.",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (choice == QMessageBox::Save) {
        return saveProject();
    }
    return choice == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::exportImage() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export Canvas", {}, "PNG Image (*.png)");
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(".png", Qt::CaseInsensitive)) {
        path += ".png";
    }
    if (!canvas_->exportPng(path)) {
        showError("Export Image", "The PNG file could not be written.");
    } else {
        log_->appendPlainText("Canvas exported: " + path);
    }
}

void MainWindow::rememberRecentFile(const QString& path) {
    QSettings settings;
    QStringList recent = settings.value("recentProjects").toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > 8) {
        recent.removeLast();
    }
    settings.setValue("recentProjects", recent);
}

void MainWindow::showError(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

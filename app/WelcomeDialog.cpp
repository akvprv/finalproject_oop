#include "WelcomeDialog.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

WelcomeDialog::WelcomeDialog(QWidget* parent)
    : QDialog(parent), canvasSize_(new QComboBox(this)),
      recentFiles_(new QListWidget(this)) {
    setWindowTitle("Proteus Circuit Studio");
    setMinimumSize(620, 430);
    auto* root = new QVBoxLayout(this);
    auto* title = new QLabel("Proteus Circuit Studio", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);
    auto* subtitle = new QLabel(
        "Create a circuit project or continue from a recent design.", this);
    subtitle->setStyleSheet("color:#546e7a");
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addSpacing(18);

    auto* actions = new QHBoxLayout;
    auto* newPanel = new QFrame(this);
    newPanel->setFrameShape(QFrame::StyledPanel);
    auto* newLayout = new QVBoxLayout(newPanel);
    auto* newTitle = new QLabel("New Project", newPanel);
    QFont sectionFont = newTitle->font();
    sectionFont.setBold(true);
    sectionFont.setPointSize(13);
    newTitle->setFont(sectionFont);
    canvasSize_->addItems({"A4 - 1600 x 1000", "A3 - 2200 x 1400", "A2 - 3000 x 2000"});
    auto* createButton = new QPushButton("Create Project", newPanel);
    newLayout->addWidget(newTitle);
    newLayout->addWidget(new QLabel("Canvas size", newPanel));
    newLayout->addWidget(canvasSize_);
    newLayout->addStretch();
    newLayout->addWidget(createButton);

    auto* openPanel = new QFrame(this);
    openPanel->setFrameShape(QFrame::StyledPanel);
    auto* openLayout = new QVBoxLayout(openPanel);
    auto* openTitle = new QLabel("Open Project", openPanel);
    openTitle->setFont(sectionFont);
    auto* openButton = new QPushButton("Browse Project File", openPanel);
    openLayout->addWidget(openTitle);
    openLayout->addWidget(new QLabel("Recent projects", openPanel));
    openLayout->addWidget(recentFiles_, 1);
    openLayout->addWidget(openButton);
    actions->addWidget(newPanel, 1);
    actions->addWidget(openPanel, 1);
    root->addLayout(actions, 1);

    QSettings settings;
    const QStringList recent = settings.value("recentProjects").toStringList();
    for (const QString& path : recent) {
        if (QFileInfo::exists(path)) {
            auto* item = new QListWidgetItem(QFileInfo(path).fileName(), recentFiles_);
            item->setData(Qt::UserRole, path);
            item->setToolTip(path);
        }
    }

    connect(createButton, &QPushButton::clicked,
            this, &WelcomeDialog::acceptNewProject);
    connect(openButton, &QPushButton::clicked,
            this, &WelcomeDialog::chooseOpenFile);
    connect(recentFiles_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem*) { openRecentProject(); });
}

bool WelcomeDialog::opensExistingProject() const noexcept {
    return opensExisting_;
}

QString WelcomeDialog::selectedPath() const { return selectedPath_; }

proteus::CanvasSettings WelcomeDialog::selectedCanvas() const {
    proteus::CanvasSettings settings;
    settings.preset = static_cast<proteus::CanvasPreset>(canvasSize_->currentIndex());
    switch (settings.preset) {
        case proteus::CanvasPreset::A4:
            settings.width = 1600.0;
            settings.height = 1000.0;
            break;
        case proteus::CanvasPreset::A3:
            settings.width = 2200.0;
            settings.height = 1400.0;
            break;
        case proteus::CanvasPreset::A2:
            settings.width = 3000.0;
            settings.height = 2000.0;
            break;
        case proteus::CanvasPreset::Custom:
            break;
    }
    return settings;
}

void WelcomeDialog::chooseOpenFile() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Project", {}, "Proteus Project (*.proteus.json *.json)");
    if (path.isEmpty()) {
        return;
    }
    selectedPath_ = path;
    opensExisting_ = true;
    accept();
}

void WelcomeDialog::acceptNewProject() {
    selectedPath_.clear();
    opensExisting_ = false;
    accept();
}

void WelcomeDialog::openRecentProject() {
    const auto* item = recentFiles_->currentItem();
    if (!item) {
        return;
    }
    selectedPath_ = item->data(Qt::UserRole).toString();
    opensExisting_ = true;
    accept();
}

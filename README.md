# Kian Dehdashti Contribution

Student number: 402101674

This package contains the files assigned to Kian Dehdashti:

- Structured component catalog and search
- Component preview and active component panel integration
- JSON persistence
- Undo and Redo snapshot history
- DRC facade and simulation log integration
- Save, Open, Save As, PNG export, tests, and release tools

## Corrections in this reviewed package

- Validates project format, version, identifiers, references, rotations, canvas
  settings, and all numeric values during JSON loading.
- Saves through a temporary file and recoverable backup instead of overwriting
  the only project copy directly.
- Makes filesystem paths Unicode-safe on Windows.
- Fixes first-use History recording and filters lifecycle information from DRC.
- Adds Save/Discard/Cancel protection for New, Open, and application exit.
- Keeps the current path unchanged when Save As fails and supports deletion from
  the Active Components panel.

Copy these paths into the complete repository without changing their relative
locations. The modules use the shared circuit, component, simulation, and canvas
interfaces from the complete project.

This is an integration contribution package. Its `MainWindow` expects the
reviewed UI package (`CanvasWidget` and `SchematicPainter`) and the reviewed
Core package to be applied to the complete repository first.

No Git author email was invented for this package. Before committing, set the
email connected to the student's GitHub account:

```bash
git config user.name "Kian Dehdashti"
git config user.email "REAL_GITHUB_EMAIL"
git switch -c feature/dehdashti-project-tools
git add include/proteus/library include/proteus/persistence include/proteus/history
git add include/proteus/drc src/library src/persistence src/history src/drc
git add app/MainWindow.* tests tools docs
git commit -m "feat(project): add library persistence history DRC and release tools"
```

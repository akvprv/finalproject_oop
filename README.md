# Amirhossein Ashkurlaji Contribution

Student number: 402101211

This package contains the files assigned to Amirhossein Ashkurlaji:

- Startup screen and recent projects
- Canvas presets
- Grid and snap
- Zoom, pan, coordinates, status reporting
- Placement, selection, multi-selection, and drag and drop
- Rotation, mirroring, deletion, wiring gestures, and property dialogs
- Shared schematic-symbol rendering for both the canvas and library preview

## Corrections in this reviewed package

- Resets transient editor state when a project is created, opened, undone, or redone.
- Tracks unsaved changes and blocks structural editing while simulation is active.
- Rebuilds label counters after loading to prevent duplicate labels such as `R1`.
- Uses rotation-aware hit boxes and validates finite canvas dimensions.
- Completes editable properties for batteries, clocks, LEDs, switches, displays,
  gates, and flip-flops.
- Adds `SchematicPainter` so components are drawn as circuit symbols instead of
  generic boxes.

Copy these paths into the complete repository without changing their relative
locations. The files use the shared core, library, persistence, history, and
simulation interfaces from the complete project.

This is a contribution package, not a standalone Qt application. Apply it to
the complete repository together with the reviewed Core and Tools packages.

No Git author email was invented for this package. Before committing, set the
email connected to the student's GitHub account:

```bash
git config user.name "Amirhossein Ashkurlaji"
git config user.email "REAL_GITHUB_EMAIL"
git switch -c feature/ashkurlaji-ui
git add include/proteus/ui src/ui app/WelcomeDialog.* app/CanvasWidget.* app/SchematicPainter.*
git commit -m "feat(canvas): add startup canvas and editor interactions"
```

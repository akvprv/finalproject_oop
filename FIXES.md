# Review and Fix Summary

- Fixed stale and duplicate junction handling.
- Rebuilt nets after wire rerouting.
- Rejected NaN and Infinity in sources, passives, clocks, LEDs, and delays.
- Blocked simulation startup for every error-level diagnostic.
- Added missing editable properties and regression coverage.
- Removed repository internals from the distributable archive.

The backend remains independent of Qt and can be built and tested on its own.

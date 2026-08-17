# Git and GitHub Workflow

The final repository should use one branch per member and a pull request for
each assigned section. Do not publish commits for another member with an
invented email address.

## Contributor identities

```bash
git config user.name "REAL FULL NAME"
git config user.email "VERIFIED_GITHUB_EMAIL_OR_NOREPLY"
```

Each person must review and commit their own section. The package does not
publish or guess a private email address.

## Recommended branches

```bash
git switch -c feature/ashkurlaji-ui
git switch -c feature/sangchi-core-simulation
git switch -c feature/dehdashti-project-tools
```

## Commit groups

Amirhossein Ashkurlaji:

```text
feat(startup): add project creation and recent files
feat(canvas): add grid snap zoom pan and coordinates
feat(editor): add placement selection transforms and properties
```

Seyed Amir Hossein Hosseini Sangchi:

```text
feat(core): add component pin circuit and net models
feat(wiring): add orthogonal routing and junctions
feat(components): add sources passive interactive and digital parts
feat(simulation): add controls live states and diagnostics
```

Kian Dehdashti:

```text
feat(library): add categories search preview and active list
feat(project): add JSON persistence undo redo and image export
feat(drc): add validation log and integration tests
```

## Final integration

Merge the three reviewed branches into `main`, run `make test`, build the Qt
application, open the sample project, and tag the submitted revision.

```bash
git switch main
git merge --no-ff feature/ashkurlaji-ui
git merge --no-ff feature/sangchi-core-simulation
git merge --no-ff feature/dehdashti-project-tools
make test
git tag -a v1.0.0 -m "Course project submission"
```

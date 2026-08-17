# Git Workflow

Set the real contributor identity locally before committing. A verified GitHub
address or the account's `noreply` address is appropriate:

```bash
git config user.name "REAL FULL NAME"
git config user.email "VERIFIED_GITHUB_EMAIL_OR_NOREPLY"
```

Do not manufacture commits or publish another person's private email address.

Use one branch for each feature and merge it after the related tests pass.

Suggested branches:

- `feature/core-model`
- `feature/basic-components`
- `feature/digital-logic`
- `feature/wiring`
- `feature/simulation-engine`
- `integration/qt-backend`

Each commit should contain one clear, testable change. Build outputs and IDE
settings should not be committed.

Good commit messages:

```text
feat(core): add component and pin transformations
feat(wiring): connect explicit junction nets
feat(logic): add configurable NAND gate delay
test(simulation): cover floating inputs and output conflicts
docs(qt): document the backend integration contract
fix(wiring): reroute wires after component rotation
```

Before pushing:

```bash
make test
git status
git diff --check
```

Add the team repository and push when its address is available:

```bash
git remote add origin https://github.com/ORGANIZATION/REPOSITORY.git
git push -u origin main
```

Do not store credentials or access tokens in the repository.

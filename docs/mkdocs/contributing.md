# Contributing

How we work on the engine. Building and testing it: [Getting started](getting-started.md#build-and-test).

## Workflow

1. An issue of the project board, and a branch named after it (`119-engine-more-lua-bindings`).
2. A pull request to `main`, merged once the CI passes and a member approved it.
3. Its changes under `[Unreleased]` in the [Changelog](changelog.md).

## Commits

[Conventional Commits](https://www.conventionalcommits.org/): `type(scope): what changes`, one change per commit. Types: `feat`, `fix`, `test`, `docs`, `refactor`; the scope is the module.

## Code style

`clang-format` (Google, 4 spaces) and `clang-tidy` run in the CI, warnings are errors. `pre-commit install` runs them before each commit.

| Name | Style | Example |
| --- | --- | --- |
| Types | PascalCase | `ScriptingSystem` |
| Functions, struct fields | camelCase | `loadScene`, `frameDuration` |
| Class members | `_camelCase` | `_gameRoot` |
| Constants | `kPascalCase` | `kMaxComponents` |

Headers in `engine/include/ampersand/<module>/`, sources in `engine/src/<module>/`.

## Docs and tests

- Every public declaration has a Doxygen `@brief`.
- A new feature updates the page of its module in `docs/mkdocs/architecture/`.
- Its tests go in `engine/tests/<module>/` (GoogleTest), and `ctest` passes on Linux and Windows.

## Release

1. In a pull request, rename `## [Unreleased]` of the [Changelog](changelog.md) to the new version and today's date (eg: `## [0.2.0] - 2026-10-16`), add an empty `## [Unreleased]` above it, and update the links at the bottom of the file.
2. Set the same version in `project()` of `CMakeLists.txt` (eg: `VERSION 0.2.0`), and the new tag in the `GIT_TAG` of the FetchContent snippets, in `README.md` and `docs/mkdocs/getting-started.md` (eg: `GIT_TAG v0.2.0`).
3. Once it is merged, tag `main` and push the tag (`git tag v0.2.0 && git push origin v0.2.0`). The release workflow does the checks and creates the GitHub release.

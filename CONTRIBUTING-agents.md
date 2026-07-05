# Contributing With Agents

These notes are for autonomous coding agents picking up issues in the v1
revival. They supplement [CONTRIBUTING.md](CONTRIBUTING.md) and the repository's
pull request template.

## Picking Up Work

- Start from issues labelled `agent-ready`. These issues should contain a
  Goal, Scope, Out of scope, Acceptance criteria, and Verification block.
- Treat `agent-design` as design or decomposition work, not permission to
  implement beyond what the issue asks for.
- Treat `roadmap:v1` as part of the revival work targeting Ubuntu 24.04
  `amd64`.
- Use `area:*` labels to understand the primary ownership area, for example
  `area:docs`, `area:build`, `area:ci`, or `area:plugins`.
- When requesting leader acknowledgment through Agent Mail, use the leader's
  Agent Mail member ID, not the team slot ID.
- Read linked issues and pull requests before editing files.

## Scope Discipline

- Keep changes inside the issue's Scope and Acceptance criteria.
- Do not broaden v1 support beyond Ubuntu 24.04 `amd64` unless the issue says
  so.
- Do not reintroduce removed or deferred streaming services into the v1 default
  path.
- Leave unrelated code, build, CI, and formatting churn alone.
- If needed behavior is ambiguous, ask in the issue or coordinate with the
  maintainer or dispatcher before implementing.
- When the issue says docs-only, change docs only.

## Pull Requests

- Open one pull request per issue and use `.github/pull_request_template.md`.
- Link the issue in the pull request's Linked Issue section, for example
  `Closes #123`.
- Keep the Summary focused on the files and behavior changed.
- Fill in the Verification section with the exact commands run and their
  results.
- Ensure the Scope Checklist remains true before requesting review.

## Verification

- Run every command in the issue's Verification block before marking the work
  ready.
- For docs-only issues, also run any lightweight checks requested by the issue.
- If a command cannot run locally, state why in the pull request and leave the
  issue open for follow-up.
- Do not close an issue because the change looks obvious; close it only after
  the requested verification passes or the maintainer accepts the documented
  exception.

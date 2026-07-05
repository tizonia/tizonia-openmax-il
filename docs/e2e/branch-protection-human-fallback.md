# Branch protection human-fallback validation

## Purpose

This end-to-end note validates that Claude Deck can open a passing docs-only
draft PR for issue #852 while repository branch protection still requires a
human review before merge.

## Expected flow

- Claude Deck dispatches the issue to the Generalist slot.
- The Generalist makes only this docs change and opens a draft PR linked to the
  issue.
- CI runs normally for the draft PR.
- Any automatic merge attempt remains blocked until the required human review is
  present.

## Validation boundary

This scenario must not change CI workflows, build configuration, repository
settings, or runtime behavior.

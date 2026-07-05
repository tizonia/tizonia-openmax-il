# Retry Budget E2E Scope

This PR branch is an autonomous dispatch E2E harness branch for validating
Deck retry-budget behavior. The intended agent-authored change is
documentation-only and deliberately low risk: this note records that the
branch exists so Deck can observe retry-budget escalation and retry action
behavior on distinct failed PR heads.

After the draft PR opens, the E2E harness may push deliberately broken
follow-up commits to this PR branch. Those harness commits are outside the
scope of this note and are used only to verify that Deck handles retry-budget
exhaustion and retry actions as expected.

This branch should not change CI workflows, repository settings, or runtime
code.

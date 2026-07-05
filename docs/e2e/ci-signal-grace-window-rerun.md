# CI Signal Grace-Window Validation Rerun

This note supports issue #856, the T-S8 rerun for Claude Deck's CI signal grace-window validation.

The scenario uses the default dispatch interval of 60 seconds and the check-signal grace window of 120 seconds. The agent change is intentionally small and documentation-only so CI can complete normally.

Expected validation flow:

- Generalist opens a draft PR linked to issue #856.
- The PR remains unmerged by the agent.
- Deck observes the CI signal after the grace window and hands the passing PR to human review under merge_policy=human.

This rerun must not change CI workflows, build configuration, repository settings, code, or unrelated docs.

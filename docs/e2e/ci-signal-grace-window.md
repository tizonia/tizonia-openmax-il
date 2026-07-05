# CI Signal Grace-Window Validation

This note records the T-S8 end-to-end validation for Claude Deck autonomous
dispatch on issue #854.

The scenario exercises the default CI signal grace window with a 60 second
dispatch interval and a 120 second check-signal grace period. The expected
path is a tiny docs-only draft PR that lets Deck observe CI status without
changing workflows, build configuration, repository settings, or code.

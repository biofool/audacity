# Executive Summary

## What this project is

This repository is a fork of **Audacity 4** (the in-development, Qt-based rewrite of the Audacity audio editor) that adds **guided onboarding** to the application: a first-run welcome tour and richer tooltips on the main workspace controls.

## Benefits for somatic studios and fields

Somatic teachers and small studios often record classes, guided sessions, and instructional media themselves. Audacity is the tool many of them already reach for. This fork lowers the cost of learning it.

### Guided learning for audio tools used in teaching

- A six-step welcome tour introduces new users to the core workflow: audio device setup, adding a track, recording, selecting and editing, applying effects, and saving or sharing.
- The tour runs once, after first-launch dialogs close, so it does not interrupt setup.
- Enriched tooltips explain what workspace buttons do — title, short description, and keyboard shortcut where one exists — so users can keep learning after the tour ends.
- Steps that point at a control the build does not have are skipped automatically, so the tour degrades gracefully instead of breaking.

### Practical value for studios

- A teacher or volunteer can record and edit class audio without prior experience in a multi-track editor.
- No subscription cost: Audacity is GPLv3 open source.
- The changes are small and additive, so they track upstream Audacity development without forking the audio engine.

## Important caveat

This work targets Audacity 4, which is still under heavy development and is not a stable release. Studios that need a production-ready editor today should use released Audacity 3.x builds. The tour and tooltip changes here have also not been merged upstream.

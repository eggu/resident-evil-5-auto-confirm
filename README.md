# RE5 Auto Confirm

[한국어](README.ko.md)

<img src="assets/logo.png" alt="RE5 Auto Confirm" width="320">

Automatically confirms OK-only system messages in Resident Evil 5, Steam PC DirectX 9. Yes/No choices remain manual. Optional conveniences include shorter save/load message display, separate pause/local co-op inputs, extra cutscene skip buttons, and startup movie/logo skipping.

## Install or upgrade

1. Exit the game. In Steam, right-click Resident Evil 5 → Manage → Browse local files.
2. An ASI loader is required. Use the ASI loader already working with your game; this package does not include one.
3. Copy `RE5.AutoConfirm.asi` beside `re5dx9.exe`. For a fresh install, copy `RE5.AutoConfirm.ini` too; when upgrading, keep the INI copied from your previous installation. Install one copy only, then start the game.

Mods that alter the same code can prevent a feature from activating; see `RE5.AutoConfirm.log` beside the ASI.

Map, Partner AI, Inventory Tweaks and Korean translation mods are separate and are not included. Keep their files when upgrading this mod.

## Settings and controls

Open `RE5.AutoConfirm.ini` in Notepad. Under `[Features]`, `1` enables a feature and `0` restores the original game behavior after restarting the game.

| Setting | Default | Effect |
| --- | --- | --- |
| `AutoConfirmOK` | 1 | Automatically confirms OK-only system messages; Yes/No choices stay manual |
| `ShortSaveMessages` | 1 | Shorter minimum save/load message display time; actual file operations still complete normally |
| `CoopInput` | 1 | Esc/Start pauses; Shift+Esc or LB+Start requests local co-op from the pause screen |
| `ExtraSkipButtons` | 1 | Adds controller X/A to existing Back cutscene/event skipping |
| `UnlockSkipRestrictions` | 0 | Experimental: allows skipping past the native event cutoff and selected blocked event restrictions |
| `SkipStartupMovies` | 0 | Experimental: skips startup movies and CAPCOM/Dolby logos through the original completion paths |

`[Timing]` sets `AutoConfirmSeconds` and `SaveMessageSeconds` (each 0–30 seconds, default 0.1). Set all six feature switches to `0` to leave the original behavior intact. Changes require a game restart. Menus, selection sounds, title Start delays, background transitions and menu fades keep their original timing.

Back remains available. Co-op skip confirmation, gameplay action/reload/map bindings and the original on-screen skip hints remain. Local co-op still requires the game's normal device/session conditions.

## Limitations

Only the matching 32-bit Steam DX9 code is supported. Other editions or conflicting code changes are rejected for the affected feature. Unsupported features can stay inactive while other features run.

Startup skipping is off by default. It keeps the title screen, rating/disclaimer screens and native save-dialog procedure. A brief frame or fade can remain. Startup skipping is experimental: if a save error appears, exit without accepting save creation/replacement, set `SkipStartupMovies=0`, and restart.

`UnlockSkipRestrictions` is experimental and off by default. It may affect progression in some scenes. The extra X/A buttons work without enabling this option.

These UI timing changes do not fix renderer stutter or Korean translation/font issues.

## Uninstall

Exit the game and delete `RE5.AutoConfirm.asi`. You may also remove its INI and log. The game executable and save files are not patched by this mod.

## Build

Requires Python 3, a host C compiler, and `i686-w64-mingw32-gcc` on PATH.

```sh
python3 build.py
```

Outputs are in `dist`. To run the synthetic Windows checks, copy the project to Windows with Python installed and run `python tests/run_windows.py` after building. These checks do not launch the game.

## Credits and license

Created by [eggu](https://github.com/eggu). Resident Evil 5 belongs to CAPCOM. The ASI loader is a separate project and is not bundled. Source code is provided under the [MIT License](LICENSE). The cover artwork was created with AI image generation in the style of the author's other mod logos.

# Hearthsong

An in-game Spotify widget for Skyrim Special Edition and Anniversary Edition - Steam and GOG - (SKSE plugin), in the look of Hearthframe's Modern style.

It controls the Spotify desktop app through Windows' own media controls (the ones behind your keyboard's
media keys), so it needs no Spotify login, no developer key and no internet calls, and it works with free
Spotify too. Spotify has to be open on the same PC.

## Requirements

- Skyrim **SE 1.5.97**, **AE 1.6.x / 1.7.x** (Steam) or the **GOG** builds (1.6.659, 1.6.1179) - one DLL for all of them
- SKSE64 for your game version (GOG has its own SKSE build)
- Address Library for SKSE Plugins (the "All in one" package covers SE, AE and GOG)
- Windows 10 version 1809 or newer, or Windows 11
- The Spotify desktop app (other players work too, see `sMusicApp`)

## Using it

| Key | What it does |
| --- | --- |
| **F7** | Show / hide the widget |
| **F6** | Cursor mode: a cursor to click, drag and resize the widget (Esc or F6 again puts it away) |
| **Ctrl + Z** | In cursor mode: undo the last move or resize |

Every key can be changed in the settings (the gear on the widget) or in `Hearthsong.ini`. Optional
play/pause, next, previous and volume keys work without the cursor.

- **While playing** (no cursor) the widget shows just the music: no settings/close buttons and no volume section,
  and it's that much shorter. They come back in cursor mode.
- **Move:** in cursor mode, drag the title bar (or the slim grip strip when the title bar is off).
- **Resize:** drag any edge or corner. Wide and short becomes a horizontal bar, tall and narrow a portrait
  layout. Parts that don't fit (volume, album art) step aside instead of being cut off.
- **Scale / opacity / parts:** in the settings window, or Ctrl + mouse wheel over the widget for the scale.

### Safeguards against moving it by accident

- Outside cursor mode the widget ignores the mouse completely: clicks go to the game.
- It can only be picked up by its title bar and its edges, never by the middle.
- It only moves after the button is held for a moment (150 ms) **and** the mouse has travelled a few pixels.
- Combat: cursor mode switches off when a fight starts, and can't be switched on during one.
- Lock position and size, undo (Ctrl + Z and a button in the settings), and it always stays on screen,
  also after a resolution change.
- Optional: fade out during combat.

## Building

GitHub Actions builds it on every push (Visual Studio 2022, vcpkg, CommonLibSSE-NG from alandtse/CommonLibVR `ng` as a submodule,
which reads game structures through version-aware accessors so one DLL fits every runtime).
The packaged mod is published to the `dist` branch and as a workflow artifact.

Fonts: Cinzel and Inter (SIL Open Font License, see `assets/fonts`).

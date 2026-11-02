# Vitrine

A visual shelf for your plugins, by Pouria Motabean. Every plugin and instrument as a card, sorted automatically, plus your own decks.

## Version 0.6.3

- VST2 plugins get Cubase's pictures too: Vitrine reads the plugin name hidden in the picture's file name and matches it (no plugin is opened).
- When a plugin leaves the library (uninstalled, or a wrong card from 0.6.1), its picture is deleted from disk and it is taken out of your decks. Vitrine never deletes plugin files.

## Version 0.6.2

- Fixed: 0.6.1 added many files that are not plugins (helper DLLs from the folders it searched). Every VST2 file is now checked before it is listed: it must be a 64-bit DLL that exports VSTPluginMain. The wrong cards disappear on the next Rescan.
- More > "Show the scan report": every folder searched and every DLL taken or skipped, with the reason.

## Version 0.6.1

- VST2 plugins are also found in the VST2 folders set in Cubase (Plug-in Manager), and in the folders added under More > Folders (e.g. C:\64\SoundToys).
- A VST2 plugin that also exists as VST3 (e.g. "LittleAlterBoy" / "Little AlterBoy") shows only once.

## Version 0.6.0

- **Ready-made pictures first**, without opening any plugin:
  1. Cubase's own plugin pictures (Documents\Steinberg\VST Thumbnails, named by the plugin's class id),
  2. the maker's picture inside the plugin (Contents\Resources\Snapshots).
  They are copied at start-up, after each scan and before "Take pictures"; only plugins without a picture (or with a black one) get them. Pictures you chose yourself are never replaced.
- The next scan reads each plugin's class id once (needed to match Cubase's pictures). Your categories, decks, counters and pictures stay.
- Right-click a card: "Use Cubase's / the maker's picture".

## Version 0.5.2

- Pictures of plugins inside shells (Waves) work: only the one plugin is opened, not the whole shell.
- Pictures wait longer for the plugin to draw and keep the most complete of several shots (fewer black or half-drawn pictures).
- More > "Retake pictures that look black or half drawn" finds such pictures and takes them again. "Retake pictures that failed" now also covers the Waves plugins that failed before.

## Version 0.5.1

- Plugin "shells" (e.g. Waves: one VST3 file holding hundreds of plugins) are now read completely: every plugin inside gets its own card. The list is read from the plugin's own factory without starting each plugin, and the scan waits longer for such files.
- Files that were only listed by file name last time are read again on the next scan.

## Version 0.5.0

- **Categories**: drag a card onto a category on the left to move it there; hold **Shift** while dropping to keep it in both. Right-click a card: "Move to category" or "Show in categories" (tick several).
- **Hide from one category only** (right-click a card inside a category), or **Hide everywhere**. More > "Hidden plugins" (also in Sidecar's "..." menu) shows them; click a card there to bring it back.
- **Use counter** on every card: how many times it was loaded through Vitrine (0 = never, dimmed). More > Sort > "Least used first" brings forgotten plugins to the front.
- Pictures are now linked to the plugin's maker + name, so they survive plugin updates that install elsewhere. Existing pictures are moved over automatically; nothing is photographed again.

## Version 0.4.0

- **Pictures**: More > "Take pictures of all plugins". Each VST3 plugin is opened by a helper process for a few seconds and its window is photographed (also GPU-drawn ones). A plugin that fails (black window, licence popup, crash) keeps its icon card and is not retried until you choose "Retake pictures that failed".
- Right-click a card: take its picture again, or use any image file as its picture.
- **Keyboard shortcuts**: right-click a card > "Set a keyboard shortcut". Ctrl/Alt + a letter or number, or an F key. They work everywhere, also while Cubase is in front, and load the plugin on the selected track. The key is shown on the card.
- The Folders button became **More** (folders, rescan, pictures, data folder).

## Version 0.3.7

- New instrument track: Cubase's Add Track command has different names; Vitrine now tries them one after another (every 2.5 s until a new track appears) and remembers the one that works. The diagnostics show which one it was.

## Version 0.3.6

- Plugin windows stay open after loading (Cubase's Edit is a toggle; Vitrine no longer switches it back off).
- Instruments: if the selected track is an instrument track with no instrument yet, the instrument goes straight onto it. Otherwise Vitrine asks Cubase for a new instrument track. If Cubase does not add one, Vitrine says so after a while.

## Version 0.3.5

- Insert slots: Vitrine now listens to Cubase's plugin-identity reports for each slot (slot titles are only numbers), and also remembers the slots it filled itself, so each new effect goes into the next free slot.
- New instrument track: the Add Track command is now really triggered (Cubase reacts only to a held button press).
- Sidecar shows 2 columns by default; "..." > Columns to choose 1, 2 or 3. Small cards drop the format tag.

## Version 0.3.4

- Fixed: Cubase names empty insert slots by their number ("1", "2"...); those now count as empty, so effects go into the first free slot again, then the next one.
- Sidecar is narrower (about 310 screen pixels, like Cubase's own right zone).
- One mode button: it says **Sidecar** in the big window and **Full** in the narrow one.

## Version 0.3.3

- Effects go into the first EMPTY insert slot, and the next one into the next empty slot (Cubase's own insert viewers report which slots are taken).
- The plugin window opens after loading, like Cubase's auto-open.
- New instrument track: the instrument now waits until the new track really exists before loading (it used to land on the old track).
- Cards can be much smaller (Tiny size).

## Version 0.3.2

- Sidecar is 300 px wide (like Cubase's right zone) and sits on the right edge of Cubase's window.
- Better detection of empty insert slots; when none is found, the message lists what Cubase reports.

## Version 0.3.1

- Instruments: a click makes a **new instrument track** with that instrument (Cubase's own Add Track > Instrument). Ctrl+click swaps the instrument on the selected instrument track instead.

## Version 0.3.0

- **Cubase link**: select a track in Cubase, click a card, the plugin goes into the first free insert slot. Instruments go into the instrument slot of a selected instrument track. Needs Cubase 15.0.20 or newer and loopMIDI (open it and press + twice).
- Vitrine puts its MIDI Remote script into Cubase's script folder by itself. The status at the top shows the connection and the selected track; click it for help and diagnostics.
- App icon. Full / Sidecar switch. Card size is kept in Sidecar too (and remembered).

## Version 0.2.0

- **Sidecar** button: a narrow window on the right edge of the screen, always on top of Cubase. **Wide** brings the big window back. Any narrow window switches to this compact layout by itself.
- **Hide from Vitrine** (right-click a card): the plugin disappears from every view; files are not touched. Bring it back from **Hidden** on the left.
- Faster, safer scan: most VST3 plugins are read from their own description file without starting them, so helper apps like UA Connect no longer pop up.

## Version 0.1.0

- Finds your VST3 plugins (standard folders plus any folder you add with **Folders**) and lists VST2 plugins by file name.
- The first scan takes a few minutes. After that only new or updated plugins are scanned again. **Rescan** checks now.
- Each plugin is opened by a separate helper process while scanning, so a broken plugin cannot crash Vitrine.
- Automatic categories: EQ, Compressor, Limiter, Reverb, Delay, Saturation, Modulation, Pitch & Tune, Mastering, Utility, Synth, Sampler, Keys, Drums, Other.

## Using it

- **Your decks**: "+ New deck" on the left (or Ctrl+N). Drag cards onto a deck's name. A plugin can be in many decks.
- Inside a deck, drag cards to change their order.
- Right-click a deck: rename, move, delete. Double-click its name to rename. Drag decks to reorder them.
- **Favorites**: hover a card and click its star, or drag the card onto Favorites.
- Right-click a card: decks, favorite, change its category, show in folder.
- **Search**: just start typing. Esc clears.
- **SIZE** slider: card size.
- Drag-and-drop from Vitrine into Cubase is not possible (Cubase does not accept plugins dropped from other apps). Loading will be one click on a card, via the Cubase link.

Your library is saved in `%APPDATA%\Vitrine\library.json`.

## Notes

- New instrument tracks: Cubase shows its Add Track dialog; press Add Track there.
- VST2 plugins are listed but cannot be photographed (Vitrine opens VST3 only).

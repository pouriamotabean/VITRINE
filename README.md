# Vitrine

A visual shelf for your plugins, by Pouria Motabean. Every plugin and instrument as a card, sorted automatically, plus your own decks.

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

## Coming next

- Cubase link: click a card and the plugin lands on the selected track (Cubase 15.0.20 or newer).
- Pictures of each plugin's own interface on its card, keyboard shortcuts, Sidecar and Mini views.

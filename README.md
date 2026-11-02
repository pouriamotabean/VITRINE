# Vitrine

A visual shelf for your plugins, by Pouria Motabean. Every plugin and instrument as a card, sorted automatically, plus your own decks.

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

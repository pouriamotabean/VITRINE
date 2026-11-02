# Vitrine

A visual shelf for your plugins, by Pouria Motabean. Every plugin and instrument as a card, sorted automatically, plus your own decks.

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

Your library is saved in `%APPDATA%\Vitrine\library.json`.

## Coming next

- Cubase link: click a card and the plugin lands on the selected track (Cubase 15.0.20 or newer).
- Pictures of each plugin's own interface on its card, keyboard shortcuts, Sidecar and Mini views.

// Vitrine <-> Cubase link (MIDI Remote script)            version: __VERSION__
// by Pouria Motabean. Installed and updated automatically by Vitrine.
//
// Vitrine sends SysEx on the loopMIDI port "Vitrine"; this script answers on "Vitrine Back".
// Message: F0 7D 56 <cmd> <hex-encoded UTF-8 text> F7
//   'P' ping                         -> 'H' hello (+ API info)
//   'L' load   text = kind \t name \t vendor
//              kind: F = effect into first free insert, I = instrument into the selected
//              instrument track, N = new instrument track with this instrument
//   'D' diagnostics                  -> 'D' text dump of the selected channel
// Replies: 'H' hello, 'T' selected track title, 'O' ok text, 'E' error text, 'D' dump

var midiremote_api = require('midiremote_api_v1')

var deviceDriver = midiremote_api.makeDeviceDriver('PouriaMotabean', 'Vitrine', 'Pouria Motabean')
var midiInput = deviceDriver.mPorts.makeMidiInput('Vitrine')
var midiOutput = deviceDriver.mPorts.makeMidiOutput('Vitrine Back')

deviceDriver.makeDetectionUnit().detectPortPair(midiInput, midiOutput)
    .expectInputNameEquals('Vitrine')
    .expectOutputNameEquals('Vitrine Back')

// loopMIDI's default names work too: press + twice in loopMIDI
deviceDriver.makeDetectionUnit().detectPortPair(midiInput, midiOutput)
    .expectInputNameEquals('loopMIDI Port')
    .expectOutputNameEquals('loopMIDI Port 1')

var surface = deviceDriver.mSurface
// one tiny label so the surface is not empty
surface.makeLabelField(0, 0, 4, 1)

// a hidden "button" bound to Cubase's own Add Track > Instrument command
var addInstrumentTrack = surface.makeCustomValueVariable('vitrineAddInstrumentTrack')

var page = deviceDriver.mMapping.makePage('Vitrine')
var host = page.mHostAccess
var selected = host.mTrackSelection.mMixerChannel
var da = host.makeDirectAccess(selected)

page.makeCommandBinding(addInstrumentTrack, 'Add Track', 'Instrument')

var gMapping = null
var gPending = null          // instrument waiting for its new track
var gDevice = null
var gTrackTitle = ''

// ---------------------------------------------------------------- helpers
function toHex(str) {
    var utf8 = unescape(encodeURIComponent(str))
    var out = []
    for (var i = 0; i < utf8.length; ++i) {
        var c = utf8.charCodeAt(i)
        out.push('0123456789ABCDEF'.charCodeAt(c >> 4))
        out.push('0123456789ABCDEF'.charCodeAt(c & 15))
    }
    return out
}

function fromHex(bytes, start, end) {
    var s = ''
    for (var i = start; i + 1 < end; i += 2) {
        var h = String.fromCharCode(bytes[i]) + String.fromCharCode(bytes[i + 1])
        s += '%' + h
    }
    try { return decodeURIComponent(s) } catch (e) { return '' }
}

function send(cmd, text) {
    if (!gDevice) return
    var msg = [0xF0, 0x7D, 0x56, cmd.charCodeAt(0)]
    var body = toHex(text || '')
    for (var i = 0; i < body.length; ++i) msg.push(body[i])
    msg.push(0xF7)
    try { midiOutput.sendMidi(gDevice, msg) } catch (e) {}
}

function lower(s) { return ('' + (s || '')).toLowerCase() }

function typeOf(id) {
    try { return '' + da.getObjectTypeName(gMapping, id) } catch (e) { return '?' }
}

function titleOf(id) {
    try { return '' + da.getObjectTitle(gMapping, id) } catch (e) { return '' }
}

function childCount(id) {
    try { return da.getNumberOfChildObjects(gMapping, id) } catch (e) { return 0 }
}

function child(id, i) {
    try { return da.getChildObjectID(gMapping, id, i) } catch (e) { return -1 }
}

// first child whose type name contains one of the words, else the fallback index
function findChild(parent, words, fallbackIndex) {
    var n = childCount(parent)
    for (var i = 0; i < n; ++i) {
        var t = lower(typeOf(child(parent, i)))
        for (var w = 0; w < words.length; ++w)
            if (t.indexOf(words[w]) >= 0) return child(parent, i)
    }
    return fallbackIndex < n ? child(parent, fallbackIndex) : -1
}

function isEmptySlot(slotID) {
    var t = lower(titleOf(slotID))
    return t === '' || t === 'no effect' || t === 'no instrument' || t === 'empty' || t === '-' || t === 'none'
}

// find a plugin UID by name (and vendor) in the plugin collections for this slot
function findPluginUID(slotID, name, vendor) {
    var pm = da.mPluginManager
    if (!pm || !pm.getNumberOfPluginCollections) return { error: 'This Cubase has no plugin manager in MIDI Remote (needs 15.0.20 or newer).' }

    var count = pm.getNumberOfPluginCollections(gMapping, slotID)
    var order = []
    var def = pm.getIndexOfDefaultPluginCollection ? pm.getIndexOfDefaultPluginCollection(gMapping, slotID) : -1
    var act = pm.getIndexOfActivePluginCollection ? pm.getIndexOfActivePluginCollection(gMapping, slotID) : -1
    if (def >= 0) order.push(def)
    if (act >= 0 && act !== def) order.push(act)
    for (var c = 0; c < count; ++c) if (c !== def && c !== act) order.push(c)

    var n = lower(name), v = lower(vendor)
    var loose = null
    for (var k = 0; k < order.length; ++k) {
        var col = pm.getPluginCollectionByIndex(gMapping, slotID, order[k])
        if (!col || !col.mEntries) continue
        for (var e = 0; e < col.mEntries.length; ++e) {
            var en = col.mEntries[e]
            var en_n = lower(en.mPluginName)
            if (en_n === n) {
                if (v === '' || lower(en.mPluginVendor) === v) return { uid: en.mPluginUID, name: en.mPluginName }
                if (!loose) loose = { uid: en.mPluginUID, name: en.mPluginName }
            }
        }
    }
    if (loose) return loose

    // last try: names that start the same way ("Pro-Q 3" vs "FabFilter Pro-Q 3")
    for (var k2 = 0; k2 < order.length; ++k2) {
        var col2 = pm.getPluginCollectionByIndex(gMapping, slotID, order[k2])
        if (!col2 || !col2.mEntries) continue
        for (var e2 = 0; e2 < col2.mEntries.length; ++e2) {
            var x = lower(col2.mEntries[e2].mPluginName)
            if (x.length > 3 && (x.indexOf(n) >= 0 || n.indexOf(x) >= 0)) return { uid: col2.mEntries[e2].mPluginUID, name: col2.mEntries[e2].mPluginName }
        }
    }
    return { error: '"' + name + '" is not in Cubase\'s plugin list.' }
}

// ---------------------------------------------------------------- commands
function loadPlugin(kind, name, vendor) {
    if (!gMapping) { send('E', 'Cubase link not ready yet: click into the project once.'); return }

    var base = da.getBaseObjectID(gMapping)
    if (base === undefined || base < 0) { send('E', 'Select a track first.'); return }

    var baseType = lower(typeOf(base))
    var slotID = -1
    var where = ''

    if (kind === 'I') {
        if (baseType.indexOf('synth') < 0 && baseType.indexOf('instrument') < 0) {
            send('E', 'Select an instrument track first (selected: ' + (gTrackTitle || baseType) + ').')
            return
        }
        slotID = findChild(base, ['instrumentslot', 'synthslot', 'instrument'], 9)
        where = 'instrument slot'
    } else {
        if (baseType.indexOf('midi') === 0) { send('E', 'MIDI tracks have no audio inserts. Select an audio, group or instrument track.'); return }
        var inserts = findChild(base, ['insert'], 3)
        if (inserts < 0) { send('E', 'Could not find the insert slots of ' + gTrackTitle + '.'); return }
        var n = childCount(inserts)
        for (var i = 0; i < n; ++i) {
            var s = child(inserts, i)
            if (isEmptySlot(s)) { slotID = s; where = 'insert ' + (i + 1); break }
        }
        if (slotID < 0) { send('E', 'All insert slots on ' + gTrackTitle + ' are full.'); return }
    }

    if (slotID < 0) { send('E', 'No free slot found on ' + gTrackTitle + '.'); return }

    var found = findPluginUID(slotID, name, vendor)
    if (found.error) { send('E', found.error); return }

    var ok = false
    try { ok = da.mPluginManager.trySetSlotPlugin(gMapping, slotID, found.uid, false) } catch (e) { send('E', 'Cubase refused: ' + e); return }

    if (ok === false) send('E', 'Cubase could not load ' + found.name + '.')
    else send('O', found.name + ' -> ' + gTrackTitle + ', ' + where)
}

function dump() {
    if (!gMapping) { send('D', 'no mapping yet'); return }
    var base = da.getBaseObjectID(gMapping)
    var lines = ['track: ' + gTrackTitle + '  base=' + base + ' type=' + typeOf(base)]
    function walk(id, depth) {
        var n = childCount(id)
        for (var i = 0; i < n && i < 24; ++i) {
            var c = child(id, i)
            var pad = ''
            for (var d = 0; d < depth; ++d) pad += '  '
            lines.push(pad + '[' + i + '] ' + typeOf(c) + '  "' + titleOf(c) + '"')
            if (depth < 2) walk(c, depth + 1)
        }
    }
    walk(base, 1)
    var pm = da.mPluginManager
    if (pm && pm.getNumberOfPluginCollections) {
        var ins = findChild(base, ['insert'], 3)
        var s0 = child(ins, 0)
        var cnt = pm.getNumberOfPluginCollections(gMapping, s0)
        lines.push('collections for insert 1: ' + cnt)
        for (var k = 0; k < cnt; ++k) {
            var col = pm.getPluginCollectionByIndex(gMapping, s0, k)
            lines.push('  ' + k + ': ' + (col ? col.mName : '?') + ' (' + (col && col.mEntries ? col.mEntries.length : 0) + ')')
        }
    } else lines.push('no plugin manager')
    send('D', lines.join('\n'))
}

// New instrument track: run Cubase's Add Track > Instrument, then load the
// instrument into the new (selected) track when Cubase reports the selection change.
function newInstrumentTrack(name, vendor) {
    if (!gDevice) { send('E', 'Cubase link not ready yet.'); return }
    gPending = { name: name, vendor: vendor, prevTitle: gTrackTitle, at: Date.now() }
    try {
        addInstrumentTrack.setProcessValue(gDevice, 1)
        addInstrumentTrack.setProcessValue(gDevice, 0)
    } catch (e) {
        gPending = null
        send('E', 'Could not add a track: ' + e)
        return
    }
    send('O', 'Adding an instrument track for ' + name + '...')
}

function tryPending() {
    if (!gPending) return
    if (Date.now() - gPending.at > 20000) { gPending = null; return }   // the new track never came
    if (gTrackTitle === gPending.prevTitle) return
    var base = da.getBaseObjectID(gMapping)
    var t = lower(typeOf(base))
    if (t.indexOf('synth') < 0 && t.indexOf('instrument') < 0) return
    var p = gPending
    gPending = null
    loadPlugin('I', p.name, p.vendor)
}

// ---------------------------------------------------------------- wiring
page.mOnActivate = function (activeDevice, activeMapping) {
    gDevice = activeDevice
    gMapping = activeMapping
    try { da.activate(activeMapping) } catch (e) {}
    send('H', 'api=' + (da.mPluginManager && da.mPluginManager.getNumberOfPluginCollections ? '1.3' : 'old'))
}

selected.mOnTitleChange = function (activeDevice, activeMapping, title) {
    gDevice = activeDevice
    gMapping = activeMapping
    gTrackTitle = '' + title
    send('T', gTrackTitle)
    tryPending()
}

midiInput.mOnSysex = function (activeDevice, message) {
    gDevice = activeDevice
    var len = message.length
    if (len < 5 || message[1] !== 0x7D || message[2] !== 0x56) return
    var cmd = String.fromCharCode(message[3])
    var text = fromHex(message, 4, len - 1)

    try {
        if (cmd === 'P') send('H', 'api=' + (da.mPluginManager && da.mPluginManager.getNumberOfPluginCollections ? '1.3' : 'old') + '\ttrack=' + gTrackTitle)
        else if (cmd === 'L') {
            var parts = text.split('\t')
            if (parts[0] === 'N') newInstrumentTrack(parts[1] || '', parts[2] || '')
            else loadPlugin(parts[0], parts[1] || '', parts[2] || '')
        }
        else if (cmd === 'D') dump()
    } catch (e) {
        send('E', 'Script error: ' + e)
    }
}

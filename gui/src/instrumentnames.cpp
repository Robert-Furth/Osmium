#include "instrumentnames.h"

#include <array>
#include <unordered_map>

// Instrument names (not taking into account instrument banks)
const std::array g_instrument_names = {
    // Piano
    "Grand Piano",
    "Bright Grand Piano",
    "Electric Grand Piano",
    "Honky-Tonk",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavinet",

    // Chromatic percussion
    "Celesta", // 8
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",

    // Organs
    "Hammond Organ", // 16
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Bandoneon",

    // Guitars
    "Nylon Guitar", // 24
    "Steel Guitar",
    "Jazz Guitar",
    "Clean Electric Guitar",
    "Muted Electric Guitar",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar Harmonics",

    // Bass
    "Acoustic Bass", // 32
    "Fingered Bass",
    "Picked Bass",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",

    // Strings
    "Violin", // 40
    "Viola",
    "Cello",
    "Contrabass",
    "Tremelo Strings",
    "Pizzicato Strings",
    "Harp",
    "Timpani",

    // Ensemble
    "Strings", // 48
    "Slow Strings",
    "Synth Strings 1",
    "Synth Strings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",

    // Brass
    "Trumpet", // 56
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass",
    "Synth Brass 1",
    "Synth Brass 2",

    // Reeds
    "Soprano Sax", // 64
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet", // represent!

    // Pipes
    "Piccolo", // 72
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",

    // Synth Leads
    "Square Lead", // 80
    "Saw Lead",
    "Synth Calliope",
    "Chiffer Lead",
    "Charang Lead",
    "Solo Synth Voice",
    "Fifth Saws",
    "Bass & Lead",

    // Synth Pads
    "Fantasia Pad", // 88
    "Warm Pad",
    "Polysynth Pad",
    "Space Voice Pad",
    "Bowed Glass Pad",
    "Metallic Pad",
    "Halo Pad",
    "Sweep Pad",

    // Synth Effects
    "Ice Rain", // 96
    "Soundtrack",
    "Crystal",
    "Atmosphere",
    "Brightness",
    "Goblin",
    "Echo Drops",
    "Star Theme",

    // Ethnic
    "Sitar", // 104
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bagpipes",
    "Fiddle",
    "Shanai",

    // Percussive
    "Tinkle Bell", // 112
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",

    // SFX
    "Guitar Fret", // 120
    "Breath",
    "Seashore",
    "Birdsong",
    "Telephone",
    "Helicopter",
    "Applause",
    "Gunshot",
};

// NOLINTNEXTLINE(clazy-non-pod-global-static)
const std::unordered_map<int, const char*> g_percussion_names = {
    {0, "Standard Kit"},
    {8, "Room Kit"},
    {16, "Power Kit"},
    {24, "Electronic Kit"},
    {25, "TR-808"},
    {32, "Jazz Kit"},
    {40, "Brush Kit"},
    {48, "Orchestra Kit"},
    {56, "SFX Kit"},
};

QString get_instrument_name(int patch, int bank, bool is_percussion) {
    if (is_percussion) {
        if (g_percussion_names.contains(patch))
            return g_percussion_names.at(patch);
        return g_percussion_names.at(0);
    }

    // TODO Handle banks
    if (patch < 0 || patch >= g_instrument_names.size())
        return g_instrument_names[0];
    return g_instrument_names[patch];
}

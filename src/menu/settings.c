#include "menu_internal.h"

/**
 * Return the address of the game settings bitfield.
 */
u64 *get_eeprom_settings_pointer(void) {
    return &gRaceSetup.eepromSettings;
}

/**
 * Set a bit in the game settings.
 * Return nonzero when done.
 */
s32 set_eeprom_settings_value(u64 valueToSet) {
    gRaceSetup.eepromSettings |= valueToSet;
    mark_write_eeprom_settings();
    return TRUE;
}

/**
 * Clear a bit in the game settings.
 * Return nonzero when done.
 */
s32 unset_eeprom_settings_value(u64 valueToUnset) {
    gRaceSetup.eepromSettings &= ~valueToUnset;
    mark_write_eeprom_settings();
    return TRUE;
}

/**
 * Return the game settings bitfield.
 */
u64 get_eeprom_settings(void) {
    return gRaceSetup.eepromSettings;
}

/**
 * Returns the current language being used by the game.
 * 0 = English, 1 = German, 2 = French, 3 = Japanese
 * Official Name: frontGetLanguage
 */
s32 get_language(void) {
    s32 language = LANGUAGE_ENGLISH;
    switch (gRaceSetup.eepromSettings & 0xC) {
        case 4:
            language = LANGUAGE_GERMAN;
            break;
        case 8:
            language = LANGUAGE_FRENCH;
            break;
        case 12:
            language = LANGUAGE_JAPANESE;
            break;
    }
    return language;
}

/**
 * Sets the language used by the game. Even though German is not selectable in the US version, it is
 * still fully intact. Japanese is not properly implemented in the US 1.0 version of the game. If you
 * tried to switch to it, all you would see is the word "Japanese" used everywhere as a placeholder.
 */
void set_language(s32 language) {
    u64 langFlag = 0; // English

    switch (language) {
        case LANGUAGE_GERMAN:
            langFlag = 4;
            break;
        case LANGUAGE_FRENCH:
            langFlag = 8;
            break;
        case LANGUAGE_JAPANESE:
            langFlag = 12;
            break;
    }

    gRaceSetup.eepromSettings &= ~0xC;     // Set bits 3 and 4 low first
    gRaceSetup.eepromSettings |= langFlag; // Then set them according to the selected lang

    load_menu_text(language);
    mark_write_eeprom_settings();
}

/**
 * Returns TRUE if the player has adventure two unlocked.
 */
s32 is_adventure_two_unlocked(void) {
    return gRaceSetup.eepromSettings & 1;
}

/**
 * Returns 1 if the player is in adventure 2
 */
s32 is_in_adventure_two(void) {
    return gRaceSetup.isInAdventureTwo;
}

/**
 * Returns 1 if the player is in the two player adventure mode.
 * This will automatically return 0 while in tracks mode.
 */
s32 is_in_two_player_adventure(void) {
    if (is_in_tracks_mode()) {
        return FALSE;
    }
    return gRaceSetup.isInTwoPlayerAdventure;
}

/**
 * Returns 1 if T.T. is available to use, or 0 if not.
 */
s32 is_tt_unlocked(void) {
    return gRaceSetup.activeMagicCodes & CHEAT_CONTROL_TT;
}

/**
 * Returns 1 if Drumstick is available to use, or 0 if not.
 */
s32 is_drumstick_unlocked(void) {
    return gRaceSetup.activeMagicCodes & CHEAT_CONTROL_DRUMSTICK;
}

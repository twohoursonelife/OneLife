#include "KeybindManager.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/game/game.h"
 
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fstream>
#include <string>

SimpleVector<KeybindRecord *> KeybindManager::sActions;
char KeybindManager::sInited = false;
char KeybindManager::sPressed[] = {};

void KeybindManager::init() {
    loadCfg();
    saveCfg();
    sInited = true;
    }

void KeybindManager::deInit() {
    for( int i = 0; i < sActions.size(); i++ ) {
        KeybindRecord *r = sActions.getElementDirect( i );
        delete [] r->actionName;
        delete [] r->displayLabel;
        delete [] r->defaultKeyStr;
        delete r;
        }
    sActions.deleteAll();
    sInited = false;
    }

void KeybindManager::registerAction( const char *inActionName, const char *inDisplayLabel, const char *inDefaultKeyStr, KeybindOptions options ) {
    KeybindRecord *r = new KeybindRecord;
    r->actionName = stringDuplicate( inActionName );
    r->displayLabel = stringDuplicate( inDisplayLabel );
    r->defaultKeyStr = stringDuplicate( inDefaultKeyStr );
    parseKeyString( inDefaultKeyStr, &r->key, &r->modifiers );
    r->options = options;
    sActions.push_back( r );
    }

int KeybindManager::getActionCount() {
    return sActions.size();
    }

KeybindRecord *KeybindManager::getAction( int inIndex ) {
    return sActions.getElementDirect( inIndex );
    }

KeybindRecord *KeybindManager::findAction( const char *inActionName ) {
    for( int i = 0; i < sActions.size(); i++ ) {
        KeybindRecord *r = sActions.getElementDirect( i );
        if( strcmp( r->actionName, inActionName ) == 0 ) return r;
        }
    return NULL;
    }

static void validateBinding( KeybindRecord *inRecord ) {
    if( inRecord->key == 0 && inRecord->modifiers == KEYBIND_MOD_NONE ) return;
    KeybindType type = inRecord->options.type;
    char invalid = false;
    if( type == MODIFIER_ONLY ) {
        char hasKey = inRecord->key != 0;
        char noMod = inRecord->modifiers == KEYBIND_MOD_NONE;
        char multiMod = ( inRecord->modifiers & ( inRecord->modifiers - 1 ) ) != 0;
        if( hasKey || noMod || multiMod ) { invalid = true; }
        }
    else if( type == KEY_ONLY && ( inRecord->key == 0 || inRecord->modifiers != KEYBIND_MOD_NONE ) ) { invalid = true; }
    else if( type == DEFAULT_TYPE && inRecord->key == 0 && inRecord->modifiers != KEYBIND_MOD_NONE ) { invalid = true; }
    if( invalid ) {
        inRecord->key = 0;
        inRecord->modifiers = KEYBIND_MOD_NONE;
        }
    }
static std::string trim( const std::string &inString ) {
    size_t start = inString.find_first_not_of( " \t\r\n" );
    if( start == std::string::npos ) return "";
    size_t end = inString.find_last_not_of( " \t\r\n" );
    return inString.substr( start, end - start + 1 );
    }

void KeybindManager::loadCfg() {
    std::ifstream file( "keybinds.cfg" );
    if( !file.is_open() ) return;

    std::string line;
    while( std::getline( file, line ) ) {
        if( line.empty() || line.rfind( "//", 0 ) == 0 ) continue;
        size_t eq = line.find( '=' );
        if( eq == std::string::npos ) continue;

        std::string key = trim( line.substr( 0, eq ) );
        std::string value = line.substr( eq + 1 );
        size_t comment = value.find( "//" );
        if( comment != std::string::npos ) value = value.substr( 0, comment );
        value = trim( value );

        KeybindRecord *r = findAction( key.c_str() );
        if( r == NULL ) continue;
        parseKeyString( value.c_str(), &r->key, &r->modifiers );
        validateBinding( r );
        }
    }

void KeybindManager::saveCfg() {
    std::ofstream file( "keybinds.cfg" );
    if( !file.is_open() ) return;

    for( int i = 0; i < sActions.size(); i++ ) {
        KeybindRecord *r = sActions.getElementDirect( i );
        if( r->options.preComment != NULL ) file << r->options.preComment << "\n";
        char *keyStr = buildKeyString( r, true, false );
        const char *typeTag = "";
        if( r->options.type == MODIFIER_ONLY ) typeTag = "  // modifier-only";
        else if( r->options.type == KEY_ONLY ) typeTag = "  // key-only";
        unsigned char defaultKey;
        int defaultMods;
        parseKeyString( r->defaultKeyStr, &defaultKey, &defaultMods );
        char changed = r->key != defaultKey || r->modifiers != defaultMods;
        const char *defaultTag = changed ? "  // default: " : "";
        const char *defaultString = changed ? ( r->defaultKeyStr[0] == '\0' ? "[none]" : r->defaultKeyStr ) : "";
        file << r->actionName << " = " << keyStr << defaultTag << defaultString << typeTag << "\n";
        delete [] keyStr;
        if( r->options.postComment != NULL ) file << r->options.postComment << "\n";
        }
    }

void KeybindManager::setBinding( const char *inActionName, unsigned char inKey, int inModifiers ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL ) return;
    r->key = inKey;
    r->modifiers = inModifiers;
    }

void KeybindManager::clearBinding( const char *inActionName ) {
    setBinding( inActionName, 0, KEYBIND_MOD_NONE );
    saveCfg();
    }

void KeybindManager::resetBinding( const char *inActionName ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL ) return;
    parseKeyString( r->defaultKeyStr, &r->key, &r->modifiers );
    saveCfg();
    }

void KeybindManager::parseKeyString( const char *inStr, unsigned char *outKey, int *outModifiers ) {
    *outKey = 0;
    *outModifiers = KEYBIND_MOD_NONE;

    if( inStr == NULL || inStr[0] == '\0' ) return;

    char *copy = stringToLowerCase( inStr );
    int j = 0;
    for( int i = 0; copy[i] != '\0'; i++ ) {
        if( !isspace( copy[i] ) ) {
            copy[j] = copy[i];
            j++;
            }
        }
    copy[j] = '\0';

    int numTokens = 0;
    char **tokens = split( copy, "+", &numTokens );
    delete [] copy;

    for( int i = 0; i < numTokens; i++ ) {
        char *tok = tokens[i];

        if( strcmp( tok, "ctrl" ) == 0 ) *outModifiers |= KEYBIND_MOD_CTRL;
        else if( strcmp( tok, "shift" ) == 0 ) *outModifiers |= KEYBIND_MOD_SHIFT;
        else if( strcmp( tok, "alt" ) == 0 ) *outModifiers |= KEYBIND_MOD_ALT;
        else if( strcmp( tok, "tab" ) == 0 ) *outKey = 9;
        else if( strcmp( tok, "enter" ) == 0 || strcmp( tok, "\\n" ) == 0 ) *outKey = 28;
        else if( strcmp( tok, "for" ) == 0 || strcmp( tok, ">>" ) == 0 ) *outKey = 30;
        else if( strcmp( tok, "back" ) == 0 || strcmp( tok, "<<" ) == 0 ) *outKey = 31;
        else if( strcmp( tok, "space" ) == 0 ) *outKey = ' ';
        else if( strcmp( tok, "__" ) == 0 ) *outKey = ' ';
        else if( strcmp( tok, "none" ) == 0 ) {
            }
        else *outKey = (unsigned char)tok[0];
        delete [] tok;
        }
    delete [] tokens;
    }

char *KeybindManager::buildKeyString( KeybindRecord *inRecord, char inLong, char inUppercase ) {
    if( inRecord == NULL || ( inRecord->key == 0 && inRecord->modifiers == KEYBIND_MOD_NONE ) ) {
        if ( inLong && inUppercase ) return stringDuplicate( "[NONE]" );
        return stringDuplicate( "" );
        }

    char buf[64] = "";

    if( inRecord->modifiers & KEYBIND_MOD_CTRL ) strcat( buf, "ctrl+" );
    if( inRecord->modifiers & KEYBIND_MOD_SHIFT ) strcat( buf, "shift+" );
    if( inRecord->modifiers & KEYBIND_MOD_ALT ) strcat( buf, "alt+" );
    // KEY_ONLY uses shortened strings to fit in smaller input boxes
    char useShort = inRecord->options.type == KEY_ONLY && !inLong;
    if( inRecord->key == 9 ) strcat( buf, "tab" );
    else if( inRecord->key == 28 ) strcat( buf, useShort ? "\\n" : "enter" );
    else if( inRecord->key == 30 ) strcat( buf, useShort ? ">>" : "for" );
    else if( inRecord->key == 31 ) strcat( buf, useShort ? "<<" : "back" );
    else if( inRecord->key == ' ' ) strcat( buf, useShort ? "__" : "space" );
    else if( inRecord->key != 0 ) {
        int len = strlen( buf );
        buf[len] = (char)tolower( inRecord->key );
        buf[len + 1] = '\0';
        }
    else {
        int len = strlen( buf );
        if( len > 0 && buf[len - 1] == '+' ) buf[len - 1] = '\0';
        }

    if( inUppercase ) return stringToUpperCase( buf );
    return stringDuplicate( buf );
    }

char *KeybindManager::buildKeyString( const char *inActionName, char inLong, char inUppercase ) {
    return buildKeyString( findAction( inActionName ), inLong, inUppercase );
    }

char KeybindManager::checkActive( const char *inActionName, char inStrict ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL || ( r->key == 0 && r->options.type != MODIFIER_ONLY ) ) return false;

    int modifiers = r->modifiers;
    char needShift = ( modifiers & KEYBIND_MOD_SHIFT ) != 0;
    char needCtrl = ( modifiers & KEYBIND_MOD_CTRL ) != 0;
    char needAlt = ( modifiers & KEYBIND_MOD_ALT ) != 0;

    if( r->options.type != KEY_ONLY ) {
        char shiftDown = isShiftKeyDown();
        char ctrlDown = isControlKeyDown();
        char altDown = isAltKeyDown();
        if( !inStrict ) {
            if( needShift && !shiftDown ) return false;
            if( needCtrl && !ctrlDown ) return false;
            if( needAlt && !altDown ) return false;
            }
        else {
            if( needShift != shiftDown ) return false;
            if( needCtrl != ctrlDown ) return false;
            if( needAlt != altDown ) return false;
            }
        }

    if( r->options.type == MODIFIER_ONLY ) return true;

    return ( sPressed[r->key] == true );
    }

char KeybindManager::isActive( const char *inActionName ) {
    return checkActive( inActionName, true );
    }

char KeybindManager::isReleased( const char *inActionName ) {
    return !checkActive( inActionName, false );
    }

void KeybindManager::clearAction( const char *inActionName ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL || r->key == 0 ) return;
    sPressed[ r->key ] = false;
    }

void KeybindManager::keyDown( unsigned char inASCII ) {
    // reroute enter key to an unused ASCII code to avoid mixups with ctrl+m ctrl code
    // this means there can't be a keybind that includes both enter and ctrl. could be routed through special key instead?
    if( inASCII == 13 && !isControlKeyDown() ) inASCII = 28;
    if( isControlKeyDown() && inASCII > 0 && inASCII < 27 ) inASCII = 'a' + inASCII - 1; // recover key from ctrl codes
    sPressed[ tolower( inASCII ) ] = true;
    }

void KeybindManager::keyUp( unsigned char inASCII ) {
    if( inASCII == 13 && !isControlKeyDown() ) inASCII = 28;
    if( isControlKeyDown() && inASCII > 0 && inASCII < 27 ) inASCII = 'a' + inASCII - 1;
    sPressed[ tolower( inASCII ) ] = false;
    }

void KeybindManager::clearAllPressed() {
    for( int i = 0; i < 256; i++ ) {
        sPressed[i] = false;
        }
    }

void KeybindManager::specialKeyDown( int inKey ) {
    }

void KeybindManager::specialKeyUp( int inKey ) {
    }


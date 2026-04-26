#include "KeybindManager.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Path.h"
#include "minorGems/io/file/Directory.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>


SimpleVector<KeybindRecord *> KeybindManager::sActions;
char KeybindManager::sInited = false;
char KeybindManager::sShiftDown = false;
char KeybindManager::sControlDown = false;
char KeybindManager::sAltDown = false;
char KeybindManager::sPressed[] = {};

void KeybindManager::init() {
    ensureDirectory();
    loadAll();
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

void KeybindManager::registerAction( const char *inActionName, const char *inDisplayLabel, const char *inDefaultKeyStr, KeybindType inType ) {
    KeybindRecord *r = new KeybindRecord;
    r->actionName = stringDuplicate( inActionName );
    r->displayLabel = stringDuplicate( inDisplayLabel );
    r->defaultKeyStr = stringDuplicate( inDefaultKeyStr );
    r->key = 0;
    r->modifiers = KEYBIND_MOD_NONE;
    r->modifierOnly = false;
    r->keyOnly = false;
    if( inType == KEY_ONLY ) r->keyOnly = true;
    else if( inType == MODIFIER_ONLY ) r->modifierOnly = true;
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

void KeybindManager::loadAll() {
    for( int i = 0; i < sActions.size(); i++ ) {
        KeybindRecord *r = sActions.getElementDirect( i );

        char *path = buildFilePath( r->actionName );
        FILE *f = fopen( path, "r" );
        delete [] path;

        if( f != NULL ) {
            char buf[64] = "";
            fscanf( f, "%63s", buf );
            fclose( f );
            parseKeyString( buf, &r->key, &r->modifiers );
            }
        else {
            resetBinding( r->actionName );
            }
        }
    }

void KeybindManager::saveBinding( const char *inActionName ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL ) return;

    char *path = buildFilePath( inActionName );
    FILE *f = fopen( path, "w" );
    delete [] path;

    if( f == NULL ) return;

    char *keyStr = buildKeyString( r );
    fprintf( f, "%s\n", keyStr );
    delete [] keyStr;

    fclose( f );
    }

void KeybindManager::setBinding( const char *inActionName, unsigned char inKey, int inModifiers ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL ) return;
    r->key = inKey;
    r->modifiers = inModifiers;
    }

void KeybindManager::clearBinding( const char *inActionName ) {
    setBinding( inActionName, 0, KEYBIND_MOD_NONE );
    saveBinding( inActionName );
    }

void KeybindManager::resetBinding( const char *inActionName ) {
    KeybindRecord *r = findAction( inActionName );
    parseKeyString( r->defaultKeyStr, &r->key, &r->modifiers );
    saveBinding( inActionName );
    }

void KeybindManager::parseKeyString( const char *inStr, unsigned char *outKey, int *outModifiers ) {
    *outKey = 0;
    *outModifiers = KEYBIND_MOD_NONE;

    if( inStr == NULL || inStr[0] == '\0' ) return;

    char *copy = stringToLowerCase( inStr );

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

char *KeybindManager::buildKeyString( KeybindRecord *inRecord, char inDisplay ) {
    if( inRecord == NULL || ( inRecord->key == 0 && inRecord->modifiers == KEYBIND_MOD_NONE ) ) return stringDuplicate( "" );

    char buf[64] = "";

    if( inRecord->modifiers & KEYBIND_MOD_CTRL ) strcat( buf, "ctrl+" );
    if( inRecord->modifiers & KEYBIND_MOD_SHIFT ) strcat( buf, "shift+" );
    if( inRecord->modifiers & KEYBIND_MOD_ALT ) strcat( buf, "alt+" );
    // keyOnly uses shortened strings to fit in smaller input boxes
    if( inRecord->key == 9 ) strcat( buf, "tab" );
    else if( inRecord->key == 28 ) strcat( buf, inRecord->keyOnly && !inDisplay ? "\\n" : "enter" );
    else if( inRecord->key == 30 ) strcat( buf, inRecord->keyOnly && !inDisplay ? ">>" : "for" );
    else if( inRecord->key == 31 ) strcat( buf, inRecord->keyOnly && !inDisplay ? "<<" : "back" );
    else if( inRecord->key == ' ' ) strcat( buf, inRecord->keyOnly && !inDisplay ? "__" : "space" );
    else if( inRecord->key != 0 ) {
        int len = strlen( buf );
        buf[len] = (char)tolower( inRecord->key );
        buf[len + 1] = '\0';
        }
    else {
        int len = strlen( buf );
        if( len > 0 && buf[len - 1] == '+' ) buf[len - 1] = '\0';
        }

    if( inDisplay ) return stringToUpperCase( buf );
    return stringDuplicate( buf );
    }

char *KeybindManager::buildKeyString( const char *inActionName, char inDisplay ) {
    return buildKeyString( findAction( inActionName ), inDisplay );
    }

char KeybindManager::checkActive( const char *inActionName, char inStrict ) {
    KeybindRecord *r = findAction( inActionName );
    if( r == NULL || ( r->key == 0 && !r->modifierOnly ) ) return false;

    int modifiers = r->modifiers;
    char needShift = ( modifiers & KEYBIND_MOD_SHIFT ) != 0;
    char needCtrl = ( modifiers & KEYBIND_MOD_CTRL ) != 0;
    char needAlt = ( modifiers & KEYBIND_MOD_ALT ) != 0;

    if( !r->keyOnly ) {
        if( !inStrict ) {
            if( needShift && !sShiftDown ) return false;
            if( needCtrl && !sControlDown ) return false;
            if( needAlt && !sAltDown ) return false;
            }
        else {
            if( needShift != sShiftDown ) return false;
            if( needCtrl != sControlDown ) return false;
            if( needAlt != sAltDown ) return false;
            }
        }

    if( r->modifierOnly ) return true;

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

void KeybindManager::ensureDirectory() {
    char **pathSteps = new char*[1];
    pathSteps[0] = stringDuplicate( "settings" );

    File *keybindsDir = new File( new Path( pathSteps, 1, false ), "keybinds" );

    delete [] pathSteps[0];
    delete [] pathSteps;

    if( !keybindsDir->exists() ) Directory::makeDirectory( keybindsDir );

    delete keybindsDir;
    }

char *KeybindManager::buildFilePath( const char *inActionName ) {
    char **pathSteps = new char*[2];
    pathSteps[0] = stringDuplicate( "settings" );
    pathSteps[1] = stringDuplicate( "keybinds" );

    char *iniName = autoSprintf( "%s.ini", inActionName );
    File *f = new File( new Path( pathSteps, 2, false ), iniName );

    delete [] iniName;
    delete [] pathSteps[0];
    delete [] pathSteps[1];
    delete [] pathSteps;

    char *fullPath = f->getFullFileName();
    delete f;
    return fullPath;
    }

void KeybindManager::keyDown( unsigned char inASCII ) {
    // reroute enter key to an unused ASCII code to avoid mixups with ctrl+m ctrl code
    // this means there can't be a keybind that includes both enter and ctrl. could be routed through special key instead?
    if( inASCII == 13 && !sControlDown ) inASCII = 28;
    if( sControlDown && inASCII > 0 && inASCII < 27 ) inASCII = 'a' + inASCII - 1; // recover key from ctrl codes
    sPressed[ tolower( inASCII ) ] = true;
    }

void KeybindManager::keyUp( unsigned char inASCII ) {
    if( inASCII == 13 && !sControlDown ) inASCII = 28;
    if( sControlDown && inASCII > 0 && inASCII < 27 ) inASCII = 'a' + inASCII - 1;
    sPressed[ tolower( inASCII ) ] = false;
    }

void KeybindManager::clearAllPressed() {
    for( int i = 0; i < 256; i++ ) {
        sPressed[i] = false;
        }
    }

void KeybindManager::specialKeyDown( int inKey ) {
    if( inKey == MG_KEY_LSHIFT || inKey == MG_KEY_RSHIFT ) sShiftDown = true;
    else if( inKey == MG_KEY_LCTRL || inKey == MG_KEY_RCTRL ) sControlDown = true;
    else if( inKey == MG_KEY_LALT || inKey == MG_KEY_RALT ) sAltDown = true;
    }

void KeybindManager::specialKeyUp( int inKey ) {
    if( inKey == MG_KEY_LSHIFT || inKey == MG_KEY_RSHIFT ) sShiftDown = false;
    else if( inKey == MG_KEY_LCTRL || inKey == MG_KEY_RCTRL ) sControlDown = false;
    else if( inKey == MG_KEY_LALT || inKey == MG_KEY_RALT ) sAltDown = false;
    }

char KeybindManager::isShiftDown() {
    return sShiftDown;
    }

char KeybindManager::isControlDown() {
    return sControlDown;
    }

char KeybindManager::isAltDown() {
    return sAltDown;
    }

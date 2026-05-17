#ifndef KEYBIND_MANAGER_INCLUDED
#define KEYBIND_MANAGER_INCLUDED

#include "minorGems/util/SimpleVector.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include <unordered_map>
#include <string>

#define KEYBIND_KEY_TABLE_SIZE (MG_KEY_LAST_CODE + 1)

#define KEYBIND_MOD_NONE 0
#define KEYBIND_MOD_SHIFT (1 << 0)
#define KEYBIND_MOD_CTRL (1 << 1)
#define KEYBIND_MOD_ALT (1 << 2)

enum KeybindType {
    DEFAULT_TYPE,
    MODIFIER_ONLY,
    KEY_ONLY
    };

struct KeybindOptions {
    KeybindType type = DEFAULT_TYPE;
    const char *preComment = NULL;
    const char *postComment = NULL;
};

struct KeybindRecord {
    char *actionName;
    char *displayLabel;
    int key;
    int modifiers;
    char *defaultKeyStr;
    KeybindOptions options;
    };

struct NamedKeyEntry {
    const char *longName;
    const char *shortName;
};

class KeybindManager {

    public:

        // call once after keybind registrations
        static void init();
        static void deInit();

        // creates a KeybindRecord with keybind details. set default key str to "" if it should be unbinded by default.
        static void registerAction( const char *inActionName, const char *inDisplayLabel, const char *inDefaultKeyStr, KeybindOptions options = {} );
        static int getActionCount();
        // gets keybind record by index in sActions
        static KeybindRecord *getAction( int inIndex );
        // gets keybind record by name
        static KeybindRecord *findAction( const char *inActionName );

        // updates keybind records from config file
        static void loadFromFile();
        // rewrites config file with current keybind records
        static void saveToFile();

        // set a keybinds key and modifiers through its record
        static void setBinding( const char *inActionName, int inKey, int inModifiers );
        static void clearBinding( const char *inActionName );
        static void resetBinding( const char *inActionName );

        // takes a key string (e.g. shift+q) and writes into a key and modifiers
        static void parseKeyString( const char *inStr, int *outKey, int *outModifiers );
        // takes a keybind record and returns a key string
        static char *buildKeyString( KeybindRecord *inRecord, char inLong = false, char inUppercase = false );
        static char *buildKeyString( const char *inActionName, char inLong = false, char inUppercase = false );

        static const char *getKeyName( int inKey, int inLong = true );
        static int getKeyCode( const char *inName );

        // returns true if a keybind's key is pressed and its required modifiers are down;
        // extra modifiers are allowed only if no other keybinds conflict
        static char isActive( const char *inActionName );
        // returns false if the key is down regardless of whether extra modifiers are held
        // used for held keybinds to prevent false clears when extra modifiers are held
        static char isReleased( const char *inActionName );

        static void keyDown( int inKey );
        static void keyUp( int inKey );
        static void clearAllPressed();
        // sets a keybinds key in sPressed to 0
        static void clearAction( const char *inActionName );


    private:

        static void initKeys();

        static SimpleVector<KeybindRecord *> sActions;
        static char sInited;
        static char sKeysInited;
        static char sPressed[KEYBIND_KEY_TABLE_SIZE];
        static NamedKeyEntry sNamedKeys[KEYBIND_KEY_TABLE_SIZE];
        static std::unordered_map<std::string, int> sNameToKey;

        static char checkActive( const char *inActionName, char inStrict );
};


#endif

#ifndef KEYBIND_MANAGER_INCLUDED
#define KEYBIND_MANAGER_INCLUDED

#include "minorGems/util/SimpleVector.h"
#include <string>

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
    unsigned char key;
    int modifiers;
    char *defaultKeyStr;
    KeybindOptions options;
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
        static void setBinding( const char *inActionName, unsigned char inKey, int inModifiers );
        static void clearBinding( const char *inActionName );
        static void resetBinding( const char *inActionName );

        // takes a key string (e.g. shift+q) and writes into a key and modifiers
        static void parseKeyString( const char *inStr, unsigned char *outKey, int *outModifiers );
        // takes a keybind record and returns a key string
        static char *buildKeyString( KeybindRecord *inRecord, char inLong = false, char inUppercase = false );
        static char *buildKeyString( const char *inActionName, char inLong = false, char inUppercase = false );

        // returns true if a keybinds key and ONLY its modifiers are pressed (returns false if any extra modifiers pressed)
        static char isActive( const char *inActionName );
        // returns false if the key is down regardless of whether extra modifiers are held
        // used for held keybinds to prevent false clears when extra modifiers are held
        static char isReleased( const char *inActionName );

        static void keyDown( unsigned char inASCII );
        static void keyUp( unsigned char inASCII );
        static void clearAllPressed();
        // sets a keybinds key in sPressed to 0
        static void clearAction( const char *inActionName );

        static void specialKeyDown( int inKey );
        static void specialKeyUp( int inKey );

    private:

        static SimpleVector<KeybindRecord *> sActions;
        static char sInited;
        static char sPressed[256];
        static char checkActive( const char *inActionName, char inStrict );
};


#endif

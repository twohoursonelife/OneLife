
#include "minorGems/util/SettingsManager.h"

#include "minorGems/util/log/AppLog.h"

#include "lineardb3.h"

#define DB LINEARDB3
#define DB_open LINEARDB3_open
#define DB_close LINEARDB3_close
#define DB_get LINEARDB3_get
#define DB_put LINEARDB3_put
// no distinction between put and put_new in lineardb3
#define DB_put_new LINEARDB3_put
#define DB_Iterator  LINEARDB3_Iterator
#define DB_Iterator_init  LINEARDB3_Iterator_init
#define DB_Iterator_next  LINEARDB3_Iterator_next
#define DB_maxStack db.maxOverflowDepth
#define DB_getShrinkSize  LINEARDB3_getShrinkSize
#define DB_getCurrentSize  LINEARDB3_getCurrentSize
#define DB_getNumRecords LINEARDB3_getNumRecords

#include "dbCommon.h"

#define MAP_TIMESEC Time::timeSec()

timeSec_t dbLookTimeGet( int inX, int inY );
void dbLookTimePut( int inX, int inY, timeSec_t inTime );

#include "minorGems/io/file/File.h"




extern char lookTimeDBEmpty;
extern char skipLookTimeCleanup;
extern char skipRemovedObjectCleanup;

// if lookTimeDBEmpty, then we init all map cell look times to NOW
extern int cellsLookedAtToInit;

// version of open call that checks whether look time exists in lookTimeDB
// for each record in opened DB, and clears any entries that are not
// rebuilding file storage for DB in the process
// lookTimeDB MUST be open before calling this
//
// If lookTimeDBEmpty, this call just opens the target DB normally without
// shrinking it.
//
// Can handle max key and value size of 16 and 12 bytes
// Assumes that first 8 bytes of key are xy as 32-bit ints

int DB_open_timeShrunk(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size) {
 
    File dbFile( NULL, path );
   
    if( ! dbFile.exists() || lookTimeDBEmpty || skipLookTimeCleanup ) {
 
        if( lookTimeDBEmpty ) {
            AppLog::infoF( "No lookTimes present, not cleaning %s", path );
            }
       
        int error = DB_open( db,
                                 path,
                                 mode,
                                 hash_table_size,
                                 key_size,
                                 value_size );
 
        if( ! error && ! skipLookTimeCleanup ) {
            // add look time for cells in this DB to present
            // essentially resetting all look times to NOW
           
            DB_Iterator dbi;
   
   
            DB_Iterator_init( db, &dbi );
   
            // key and value size that are big enough to handle all of our DB
            unsigned char key[16];
   
            unsigned char value[12];
   
            while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
                int x = valueToInt( key );
                int y = valueToInt( &( key[4] ) );
 
                cellsLookedAtToInit++;
               
                dbLookTimePut( x, y, MAP_TIMESEC );
                }
            }
        return error;
        }
   
    char *dbTempName = autoSprintf( "%s.temp", path );
    File dbTempFile( NULL, dbTempName );
   
    if( dbTempFile.exists() ) {
        dbTempFile.remove();
        }
   
    if( dbTempFile.exists() ) {
        AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );
 
        delete [] dbTempName;
 
        return DB_open( db,
                            path,
                            mode,
                            hash_table_size,
                            key_size,
                            value_size );
        }
   
    DB oldDB;
   
    int error = DB_open( &oldDB,
                             path,
                             mode,
                             hash_table_size,
                             key_size,
                             value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        path );
        delete [] dbTempName;
 
        return error;
        }
 
   
 
   
 
   
    DB_Iterator dbi;
   
   
    DB_Iterator_init( &oldDB, &dbi );
   
    // key and value size that are big enough to handle all of our DB
    unsigned char key[16];
   
    unsigned char value[12];
   
    int total = 0;
    int stale = 0;
    int nonStale = 0;
   
    // first, just count
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;
 
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );
 
        if( dbLookTimeGet( x, y ) > 0 ) {
            // keep
            nonStale++;
            }
        else {
            // stale
            // ignore
            stale++;
            }
        }
 
 
 
    // optimial size for DB of remaining elements
    unsigned int newSize = DB_getShrinkSize( &oldDB, nonStale );
 
    AppLog::infoF( "Shrinking hash table in %s from %d down to %d",
                   path,
                   DB_getCurrentSize( &oldDB ),
                   newSize );
 
 
    DB tempDB;
   
    error = DB_open( &tempDB,
                         dbTempName,
                         mode,
                         newSize,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        dbTempName );
        delete [] dbTempName;
        DB_close( &oldDB );
        return error;
        }
 
 
    // now that we have new temp db properly sized,
    // iterate again and insert, but don't count
    DB_Iterator_init( &oldDB, &dbi );
 
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );
 
        if( dbLookTimeGet( x, y ) > 0 ) {
            // keep
            // insert it in temp
            DB_put_new( &tempDB, key, value );
            }
        else {
            // stale
            // ignore
            }
        }
 
 
   
    AppLog::infoF( "Cleaned %d / %d stale map cells from %s", stale, total,
                   path );
 
    printf( "\n" );
   
   
    DB_close( &tempDB );
    DB_close( &oldDB );
 
    dbTempFile.copy( &dbFile );
    dbTempFile.remove();
 
    delete [] dbTempName;
 
    // now open new, shrunk file
    return DB_open( db,
                        path,
                        mode,
                        hash_table_size,
                        key_size,
                        value_size );
    }
    
    
    




int getTweakedBaseMap( int inX, int inY );

int dbGet_noCache( int inX, int inY, int inSlot, int inSubCont );

extern char skipTrackingMapChanges;

// Biome settings need to be loaded before this
// as it will call the map generating function
int DB_open_naturalTileShrunk_mapDb(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size) {
 
    File dbFile( NULL, path );
   
    if( ! dbFile.exists() ) {
       
        int error = DB_open( db,
                             path,
                             mode,
                             hash_table_size,
                             key_size,
                             value_size );
 
        return error;
        }
   
    char *dbTempName = autoSprintf( "%s.temp", path );
    File dbTempFile( NULL, dbTempName );
   
    if( dbTempFile.exists() ) {
        dbTempFile.remove();
        }
   
    if( dbTempFile.exists() ) {
        AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );
 
        delete [] dbTempName;
 
        return DB_open( db,
                        path,
                        mode,
                        hash_table_size,
                        key_size,
                        value_size );
        }
   
    DB oldDB;
   
    int error = DB_open( &oldDB,
                         path,
                         mode,
                         hash_table_size,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_naturalTileShrunk",
                        path );
        delete [] dbTempName;
 
        return error;
        }
 
   
 
   
 
   
    DB_Iterator dbi;
   
   
    DB_Iterator_init( &oldDB, &dbi );
   
    // key and value size that are big enough to handle all of our DB
    unsigned char key[16];
   
    unsigned char value[12];
   
    int totalCount = 0;
    int keepCount = 0;
    int discardCount = 0;
   
    // first, just count
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        totalCount++;
       
        int s = valueToInt( &( key[8] ) ); // slot number
        int b = valueToInt( &( key[12] ) ); // sub-container index
        int id = valueToInt( value );
        
        if( s == 0 && b == 0 ) { // the base object
            int x = valueToInt( key );
            int y = valueToInt( &( key[4] ) );
            int naturalTile = getTweakedBaseMap( x, y );
           
            if( naturalTile == id ) {
                // leave out this record if the object is the same as the natural map
                discardCount++;
                continue;
                }
            }
        // otherwise, keep it
        keepCount++;
        }
 
 
 
    // optimial size for DB of remaining elements
    unsigned int newSize = DB_getShrinkSize( &oldDB, keepCount );
 
    AppLog::infoF( "Shrinking hash table in %s from %d down to %d",
                   path,
                   DB_getCurrentSize( &oldDB ),
                   newSize );
 
 
    DB tempDB;
   
    error = DB_open( &tempDB,
                         dbTempName,
                         mode,
                         newSize,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        dbTempName );
        delete [] dbTempName;
        DB_close( &oldDB );
        return error;
        }
 
 
    // now that we have new temp db properly sized,
    // iterate again and insert, but don't count
    DB_Iterator_init( &oldDB, &dbi );
 
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );
 
        int s = valueToInt( &( key[8] ) ); // slot number
        int b = valueToInt( &( key[12] ) ); // sub-container index
        int id = valueToInt( value );
        
        if( s == 0 && b == 0 ) { // the base object
            int naturalTile = getTweakedBaseMap( x, y );
            if( naturalTile == id ) continue;
            }
        
        DB_put_new( &tempDB, key, value );
        }
 
 
   
    AppLog::infoF( "Cleaned %d / %d natural map cells from %s", discardCount, totalCount,
                   path );
 
    printf( "\n" );
   
   
    DB_close( &tempDB );
    DB_close( &oldDB );
 
    dbTempFile.copy( &dbFile );
    dbTempFile.remove();
 
    delete [] dbTempName;
 
    // now open new, shrunk file
    return DB_open( db,
                    path,
                    mode,
                    hash_table_size,
                    key_size,
                    value_size );
    }

// Biome settings need to be loaded before this
// as it will call the map generating function
int DB_open_naturalTileShrunk(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size) {
 
    File dbFile( NULL, path );
   
    if( ! dbFile.exists() ) {
       
        int error = DB_open( db,
                             path,
                             mode,
                             hash_table_size,
                             key_size,
                             value_size );
 
        return error;
        }
   
    char *dbTempName = autoSprintf( "%s.temp", path );
    File dbTempFile( NULL, dbTempName );
   
    if( dbTempFile.exists() ) {
        dbTempFile.remove();
        }
   
    if( dbTempFile.exists() ) {
        AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );
 
        delete [] dbTempName;
 
        return DB_open( db,
                        path,
                        mode,
                        hash_table_size,
                        key_size,
                        value_size );
        }
   
    DB oldDB;
   
    int error = DB_open( &oldDB,
                         path,
                         mode,
                         hash_table_size,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_naturalTileShrunk",
                        path );
        delete [] dbTempName;
 
        return error;
        }
 
   
 
   
 
   
    DB_Iterator dbi;
   
   
    DB_Iterator_init( &oldDB, &dbi );
   
    // key and value size that are big enough to handle all of our DB
    unsigned char key[16];
   
    unsigned char value[12];
   
    int totalCount = 0;
    int keepCount = 0;
    int discardCount = 0;
   
    // first, just count
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        totalCount++;
       
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );
        
        int tile = dbGet_noCache( x, y, 0, 0 );
        
        if( tile == -1 ) {
            // tile not found in map.db, meaning it has been discarded
            // discard this record in this other db as well
            discardCount++;
            continue;
            }
        // otherwise, keep it
        keepCount++;
        }
 
 
 
    // optimial size for DB of remaining elements
    unsigned int newSize = DB_getShrinkSize( &oldDB, keepCount );
 
    AppLog::infoF( "Shrinking hash table in %s from %d down to %d",
                   path,
                   DB_getCurrentSize( &oldDB ),
                   newSize );
 
 
    DB tempDB;
   
    error = DB_open( &tempDB,
                     dbTempName,
                     mode,
                     newSize,
                     key_size,
                     value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        dbTempName );
        delete [] dbTempName;
        DB_close( &oldDB );
        return error;
        }
 
 
    // now that we have new temp db properly sized,
    // iterate again and insert, but don't count
    DB_Iterator_init( &oldDB, &dbi );
 
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );
        int tile = dbGet_noCache( x, y, 0, 0 );
        
        if( tile == -1 ) {
            // tile not found in map.db, meaning it has been discarded
            // discard this record in this other db as well
            continue;
            }
        // otherwise, keep it
        DB_put_new( &tempDB, key, value );
        }
 
 
   
    AppLog::infoF( "Cleaned %d / %d natural map cells from %s", discardCount, totalCount,
                   path );
 
    printf( "\n" );
   
   
    DB_close( &tempDB );
    DB_close( &oldDB );
 
    dbTempFile.copy( &dbFile );
    dbTempFile.remove();
 
    delete [] dbTempName;
 
    // now open new, shrunk file
    return DB_open( db,
                    path,
                    mode,
                    hash_table_size,
                    key_size,
                    value_size );
    }
    

// 0 - No shrinking, directly open the dbs
// 1 - Shrink dbs by lookTime, "stale" cells are cleared
// 2 - Shrink dbs by natural tiles, cells unchanged from it's naturally generated state are cleared
int dbShrinkMode = 0;

int DB_open_modeSwitch(
    DB *db,
    const char *path,
    int mode,
    unsigned long hash_table_size,
    unsigned long key_size,
    unsigned long value_size) {
    
    int error;
    
    if( dbShrinkMode == 0 ) {
        error = DB_open( db,
                         path,
                         mode,
                         hash_table_size,
                         key_size,
                         value_size );
        }
    else if( dbShrinkMode == 1 ) {
        error = DB_open_timeShrunk( db,
                                    path,
                                    mode,
                                    hash_table_size,
                                    key_size,
                                    value_size );
        }
    else if( dbShrinkMode == 2 ) {
        if( strcmp( path, "map.db" ) == 0 ) {
            error = DB_open_naturalTileShrunk_mapDb( db,
                                                     path,
                                                     mode,
                                                     hash_table_size,
                                                     key_size,
                                                     value_size );
            }
        else {
            error = DB_open_naturalTileShrunk( db,
                                               path,
                                               mode,
                                               hash_table_size,
                                               key_size,
                                               value_size );
            }
        }
    return error;
    }



extern char skipLookTimeCleanup;
extern int staleSec;

void loadDBShrinkSettings() {
    
    skipLookTimeCleanup = SettingsManager::getIntSetting( "skipLookTimeCleanup", 0 );
    staleSec = SettingsManager::getIntSetting( "mapCellForgottenSeconds", 0 );
    // If staleSec is 0 and skipLookTimeCleanup is 0
    // we will walk through the dbs without clearing anything
    // greatly lengthening the server startup time...
    if( staleSec == 0 ) skipLookTimeCleanup = 1;
    
    dbShrinkMode = SettingsManager::getIntSetting( "dbShrinkMode", 0 );
    if( dbShrinkMode != 1 ) skipLookTimeCleanup = 1;
    
    AppLog::errorF( "DB shrinking mode = %d", dbShrinkMode );
    
    }
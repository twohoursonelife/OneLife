#include "objectMetadata.h"
#include "objectBank.h"
#include "categoryBank.h"

#include "minorGems/util/SettingsManager.h"

// bottom 16 bits of map database item are object ID
// enough for 65535 objects
static int objectIDMask = 0x0000FFFF;

// top 15 bits are metadata ID,
// enough for 32767 metadata records
// the sign bit is not used
static int metadataIDShift = 16;

static int maxMetadataID = 32767;

// 0 used to represent non-existing metadata
static int nextMetadataID = 1;



int extractObjectID( int inMapID ) {
    if( inMapID <= 0 ) {
        return inMapID;
        }
    return inMapID & objectIDMask;
    }



// extracts top portion of map ID as a metadata ID 
int extractMetadataID( int inMapID ) {
    if( inMapID <= 0 ) {
        return 0;
        }
    return inMapID >> metadataIDShift;
    }



void setLastMetadataID( int inMetadataID ) {
    nextMetadataID = inMetadataID;
    // call once to increment and wrap around if needed
    getNewMetadataID();
    }



int getNewMetadataID() {
    nextMetadataID = SettingsManager::getIntSetting( "nextMetadataID", 0 );
    
    do {
        nextMetadataID++;
        if( nextMetadataID > maxMetadataID ) {
            // wrap around, reuse old IDs
            nextMetadataID = 1;
            }
        } while( nextMetadataID % 2 == 0 ); // skip multiples of 2 to protect legacy migrated metadata IDs

    SettingsManager::setSetting( "nextMetadataID", nextMetadataID );

    return nextMetadataID;
    }



int packMetadataID( int inObjectID, int inMetadataID ) {
    return ( inMetadataID << metadataIDShift ) | inObjectID;
    }




#define NUM_META_RECORDS 100
static int nextMetaRecord;
static TransRecord metaRecords[NUM_META_RECORDS];



TransRecord *getMetaTrans( int inActor, int inTarget, 
                           char inLastUseActor,
                           char inLastUseTarget ) {
    
    int actorMeta = extractMetadataID( inActor );
    int targetMeta = extractMetadataID( inTarget );

    if( actorMeta != 0 )
        inActor = extractObjectID( inActor );
    if( targetMeta != 0 )
        inTarget = extractObjectID( inTarget );
    


    TransRecord *r = getTrans( inActor, inTarget, 
                               inLastUseActor, inLastUseTarget );
    
    if( r == NULL ) {
        return r;
        }
    
    
    TransRecord *rStatic = &( metaRecords[ nextMetaRecord ] );
    
    nextMetaRecord++;
    if( nextMetaRecord >= NUM_META_RECORDS ) {
        nextMetaRecord = 0;
        }
    
    *rStatic = *r;
    

    if( isProbabilitySet( rStatic->newActor ) ) {
        rStatic->newActor = pickFromProbSet( rStatic->newActor );
        }
    if( isProbabilitySet( rStatic->newTarget ) ) {
        rStatic->newTarget = pickFromProbSet( rStatic->newTarget );
        }

    
    int passThroughMeta = actorMeta;
    
    if( actorMeta == 0 ) {
        passThroughMeta = targetMeta;
        }

    if( passThroughMeta != 0 ) {
        if( rStatic->newActor > 0 &&
            getObject( rStatic->newActor )->mayHaveMetadata ) {
            rStatic->newActor = packMetadataID( rStatic->newActor, 
                                                passThroughMeta );
            }
        else if( rStatic->newTarget > 0 &&
                 getObject( rStatic->newTarget )->mayHaveMetadata ) {
            rStatic->newTarget = packMetadataID( rStatic->newTarget, 
                                                 passThroughMeta );
            }
        }

    return rStatic;
    }

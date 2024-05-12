#include "cravings.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"


Craving noCraving = { -1, -1, 0 };

int eatEverythingModeEnabled = 0;




typedef struct CravingList {
        int lineageEveID;
        int lineageMaxFoodDepth;
        SimpleVector<Craving> cravedFoods;
    } CravingList;
    

static SimpleVector<CravingList> list;

static int nextUniqueID = 1;



// returns NULL if not found
static CravingList *getListForLineage( int inLineageEveID ) {
    for( int i=0; i<list.size(); i++ ) {
        CravingList *l = list.getElement( i );
        if( l->lineageEveID == inLineageEveID ) {
            return l;
            }
        }
    return NULL;
    }



static JenkinsRandomSource randSource;


static Craving getRandomFood( int inLineageMaxFoodDepth, 
                              Craving inFoodToAvoid = noCraving ) {

    SimpleVector<int> *allFoods = getAllPossibleFoodIDs();
    
    if( eatEverythingModeEnabled ) {
        // everything can be eaten
        allFoods = getAllPossibleNonPermanentIDs();
        }
    
    SimpleVector<int> possibleFoods;
    
    if( inLineageMaxFoodDepth > 0 ) {
        
        // The next craving is drawn with certain probability to "advance the tech"
        // with a limit on the size of the "tech jump"
        
        double harderCravingProb = SettingsManager::getDoubleSetting( "harderCravingProb", 0.6 );
        int maxCravingDepthGap = SettingsManager::getIntSetting( "maxCravingDepthGap", 10 );
        
        bool harderCravingOrNot = randSource.getRandomBoundedDouble( 0, 1 ) <= harderCravingProb;
        
        int maxDepth = 0;
        
        for( int i=0; i<allFoods->size(); i++ ) {
            int id = allFoods->getElementDirect( i );
            
            if( id != inFoodToAvoid.foodID ) {
                
                int d = getObjectDepth( id );
                if( d == UNREACHABLE ) continue;
                if( d > maxDepth ) maxDepth = d;
                    
                if( 
                    (!harderCravingOrNot && d <= inLineageMaxFoodDepth) ||
                    (harderCravingOrNot && d > inLineageMaxFoodDepth && d <= inLineageMaxFoodDepth + maxCravingDepthGap)
                ) {
                    possibleFoods.push_back( id );
                    }
                }
            }
        
        // We are at a point on the tech tree that 
        // the depth gap between our most advanced food and the next harder food
        // is greater than maxCravingDepthGap.
        // We ignore maxCravingDepthGap and pick from the next harder food
        if( harderCravingOrNot && possibleFoods.size() == 0 ) {
            // printf( "Tech Tree Gap ...\n" );
            int nextDepth = inLineageMaxFoodDepth;
            // have like at least 3 food in the pool to avoid repeating between chains
            while( possibleFoods.size() < 3 ) {
                nextDepth = nextDepth + 1;
                for( int i=0; i<allFoods->size(); i++ ) {
                    int id = allFoods->getElementDirect( i );
                    
                    if( id != inFoodToAvoid.foodID ) {
                        
                        int d = getObjectDepth( id );
                        if( d == UNREACHABLE ) continue;

                        if( d == nextDepth ||
                            nextDepth > maxDepth // We reached the top of tech tree
                        ) {
                            possibleFoods.push_back( id );
                            }
                        }
                    }
                }
            }

        }
    else {
        // new lineage, crave some wild food
        for( int i=0; i<allFoods->size(); i++ ) {
            int id = allFoods->getElementDirect( i );
            
            if( id != inFoodToAvoid.foodID ) {
                
                int d = getObjectDepth( id );

                if( d <= 1 ) {
                    possibleFoods.push_back( id );
                    }
                }
            }
        }
    
    if( possibleFoods.size() > 0 ) {
        int pick = 
            randSource.getRandomBoundedInt( 0, possibleFoods.size() - 1 );
            
        int pickedFood = possibleFoods.getElementDirect( pick );
        
        double cravingBonusScale = SettingsManager::getDoubleSetting( "cravingBonusScale", 0.4 );
        
        int bonus = ( getObjectDepth( pickedFood ) - inLineageMaxFoodDepth ) * cravingBonusScale;
        
        if( bonus < 1 ) bonus = 1;
    
        // ObjectRecord *o = getObject( pickedFood );
        // printf( "harderCravingOrNot = %d, %d possible foods, picking #%d, max %d, depth %d, gap %d, scale %.2f, bonus %d, %s\n",
                // harderCravingOrNot,
                // possibleFoods.size(),
                // pick,
                // inLineageMaxFoodDepth,
                // getObjectDepth( pickedFood ),
                // getObjectDepth( pickedFood ) - inLineageMaxFoodDepth,
                // cravingBonusScale,
                // bonus,
                // o->description
                // );
        
        Craving c = { pickedFood,
                      nextUniqueID,
                      bonus
                      };
        nextUniqueID ++;
        
        return c;
        }
    else {
        // no possible food - this case should not happen

        // return first food in main list
        if( allFoods->size() > 0 ) {
            int pickedFood = allFoods->getElementDirect( 0 );
            Craving c = { pickedFood,
                          nextUniqueID,
                          getObjectDepth( pickedFood )
                          };
            nextUniqueID ++;
        
            return c;
            }
        else {
            return noCraving;
            }
        }
    }




Craving getCravedFood( int inLineageEveID, int inPlayerGenerationNumber,
                       Craving inLastCraved ) {

    CravingList *l = getListForLineage( inLineageEveID );
   
    if( l == NULL ) {
        // push a new empty list
        CravingList newL;
        newL.lineageEveID = inLineageEveID;
        newL.lineageMaxFoodDepth = 0;
        list.push_back( newL );
        
        l = getListForLineage( inLineageEveID );
        }
    
    int listSize = l->cravedFoods.size();
    for( int i=0; i < listSize; i++ ) {
        Craving food = l->cravedFoods.getElementDirect( i );
        
        if( food.foodID == inLastCraved.foodID && 
            food.uniqueID == inLastCraved.uniqueID &&
            i < listSize - 1 ) {
            return l->cravedFoods.getElementDirect( i + 1 );
            }
        }
    
    // got here, we went off end of list without finding our last food
    // add a new one and return that.

    // avoid repeating last food we craved
    Craving newFood = getRandomFood( l->lineageMaxFoodDepth, inLastCraved );
    
    l->cravedFoods.push_back( newFood );
    
    return newFood;
    }


void logFoodDepth( int inLineageEveID, int inEatenID ) {
    
    // the food with max depth ever eaten is
    // the way we gauge how advanced the tech is for a lineage
    
    int d = getObjectDepth( inEatenID );
    
    // do not include uncraftable food in determining cravings
    if( d == UNREACHABLE ) return;

    CravingList *l = getListForLineage( inLineageEveID );
   
    if( l == NULL ) {
        // push a new empty list
        CravingList newL;
        newL.lineageEveID = inLineageEveID;
        newL.lineageMaxFoodDepth = 0;
        list.push_back( newL );
        
        l = getListForLineage( inLineageEveID );
        }

    if( d > l->lineageMaxFoodDepth ) {
        l->lineageMaxFoodDepth = d;
        }

    }


void purgeStaleCravings( int inLowestUniqueID ) {
    
    int numPurged = 0;
    
    for( int i=0; i<list.size(); i++ ) {
        CravingList *l = list.getElement( i );        

        // walk backwards and find first id that is below inLowestUniqueID
        // then trim all records that come before that
        int start = l->cravedFoods.size() - 1;
        
        for( int f=start; f >= 0; f-- ) {
            
            if( l->cravedFoods.getElementDirect( f ).uniqueID < 
                inLowestUniqueID ) {
                
                l->cravedFoods.deleteStartElements( f + 1 );
                
                numPurged += f + 1;
                break;
                }
            }
        }
    }




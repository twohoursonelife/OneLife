#include "groundSprites.h"
#include "LivingLifePage.h"
#include "objectBank.h"
#include "minitech.h"
#include "newbieTips.h"

#include "minorGems/util/SettingsManager.h"


extern double viewWidth;
extern double viewHeight;
extern float gui_fov_scale_hud;
extern doublePair lastScreenViewCenter;

LivingLifePage *newbieTips::livingLifePage;
SimpleVector<LiveObject> *newbieTips::players;
int newbieTips::mMapD;
int newbieTips::pathFindingD;

bool newbieTips::newbieTipsEnabled = false;

bool newbieTips::yumSlipShowing;
doublePair newbieTips::yumBubblePos = {9999, 9999};
int newbieTips::hungerSlipShowing;

bool newbieTips::drawTipsArrow = false;
doublePair newbieTips::rawTipsArrowPos;
bool newbieTips::screenOrTile = false;
bool newbieTips::shouldDisplayMessage = false;
const char *newbieTips::messageToDisplay;

bool newbieTips::sessionStarted = false;
double newbieTips::sessionStartTime = 0;
int newbieTips::currentTipsIndex = 0;

bool newbieTips::kidLessonDone = false;
SimpleVector<int> newbieTips::kids;
SimpleVector<int> newbieTips::kidsEverHeld;
int newbieTips::lastKidID = 0;
int newbieTips::lastNeverHeldKidsDeathCount = 0;
double newbieTips::lastSeedLessonTime = 0;
double newbieTips::lastLootLessonTime = 0;
bool newbieTips::justTriedToKill = false;
int newbieTips::justUsedOnObjectID = 0;


void newbieTips::init( 
    LivingLifePage *inLivingLifePage,
    SimpleVector<LiveObject> *inGameObjects, 
    int inmMapD, 
    int inPathFindingD
    ) {
    livingLifePage = inLivingLifePage;
    players = inGameObjects;
    mMapD = inmMapD;
    pathFindingD = inPathFindingD;
    
    newbieTipsEnabled = SettingsManager::getIntSetting( "newbieTipsEnabled", 0 );
    
}

void newbieTips::startTipsArrow( doublePair pos, bool inScreenOrTile ) {
    drawTipsArrow = true;
    rawTipsArrowPos = pos;
    screenOrTile = inScreenOrTile;
}

void newbieTips::stopTipsArrow() {
    drawTipsArrow = false;
}

float newbieTips::arrowScale() {
    if( screenOrTile == false ) return minitech::guiScale;
    return 1.0;
}

// {-259, 325.5} // first row of recipe
// {-145, 421} // make/use toggles
// {-145, 62} // max button

doublePair newbieTips::calcTipsArrowPos() {
    doublePair targetPos = {9999, 9999};
    if( screenOrTile == false ) {
        targetPos = lastScreenViewCenter;
        targetPos.y += -viewHeight/2;
        targetPos.y += rawTipsArrowPos.y * minitech::guiScale;
        // move up so that when bouncing arrow reaches the lowerest point
        // the tip touches the target position
        targetPos.y += 38 * minitech::guiScale;
        targetPos.x += viewWidth/2;
        targetPos.x += rawTipsArrowPos.x * minitech::guiScale;
    } else if( screenOrTile == true ) {
        targetPos.x = rawTipsArrowPos.x * CELL_D;
        targetPos.y = rawTipsArrowPos.y * CELL_D;
        // round to closest cell pos
        targetPos.x = CELL_D * lrint( targetPos.x / CELL_D );
        targetPos.y = CELL_D * lrint( targetPos.y / CELL_D );
        // move up
        targetPos.y += 64;
    }
    return targetPos;
}

doublePair newbieTips::conversionFromMinitechPos( doublePair pos ) {
    pos.x -= lastScreenViewCenter.x;
    pos.y -= lastScreenViewCenter.y;
    pos.x -= viewWidth/2;
    pos.y -= -viewHeight/2;
    pos.x = pos.x / minitech::guiScale;
    pos.y = pos.y / minitech::guiScale;
    return pos;
}

bool newbieTips::tryToStartSession( int tipsIndex ) {
    if( !sessionStarted && (game_getCurrentTime() - sessionStartTime > 15) ) {
        sessionStarted = true;
        sessionStartTime = game_getCurrentTime();
        currentTipsIndex = tipsIndex;
        return true;
    }
    return false;
}

bool newbieTips::tryToEndSession( int tipsIndex ) {
    if( sessionStarted && tipsIndex == currentTipsIndex && (game_getCurrentTime() - sessionStartTime > 5) ) {
        sessionStarted = false;
        currentTipsIndex = 0;
        return true;
    }
    return false;
}

bool newbieTips::isInSession( int tipsIndex ) {
    return sessionStarted && currentTipsIndex == tipsIndex;
}


void newbieTips::livingLifeStep(
    int mTutorialNumber,
    int mLiveTutorialTriggerNumber
) {
    
    stopTipsArrow();
    
    if( mTutorialNumber == 1 ) {
        if( mLiveTutorialTriggerNumber == 2 ) { // Berry bush
            startTipsArrow( {14, 0}, true );
        } else if( mLiveTutorialTriggerNumber == 3 ) { // Food meter
            startTipsArrow( {-viewWidth / minitech::guiScale + 70, 40}, false );
        } else if( mLiveTutorialTriggerNumber == 5001 ) { // Crafting guide - make/use mode
            if( !minitech::minitechMinimized ) {
                doublePair pos = minitech::makeUseTogglePos;
                pos = conversionFromMinitechPos( pos );
                startTipsArrow( pos, false );
            } else {
                doublePair pos = minitech::maxButtonPos;
                pos = conversionFromMinitechPos( pos );
                startTipsArrow( pos, false );
            }
        } else if( mLiveTutorialTriggerNumber == 7 ) { // Crafting guide - how to make sharp stone
            if( !minitech::minitechMinimized ) {
                doublePair pos = minitech::sharpyRecipePos;
                if( minitech::useOrMake == 0 && !(pos.x == 9999 && pos.y == 9999) ) {
                    pos = conversionFromMinitechPos( pos );
                    startTipsArrow( pos, false );
                }
            } else {
                doublePair pos = minitech::maxButtonPos;
                pos = conversionFromMinitechPos( pos );
                startTipsArrow( pos, false );
            }
        } else if( mLiveTutorialTriggerNumber == 10 || mLiveTutorialTriggerNumber == 1010 ) { // Yum slip
            if( yumSlipShowing ) {
                LiveObject *ourLiveObject = livingLifePage->getOurLiveObject();
                if( ourLiveObject == NULL ) return;
                doublePair pos = yumBubblePos;
                pos.y += 16;
                startTipsArrow( conversionFromMinitechPos(pos), false );
            }
        } else if( mLiveTutorialTriggerNumber == 11 ) { // Tule reeds
            if( getObjId(118, 1) == 121 ) { //Tule Reeds
                startTipsArrow( {118, 1}, true );
            } else if( getObjId(120, 1) == 121 ) { //Tule Reeds
                startTipsArrow( {120, 1}, true );
            }
        } else if( mLiveTutorialTriggerNumber == 1101 ) { // Vertical door
            startTipsArrow( {125, 0}, true );
        } else if( mLiveTutorialTriggerNumber == 13 ) { // Temp meter
            startTipsArrow( {-145, 25}, false );
        } else if( mLiveTutorialTriggerNumber == 15 || mLiveTutorialTriggerNumber == 1501 ) { // Box of clothes
            startTipsArrow( {162, 0}, true );
            
        } else if( mLiveTutorialTriggerNumber == 1901 ) { // Crafting guide - search
            if( !minitech::minitechMinimized ) {
                doublePair pos = minitech::topBarPos;
                if( minitech::showBar && !(pos.x == 9999 && pos.y == 9999) ) {
                    pos = conversionFromMinitechPos( pos );
                    startTipsArrow( pos, false );
                }
            } else {
                doublePair pos = minitech::maxButtonPos;
                pos = conversionFromMinitechPos( pos );
                startTipsArrow( pos, false );
            }
            
        } else if( mLiveTutorialTriggerNumber == 1902 ) { // Crafting guide - how to make hatchet
            if( !minitech::minitechMinimized ) {
                doublePair pos = {9999, 9999};
                if( minitech::useOrMake == 0 ) {
                    pos = minitech::makeUseTogglePos;
                } else {
                    pos = minitech::hatchetRecipePos;
                }
                if( !(pos.x == 9999 && pos.y == 9999) ) {
                    pos = conversionFromMinitechPos( pos );
                    startTipsArrow( pos, false );
                }
            } else {
                doublePair pos = minitech::maxButtonPos;
                pos = conversionFromMinitechPos( pos );
                startTipsArrow( pos, false );
            }
        }
    } else if( mTutorialNumber == 0 && newbieTipsEnabled ) {
        
        LiveObject *ourLiveObject = livingLifePage->getOurLiveObject();
        if( ourLiveObject == NULL ) return;
        bool amFemale = !getObject(ourLiveObject->displayID)->male;
        if( amFemale ) {
            if( ourLiveObject->holdingID < 0 ) {
                int babyID = -ourLiveObject->holdingID;
                bool found = false;
                for( int i=0; i<kidsEverHeld.size(); i++ ) {
                    if( babyID == kidsEverHeld.getElementDirect( i ) ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) kidsEverHeld.push_back( babyID );
            }
        }
        
        // Tips 1 - Food
        if( ourLiveObject->age > 3 && hungerSlipShowing >= 1 ) {
            if( tryToStartSession( 1 ) ) {
                shouldDisplayMessage = true;
                messageToDisplay = "YOU ARE GETTING HUNGRY, WALK AROUND AND LOOK FOR FOOD !##REMEMBER TO CHECK FOR YUM IF YOU CAN AFFORD TO.";
            }
            if( isInSession( 1 ) ) {
                doublePair nearestFood = getClosestFood();
                if( !( nearestFood.x == 9999 && nearestFood.y == 9999 ) ) {
                    startTipsArrow( nearestFood, true );
                }
            }
        } else {
            tryToEndSession( 1 );
        }
        
        // Tips 2 - Stay with mom
        if( ourLiveObject->age < 3 && ourLiveObject->heldByAdultID == -1 ) {
            LiveObject *mother = newbieTips::getMother();
            
            if( mother != NULL ) {
                doublePair currentPos = ourLiveObject->currentPos;
                doublePair motherPos = mother->currentPos;
                float dist = sqrt(pow(motherPos.y - currentPos.y, 2) + pow(motherPos.x - currentPos.x, 2));
                
                if( dist > 3.5 ) {
                    if( tryToStartSession( 2 ) ) {
                        shouldDisplayMessage = true;
                        messageToDisplay = "STAY CLOSE TO MOM, SAY 'F' WHEN YOU'RE HUNGRY.##IF YOU WANT TO BE BORN ELSEWHERE, USE THE   /DIE   COMMAND.";
                    }
                } else {
                    tryToEndSession( 2 );
                }
                if( isInSession( 2 ) ) {
                    startTipsArrow( mother->currentPos, true );
                }
            }
        } else {
            tryToEndSession( 2 );
        }
        
        // Tips 3 - Feed and name babies
        if( amFemale && !kidLessonDone && haveKids() && tryToStartSession( 3 ) ) {
            shouldDisplayMessage = true;
            messageToDisplay = "YOU HAVE A BABY! CLICK TO HOLD AND FEED THEM.##WHEN YOU ARE HOLDING THEM, SAY   YOU ARE [NAME]    TO NAME THEM.";
            kidLessonDone = true;
        } else {
            tryToEndSession( 3 );
        }
        
        // Tips 4 - Don't ditch kids
        if( amFemale ) {
            int count = getNeverHeldKidsDeathCount();
            if( count > lastNeverHeldKidsDeathCount && count > 1 ) {
                if( tryToStartSession( 4 ) ) {
                    shouldDisplayMessage = true;
                    messageToDisplay = "PLEASE DO NOT ABANDON YOUR BABIES.##IF YOU DON'T WANT BABIES, SAY   NO BB   TO BE INFERTILE.";
                    lastNeverHeldKidsDeathCount = count;
                }
            } else {
                tryToEndSession( 4 );
            }
        }
        
        // Tips 5 - No griefing
        if( ourLiveObject->holdingID > 0 ) {
            int heldObjectParent = getObjectParent( ourLiveObject->holdingID );
            bool holdingWeapon = 
                heldObjectParent == 560 || //Knife
                heldObjectParent == 11671 || //Bronze Knife
                heldObjectParent == 8709 || //Flint Knife
                heldObjectParent == 3047 || //Sharp War Sword
                heldObjectParent == 6740 || //Blunt War Sword
                heldObjectParent == 152 //Bow and Arrow
                ;
            
            bool repeatedlyKillingFarmAnimal = false;
            if( ( heldObjectParent == 750 || //Knife #just-used
                heldObjectParent == 11888 ) //Bronze Knife #just-used
                && justUsedOnObjectID > 0 ) { 
                int targetParent = getObjectParent( justUsedOnObjectID );
                repeatedlyKillingFarmAnimal = 
                    targetParent == 4757 || // Hen
                    targetParent == 4754 || // Hen #just fed
                    targetParent == 4758 || // Rooster
                    targetParent == 4765 || // Hen #pregnant fertilized
                    targetParent == 575 || // Domestic Sheep
                    targetParent == 1325 || // Domestic Pig
                    targetParent == 576 // Shorn Domestic Sheep
                    ;
            }
            
            if( (repeatedlyKillingFarmAnimal ||
                (justTriedToKill && holdingWeapon) ) &&
                tryToStartSession( 5 ) ) {
                shouldDisplayMessage = true;
                messageToDisplay = "PLEASE BE REMINDED THAT THE SERVER IS MODERATED.##GRIEFING MAY GET YOU BANNED. PLEASE PLAY NICE.";
            } else {
                tryToEndSession( 5 );
            }
        } else {
            tryToEndSession( 5 );
        }
        justTriedToKill = false;
        justUsedOnObjectID = -1;
        
        // Tips 6 - seed
        doublePair currentPos = ourLiveObject->currentPos;
        float dist = sqrt(pow(currentPos.y, 2) + pow(currentPos.x, 2));
        if( game_getCurrentTime() - lastSeedLessonTime > 120 && dist > 500 && tryToStartSession( 6 ) ) {
            shouldDisplayMessage = true;
            messageToDisplay = "IF YOU WANT TO START A CAMP IN THE WILD,##YOU CAN ENTER YOUR OWN  'SPAWN CODE'  IN THE LOGIN PAGE.";
            lastSeedLessonTime = game_getCurrentTime();
        } else {
            tryToEndSession( 6 );
        }
        
        // Tips 7 - No looting
        if( game_getCurrentTime() - lastLootLessonTime > 120 && dist > 500 && tryToStartSession( 7 ) ) {
            shouldDisplayMessage = true;
            messageToDisplay = "IF YOU FIND SETTLEMENTS SEEMINGLY ABANDONED##PLEASE DO NOT TAKE ANYTHING, OTHERS MAY RETURN TO PLAY LATER.";
            lastLootLessonTime = game_getCurrentTime();
        } else {
            tryToEndSession( 7 );
        }
        
        if( ourLiveObject->age > 104 ) {
            SettingsManager::setSetting( "newbieTipsEnabled", 0 );
        }
        newbieTipsEnabled = SettingsManager::getIntSetting( "newbieTipsEnabled", 0 );
        
    }
}


doublePair newbieTips::getClosestFood() {
    
    LiveObject *ourLiveObject = livingLifePage->getOurLiveObject();
    doublePair currentPos = ourLiveObject->currentPos;
    
    int *mMap = livingLifePage->mMap;
    int mMapOffsetX = livingLifePage->mMapOffsetX;
    int mMapOffsetY = livingLifePage->mMapOffsetY;
    int pathOffsetX = pathFindingD/2 - currentPos.x;
    int pathOffsetY = pathFindingD/2 - currentPos.y;
    
    float bestDist = 9999.0;
    doublePair foundPos = {9999, 9999};
    
    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 
                
                int posX = mapX + mMapOffsetX - mMapD / 2;
                int posY = mapY + mMapOffsetY - mMapD / 2;
                
                float dist = sqrt(pow(posY - currentPos.y, 2) + pow(posX - currentPos.x, 2));
                if (dist >= bestDist) continue;

                int mapI = mapY * mMapD + mapX;
                int id = mMap[mapI];
                
                if ( isEasyFood( id ) ) {
                    foundPos = {(double)posX, (double)posY};
                    bestDist = dist;
                    continue;
                }
                
                
            }
        }
    }
    
    return foundPos;
}

int newbieTips::getObjId( int tileX, int tileY ) {
    int *mMap = livingLifePage->mMap;
    int mMapOffsetX = livingLifePage->mMapOffsetX;
    int mMapOffsetY = livingLifePage->mMapOffsetY;
    int mapX = tileX - mMapOffsetX + mMapD / 2;
    int mapY = tileY - mMapOffsetY + mMapD / 2;
    int i = mapY * mMapD + mapX;
    if (i < 0 || i >= mMapD*mMapD) return -1;
    return mMap[i];
}


bool newbieTips::isEasyFood( int id ) {
    if (id <= 0 || id >= getMaxObjectID() + 1) return false;
    ObjectRecord* o = getObject(id, true);
    if( o == NULL ) return false;
    if( o->foodValue > 0 || o->bonusValue > 0 ) return true;
    TransRecord* pick_trans = getTrans( 0, id );
    if (pick_trans == NULL) return false;
    int food_id = pick_trans->newActor;
    if (food_id == 837) return false; //We dont eat mushroom...
    ObjectRecord* food = getObject(food_id, true);
    if (food == NULL) return false;
    if (food->foodValue > 0 || food->bonusValue > 0) return true;
    return false;
}

LiveObject *newbieTips::getMother() {
    LiveObject *me = livingLifePage->getOurLiveObject();
    if( me == NULL ) return NULL;
    SimpleVector<int> ourLin = me->lineage;
    if( ourLin.size() == 0 ) return NULL;
    for( int i=0; i<players->size(); i++ ) {
        LiveObject *player = players->getElement( i );
        if( player == NULL ) continue;
        if( player->id == ourLin.getElementDirect( 0 ) ) return player;
    }
    return NULL;
}

bool newbieTips::haveKids() {
    LiveObject *me = livingLifePage->getOurLiveObject();
    if( me == NULL ) return 0;
    for( int i=0; i<players->size(); i++ ) {
        LiveObject *player = players->getElement( i );
        if( player == NULL ) continue;
        if( player->lineage.getElementDirect( 0 ) == me->id ) return true;
    }
    return false;
}
        
int newbieTips::getNeverHeldKidsDeathCount() {
    LiveObject *me = livingLifePage->getOurLiveObject();
    if( me == NULL ) return 0;
    
    for( int i=0; i<players->size(); i++ ) {
        LiveObject *player = players->getElement( i );
        if( player == NULL ) continue;
        if( player->id > lastKidID && player->lineage.getElementDirect( 0 ) == me->id ) {
            kids.push_back( player->id );
            lastKidID = player->id;
        }
    }
    
    int count = 0;
    for( int i=0; i<kids.size(); i++ ) {
        int kidID = kids.getElementDirect( i );
        bool alive = false;
        for( int j=0; j<players->size(); j++ ) {
            LiveObject *player = players->getElement( j );
            if( player->id == kidID ) {
                alive = true;
                break;
            }
        }
        if( alive ) continue;
        bool found = false;
        for( int i=0; i<kidsEverHeld.size(); i++ ) {
            if( kidID == kidsEverHeld.getElementDirect( i ) ) {
                found = true;
                break;
            }
        }
        if( !found ) count++;
    }
    
    return count;
}


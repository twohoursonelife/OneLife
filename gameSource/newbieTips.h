#ifndef newbieTips_H
#define newbieTips_H

#include "LivingLifePage.h"


class newbieTips
{

public:

    static bool newbieTipsEnabled;
    
    static bool yumSlipShowing;
    static int hungerSlipShowing;

    static bool drawTipsArrow;
    static doublePair rawTipsArrowPos;
    static bool screenOrTile;
    static bool shouldDisplayMessage;
    static const char *messageToDisplay;
    static void startTipsArrow( doublePair pos, bool inScreenOrTile );
    static void stopTipsArrow();
    static float arrowScale();
    static doublePair calcTipsArrowPos();
    static doublePair converationFromMinitechPos( doublePair pos );
    
    static bool sessionStarted;
    static double sessionStartTime;
    static int currentTipsIndex;
    static bool tryToStartSession( int tipsIndex );
    static bool tryToEndSession( int tipsIndex );
    static bool isInSession( int tipsIndex );
    
    static LivingLifePage *livingLifePage;
    static SimpleVector<LiveObject> *players;
	static int mMapD;
	static int pathFindingD;
    static void init(
        LivingLifePage *inLivingLifePage,
        SimpleVector<LiveObject> *inGameObjects, 
        int inmMapD, 
        int inPathFindingD
    );
    
    static void livingLifeStep(
        int mTutorialNumber,
        int mLiveTutorialTriggerNumber
    );
    
    static doublePair getClosestFood();
    static bool isEasyFood( int id );
    static LiveObject *getMother();
    static bool haveKids();
    static int getNeverHeldKidsDeathCount();
    
    static bool kidLessonDone;
    static SimpleVector<int> kids;
    static SimpleVector<int> kidsEverHeld;
    static int lastKidID;
    static int lastNeverHeldKidsDeathCount;
    static bool seedLessonDone;
    static bool justTriedToKill;
    static int justUsedOnObjectID;
	
};


#endif

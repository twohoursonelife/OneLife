int versionNumber = 256;
int dataVersionNumber = 0;

int binVersionNumber = versionNumber;


// NOTE that OneLife doesn't use account hmacs

// retain an older version number here if server is compatible
// with older client versions.
// Change this number (and number on server) if server has changed
// in a way that breaks old clients.
int accountHmacVersionNumber = 0;



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif


#include "minorGems/graphics/Color.h"





#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/io/file/File.h"

#include "minorGems/system/Time.h"

#include "minorGems/crypto/hashes/sha1.h"


// static seed
CustomRandomSource randSource( 34957197 );



#include "minorGems/util/log/AppLog.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"

#include "minorGems/graphics/RGBAImage.h"





#include "spriteBank.h"
#include "objectBank.h"
#include "categoryBank.h"
#include "transitionBank.h"
#include "soundBank.h"

#include "liveObjectSet.h"

#include "groundSprites.h"

#include "emotion.h"
#include "photos.h"
#include "lifeTokens.h"
#include "fitnessScore.h"


#include "FinalMessagePage.h"
#include "LoadingPage.h"
#include "AutoUpdatePage.h"
#include "LivingLifePage.h"
#include "ExistingAccountPage.h"
#include "ExtendedMessagePage.h"
#include "RebirthChoicePage.h"
#include "SettingsPage.h"
#include "ReviewPage.h"
#include "TwinPage.h"
#include "PollPage.h"
#include "GeneticHistoryPage.h"
//#include "TestPage.h"

#include "ServerActionPage.h"

#include "ageControl.h"

#include "musicPlayer.h"

#include "whiteSprites.h"

#include "message.h"

#ifdef USE_DISCORD
#include "DiscordController.h"
#endif // USE_DISCORD

// should we pull the map
static char mapPullMode = 0;
static char autoLogIn = 0;


char loginEditOverride = false;


// start at reflector URL
char *reflectorURL = NULL;

char usingCustomServer = false;
char *serverIP = NULL;
int serverPort = 0;


char useTargetFamily;
char useSpawnSeed;

char *userEmail = NULL;
char *accountKey = NULL;
char *userTwinCode = NULL;
int userTwinCount = 0;
char userReconnect = false;

char showingInGameSettings = false;


// these are needed by ServerActionPage, but we don't use them
int userID = -1;
int serverSequenceNumber = 0;


FinalMessagePage *finalMessagePage;

ServerActionPage *getServerAddressPage;

LoadingPage *loadingPage;
AutoUpdatePage *autoUpdatePage;
LivingLifePage *livingLifePage;
ExistingAccountPage *existingAccountPage;
ExtendedMessagePage *extendedMessagePage;
RebirthChoicePage *rebirthChoicePage;
SettingsPage *settingsPage;
ReviewPage *reviewPage;
TwinPage *twinPage;
PollPage *pollPage;
GeneticHistoryPage *geneticHistoryPage;

#ifdef USE_DISCORD
DiscordController *discordController;
#endif // USE_DISCORD

//TestPage *testPage = NULL;


GamePage *currentGamePage = NULL;

int loadingPhase = 0;

int loadingStepBatchSize = 1;
double loadingPhaseStartTime;

int numLoadingSteps = 20;



SpriteHandle instructionsSprite;



// position of view in world
doublePair lastScreenViewCenter = {0, 0 };



// world width of one view
//FOV
int gui_hud_mode = 0;
float gui_fov_scale = 1.0f;
float gui_fov_scale_hud = 1.0f;
float gui_fov_target_scale_hud = 1.0f;
float gui_fov_preferred_max_scale = 3.0f;
int gui_fov_offset_x = (int)(((1280 * gui_fov_target_scale_hud) - 1280)/2);
int gui_fov_offset_y = (int)(((720 * gui_fov_target_scale_hud) - 720)/2);

float gui_fov_scale_before_settings = 1.0f;


double viewWidth = 1280;
double viewHeight = 720;


// this is the desired visible width
// if our screen is wider than this (wider than 16:9 aspect ratio)
// then we will put letterbox bars on the sides
// Usually, if screen is not 16:9, it will be taller, not wider,
// and we will put letterbox bars on the top and bottom 
double visibleViewWidth = viewWidth;



void loadFovSettings() {

    gui_hud_mode = SettingsManager::getIntSetting( "hudDrawMode", 0 );
    if( gui_hud_mode < 0 ) {
        gui_hud_mode = 0;
        SettingsManager::setSetting( "hudDrawMode", gui_hud_mode );
        }
    else if( gui_hud_mode > 2 ) {
        gui_hud_mode = 2;
        SettingsManager::setSetting( "hudDrawMode", gui_hud_mode );
        }
    
    gui_fov_target_scale_hud = SettingsManager::getFloatSetting( "fovScaleHUD", 1.0f );
    if( gui_fov_target_scale_hud < 1.0f ) {
        gui_fov_target_scale_hud = 1.0f;
        SettingsManager::setSetting( "fovScaleHUD", gui_fov_target_scale_hud );
        }
    else if( gui_fov_target_scale_hud > 1.75f ) {
        gui_fov_target_scale_hud = 1.75f;
        SettingsManager::setSetting( "fovScaleHUD", gui_fov_target_scale_hud );
        }

    gui_fov_scale = 1.0f;

    gui_fov_scale_hud = gui_fov_scale / gui_fov_target_scale_hud;
    gui_fov_offset_x = (int)(((1280 * gui_fov_target_scale_hud) - 1280)/2);
    gui_fov_offset_y = (int)(((720 * gui_fov_target_scale_hud) - 720)/2);
    viewWidth = 1280 * gui_fov_scale;
    viewHeight = 720 * gui_fov_scale;
    visibleViewWidth = viewWidth;
    
}


// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;

int screenW, screenH;

char initDone = false;

float mouseSpeed;


int maxSimultaneousExpectedSoundEffects = 10;

// fraction of full volume devoted to music
// Note that musicLoudness and soundEffectLoudness settings still
// effect absolute loudness of each, beyond this setting
// this setting is used to trim music volume relative to sound effects
// if both are at full volume

// 1.0 makes it as loud as the sound effect mix
// on the other hand, it's stereo, compressed, full-frequency etc.
// so it's subjectively louder
double musicHeadroom = 1.0;




int musicOff = 0;
float musicLoudness;

int webRetrySeconds;


double frameRateFactor = 1;
int baseFramesPerSecond = 60;
int targetFramesPerSecond = baseFramesPerSecond;

char firstDrawFrameCalled = false;
int firstServerMessagesReceived = 0;


char upKey = 'w';
char leftKey = 'a';
char downKey = 's';
char rightKey = 'd';







char doesOverrideGameImageSize() {
    return true;
    }



void getGameImageSize( int *outWidth, int *outHeight ) {
    *outWidth = (int)viewWidth;
    *outHeight = (int)viewHeight;
    }


char shouldNativeScreenResolutionBeUsed() {
    return true;
    }


char isNonIntegerScalingAllowed() {
    return true;
    }


const char *getWindowTitle() {
    return "OneLife";
    }


const char *getAppName() {
    return "OneLife";
    }

int getAppVersion() {
    return versionNumber;
    }

const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "OneLifeApp";
    }



const char *getFontTGAFileName() {
    return "font_32_64.tga";
    }
    
bool newFontExist() {
    bool exist = false;
    Image *spriteImage = readTGAFile( "newfont_32_64.tga" );
    if( spriteImage != NULL ) exist = true;
    delete spriteImage;
    return exist;
    }
    
const char *getNewFontTGAFileName() {
    return "newfont_32_64.tga";
    }


char isDemoMode() {
    return false;
    }


const char *getDemoCodeSharedSecret() {
    return "fundamental_right";
    }


const char *getDemoCodeServerURL() {
    return "http://FIXME/demoServer/server.php";
    }



char gamePlayingBack = false;


Font *mainFont;
Font *oldMainFont;
Font *mainFontFixed;
// closer spacing
Font *mainFontReview;
Font *numbersFontFixed;
Font *handwritingFont;
Font *tinyHandwritingFont;
Font *pencilFont;
Font *pencilErasedFont;

Font *smallFont;

Font *titleFont;

SpriteHandle sheetSprites[9] = {nullptr};


char *shutdownMessage = NULL;








static float pauseScreenFade = 0;

static char *currentUserTypedMessage = NULL;



// for delete key repeat during message typing
static int holdDeleteKeySteps = -1;
static int stepsBetweenDeleteRepeat;



static void updateDataVersionNumber() {
    File file( NULL, "dataVersionNumber.txt" );
    
    if( file.exists() ) {
        char *contents = file.readFileContents();
        
        if( contents != NULL ) {
            sscanf( contents, "%d", &dataVersionNumber );
        
            delete [] contents;

            if( dataVersionNumber > versionNumber ) {
                versionNumber = dataVersionNumber;
                }
            }
        }
    }




#define SETTINGS_HASH_SALT "another_loss"


static const char *customDataFormatWriteString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";

static const char *customDataFormatReadString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";


char *getCustomRecordedGameData() {    
    
    updateDataVersionNumber();

    float mouseSpeedSetting = 
        SettingsManager::getFloatSetting( "mouseSpeed", 1.0f );
    int musicOffSetting = 
        SettingsManager::getIntSetting( "musicOff", 0 );
    float musicLoudnessSetting = 
        SettingsManager::getFloatSetting( "musicLoudness", 1.0f );
    
    int webRetrySecondsSetting = 
        SettingsManager::getIntSetting( "webRetrySeconds", 10 );
    

    char * result = autoSprintf(
        customDataFormatWriteString,
        versionNumber, mouseSpeedSetting, musicOffSetting, 
        musicLoudnessSetting,
        webRetrySecondsSetting );
    

    return result;
    }



char showMouseDuringPlayback() {
    // since we rely on the system mouse pointer during the game (and don't
    // draw our own pointer), we need to see the recorded pointer position
    // to make sense of game playback
    return true;
    }



char *getHashSalt() {
    return stringDuplicate( SETTINGS_HASH_SALT );
    }




void initDrawString( int inWidth, int inHeight ) {

    loadFovSettings();

    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    
    mainFont = new Font( getNewFontTGAFileName(), 3, 4, false, 16 );
    // mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );
    oldMainFont = new Font( getFontTGAFileName(), 6, 6, false, 16 );
    oldMainFont->setMinimumPositionPrecision( 1 );
    
    if( newFontExist() ) {
        mainFont = new Font( getNewFontTGAFileName(), 3, 4, false, 16 );
        }
    else {
        mainFont = new Font( "font_32_64.tga", 3, 6, false, 12 );
        }
    // mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;
    
    if( viewHeightFraction < 9.0 / 16.0 ) {
        // weird, wider than 16:9 aspect ratio
        
        viewWidth = viewHeight / viewHeightFraction;
        }
    

    setViewSize( viewWidth );
    setLetterbox( visibleViewWidth, viewHeight );
    }


void freeDrawString() {
    delete mainFont;
    delete oldMainFont;
    }



void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
                      const char *inCustomRecordedGameData,
                      char inPlayingBack ) {

    // it's always safe to call this, just in case we're launching post-update
    postUpdate();
        

    instructionsSprite = loadWhiteSprite( "instructions.tga" );
    
    

    initAgeControl();
    
    updateDataVersionNumber();


    AppLog::printOutNextMessage();
    AppLog::infoF( "OneLife client v%d (binV=%d, dataV=%d) starting up",
                   versionNumber, binVersionNumber, dataVersionNumber );
            

    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    
    gamePlayingBack = inPlayingBack;
    
    screenW = inWidth;
    screenH = inHeight;
    
    if( inTargetFrameRate != baseFramesPerSecond ) {
        frameRateFactor = 
            (double)baseFramesPerSecond / (double)inTargetFrameRate;
        
        numLoadingSteps /= frameRateFactor;
        }
    
    targetFramesPerSecond = inTargetFrameRate;
    
    


    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;
    

    if( viewHeightFraction < 9.0 / 16.0 ) {
        // weird, wider than 16:9 aspect ratio
        
        viewWidth = viewHeight / viewHeightFraction;
        }
    
    setViewSize( viewWidth );
    setLetterbox( visibleViewWidth, viewHeight );


    
    

    

    setCursorVisible( true );
    grabInput( false );
    
    // world coordinates
    setMouseReportingMode( true );
    
    
    
    mainFontReview = new Font( getFontTGAFileName(), 4, 8, false, 16 );
    mainFontReview->setMinimumPositionPrecision( 1 );

    mainFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16 );
    numbersFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16, 16 );
    
    mainFontFixed->setMinimumPositionPrecision( 1 );
    numbersFontFixed->setMinimumPositionPrecision( 1 );
    
    smallFont = new Font( getFontTGAFileName(), 3, 8, false, 8 * gui_fov_scale_hud );

    titleFont = 
        new Font( "font_handwriting_32_32.tga", 3, 6, false, 20 * gui_fov_scale_hud );

    handwritingFont = 
        new Font( "font_handwriting_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

    handwritingFont->setMinimumPositionPrecision( 1 );
    
    tinyHandwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16/2 );
    tinyHandwritingFont->setMinimumPositionPrecision( 1 );

    pencilFont = 
        new Font( "font_pencil_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

    pencilFont->setMinimumPositionPrecision( 1 );

    pencilErasedFont = 
        new Font( "font_pencil_erased_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

    pencilErasedFont->setMinimumPositionPrecision( 1 );

    pencilErasedFont->copySpacing( pencilFont );
    
    
    float mouseSpeedSetting = 1.0f;
    
    int musicOffSetting = 0;
    float musicLoudnessSetting = 1.0f;
    
    int webRetrySecondsSetting = 10;

    
    int readVersionNumber;
    
    int numRead = sscanf( inCustomRecordedGameData, 
                          customDataFormatReadString, 
                          &readVersionNumber,
                          &mouseSpeedSetting, 
                          &musicOffSetting,
                          &musicLoudnessSetting,
                          &webRetrySecondsSetting );
    if( numRead != 6 ) {
        // no recorded game?
        }
    else {

        if( readVersionNumber != versionNumber ) {
            AppLog::printOutNextMessage();
            AppLog::warningF( 
                "WARNING:  version number in playback file is %d "
                "but game version is %d...",
                readVersionNumber, versionNumber );
            }
        }

    
    userEmail = SettingsManager::getStringSetting( "email" );    
    accountKey = SettingsManager::getStringSetting( "accountKey" );

    
    double mouseParam = 0.000976562;

    mouseParam *= mouseSpeedSetting;

    mouseSpeed = mouseParam * inWidth / viewWidth;

    musicOff = musicOffSetting;
    musicLoudness = musicLoudnessSetting;
    
    
    webRetrySeconds = webRetrySecondsSetting;

    reflectorURL = SettingsManager::getStringSetting( "reflectorURL" );

    if( reflectorURL == NULL ) {
        reflectorURL = 
            stringDuplicate( 
                "http://localhost/jcr13/oneLifeReflector/server.php" );
        }



    setSoundLoudness( 1.0 );
    setSoundPlaying( true );

    


    const char *resultNamesA[4] = { "serverIP", "serverPort",
                                    "requiredVersionNumber",
                                    "autoUpdateURL" };
    
    getServerAddressPage = new ServerActionPage( reflectorURL,
                                                 "reflect", 
                                                 4, resultNamesA, false );
    
    
    finalMessagePage = new FinalMessagePage;
    loadingPage = new LoadingPage;
    autoUpdatePage = new AutoUpdatePage;
    livingLifePage = NULL;
    existingAccountPage = new ExistingAccountPage;
    extendedMessagePage = new ExtendedMessagePage;
    rebirthChoicePage = new RebirthChoicePage;
    settingsPage = new SettingsPage;

#ifdef USE_DISCORD
    discordController = new DiscordController;
    EDiscordResult result = discordController->connect();
    if (result != EDiscordResult::DiscordResult_Ok) {
        printf("discord error: game: discord connection not successful\n");
        }
#endif // USE_DISCORD

    char *reviewURL = 
        SettingsManager::getStringSetting( "reviewServerURL", "" );
    
    if( strcmp( reviewURL, "" ) == 0 ) {
        existingAccountPage->showReviewButton( false );
        rebirthChoicePage->showReviewButton( false );
        }

    reviewPage = new ReviewPage( reviewURL );
    

    twinPage = new TwinPage();

    pollPage = new PollPage( reviewURL );
    delete [] reviewURL;
    
    geneticHistoryPage = new GeneticHistoryPage();
    

    // 0 music headroom needed, because we fade sounds before playing music
    setVolumeScaling( 10, 0 );
    //setSoundSpriteRateRange( 0.95, 1.05 );
    setSoundSpriteVolumeRange( 0.60, 1.0 );
    
    char rebuilding;
    
    int numSprites = 
        initSpriteBankStart( &rebuilding );
                        
    if( rebuilding ) {
        loadingPage->setCurrentPhase( translate( "spritesRebuild" ) );
        }
    else {
        loadingPage->setCurrentPhase( translate( "sprites" ) );
        }
    loadingPage->setCurrentProgress( 0 );
                        

    loadingStepBatchSize = numSprites / numLoadingSteps;
    
    if( loadingStepBatchSize < 1 ) {
        loadingStepBatchSize = 1;
        }

    // for filter support in LivingLifePage
    enableObjectSearch( true );


    currentGamePage = loadingPage;

    //testPage = new TestPage;
    //currentGamePage = testPage;

    currentGamePage->base_makeActive( true );

    initDone = true;
    }



void freeFrameDrawer() {


    freeSprite( instructionsSprite );
    
    delete mainFontReview;
    delete mainFontFixed;
    delete numbersFontFixed;
    
    delete handwritingFont;
    delete tinyHandwritingFont;
    delete pencilFont;
    delete pencilErasedFont;
    
    delete smallFont;
    
    if( currentUserTypedMessage != NULL ) {
        delete [] currentUserTypedMessage;
        currentUserTypedMessage = NULL;
        }

    

    if( shutdownMessage != NULL ) {
        delete [] shutdownMessage;
        shutdownMessage = NULL;
        }
    

    delete getServerAddressPage;
    
    delete finalMessagePage;
    delete loadingPage;
    delete autoUpdatePage;
    if( livingLifePage != NULL ) {
        delete livingLifePage;
        livingLifePage = NULL;
        }

    delete existingAccountPage;
    delete extendedMessagePage;
    delete rebirthChoicePage;
    delete settingsPage;
    delete reviewPage;
    delete twinPage;
    delete pollPage;
    delete geneticHistoryPage;
    
    //if( testPage != NULL ) {
    //    delete testPage;
    //    testPage = NULL;
    //    }

    
    freeGroundSprites();

    freeAnimationBank();
    freeObjectBank();
    freeSpriteBank();

    freeTransBank();
    
    freeCategoryBank();

    freeLiveObjectSet();

    freeSoundBank();
    
    freeMusicPlayer();
    freeEmotion();
    
    freePhotos();
    freeLifeTokens();
    freeFitnessScore();

    if( reflectorURL != NULL ) {
        delete [] reflectorURL;
        reflectorURL = NULL;
        }

    if( serverIP != NULL ) {
        delete [] serverIP;
        serverIP = NULL;
        }
    

    if( userEmail != NULL ) {
        delete [] userEmail;
        }
    if( accountKey != NULL ) {
        delete [] accountKey;
        }
    if( userTwinCode != NULL ) {
        delete [] userTwinCode;
        }
    }





    


// draw code separated from updates
// some updates are still embedded in draw code, so pass a switch to 
// turn them off
static void drawFrameNoUpdate( char inUpdate );



doublePair lastCursorPos = {0, 0};
bool hoveringResumeButton = false;
bool hoveringSettingsButton = false;
bool hoveringQuitButton = false;


static void drawPauseScreen() {
    
    if( currentGamePage == existingAccountPage && isPaused() ) {
        pauseGame();
        return;
        }
    
    
    doublePair messagePos = lastScreenViewCenter;
    if( currentGamePage != livingLifePage ) {
        double viewHeight = viewHeightFraction * viewWidth;

        setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
            
        drawSquare( lastScreenViewCenter, 1.05 * ( viewHeight / 3 ) );
            

        setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
            
        drawSquare( lastScreenViewCenter, viewHeight / 3 );
            

        setDrawColor( 1, 1, 1, pauseScreenFade );

        messagePos = lastScreenViewCenter;

        messagePos.y += 4.5  * (viewHeight / 15);

        oldMainFont->drawString( translate( "pauseMessage1" ), 
                               messagePos, alignCenter );
            
        messagePos.y -= 1.25 * (viewHeight / 15);
        oldMainFont->drawString( translate( "pauseMessage2" ), 
                               messagePos, alignCenter );
        }


    // Drawing the Pause screen
    if( currentGamePage == livingLifePage ) {
        
        int columnWidth = 450 * gui_fov_scale;
        int columnHeight = 600 * gui_fov_scale;
        int columnOffset = 300 * gui_fov_scale;
        int columnNumber = 0;
        
        // int lastDrawnColumn = 0;
        int lineHeight = 40 * gui_fov_scale;
        
        int columnStartX = -600 * gui_fov_scale;
        int columnStartY = -100 * gui_fov_scale;
        
        int temp;
        
        
        doublePair writePos;
        writePos.x = lastScreenViewCenter.x + columnStartX;
        writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight / 2;
        
            
        bool backgroundDrawn = false;
        
        // 2 passes for the stencil
        // This is to draw a background matching the color of sheet1 and sheet2
        for(int p=0;p<2;p++) 
        for(int c=1;c<3;c++) {
            
            if( p == 1 && !backgroundDrawn ) {
                startDrawingThroughStencil( true );
                setDrawColor( 0.91f, 0.82f, 0.66f, 0.8*pauseScreenFade ); // This is the color of sheet1 and 2
                drawRect( lastScreenViewCenter, 1280*4*gui_fov_scale, 720*4*gui_fov_scale );
                stopStencil();
                
                backgroundDrawn = true;
                }
            
            doublePair backgroundWritePos = writePos;
            
            if ( c == 2 ) {
                int current_columnX = columnStartX + ( abs( columnWidth ) + columnOffset ) * ( c - 1 );
                backgroundWritePos.x = lastScreenViewCenter.x + current_columnX;
                }
                
            if( p == 0 ) startAddingToStencil( false, true );
            
            setDrawColor( 1, 1, 1, 0.8f*pauseScreenFade );
                                        
            if ( sheetSprites[c] == nullptr ) {
                char columnName[11] = "sheet";
                char n[6];
                sprintf( n, "%d.tga", c );
                strcat( columnName, n );
                sheetSprites[c] = loadSprite( columnName, false );
                }
                
            doublePair drawPos;
            drawPos.x = backgroundWritePos.x + columnWidth/2;
            drawPos.y = lastScreenViewCenter.y; // + columnStartY;

            drawSprite( sheetSprites[c], drawPos, gui_fov_scale );
        
            }
            
        // This is the complementary color of that of sheet 1 and 2
        // This makes the Pause screen white-ish instead of yellow-ish as in sheet 1 and 2
        setDrawColor( 0.01f, 0.18f, 0.34f, 0.2*pauseScreenFade );
        drawRect( lastScreenViewCenter, 1280*4*gui_fov_scale, 720*4*gui_fov_scale );
        
        // Darkening the whole background a bit
        setDrawColor( 1.0f, 1.0f, 1.0f, 0.1*pauseScreenFade );
        drawRect( lastScreenViewCenter, 1280*4*gui_fov_scale, 720*4*gui_fov_scale );
        
        
        File languagesDir( NULL, "languages" );
        if ( languagesDir.exists() && languagesDir.isDirectory() ) {
            File *helpFile = languagesDir.getChildFile( "help_English.txt" );
            char *helpFileContents = helpFile->readFileContents();
            
            if( helpFileContents != NULL ) {
                int numLines;
                char **lines = split( helpFileContents, "\n", &numLines );
                char *subString;

                for( int i=0; i<numLines; i++ ) {
                    bool isTitle = false;
                    bool isSub = false;
                    bool isComment = false;
                    // bool isSheet = false;
                    if ( (lines[i][0] == '\0') || (lines[i][0] == '\r') ) {
                        //continue;
                        }
                    else if ( strstr( lines[i], "#sheet" ) != NULL ) {
                        sscanf( lines[i], "#sheet%d", &( columnNumber ) );
                        writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight/2; //reset lineHeight additions
                        // isSheet = true;
                        continue;
                        }
                    else if ( strstr( lines[i], "@COLUMN_W" ) != NULL ) {
                        sscanf( lines[i], "@COLUMN_W=%d", &( temp ) );
                        columnWidth = gui_fov_scale * temp;
                        continue;
                        }
                    else if ( strstr( lines[i], "@COLUMN_H" ) != NULL ) {
                        sscanf( lines[i], "@COLUMN_H=%d", &( temp ) );
                        columnHeight = gui_fov_scale * temp;
                        writePos.y = lastScreenViewCenter.y + columnHeight/2;
                        continue;
                        }
                    else if ( strstr( lines[i], "@COLUMN_O=" ) != NULL ) {
                        sscanf( lines[i], "@COLUMN_O=%d", &( temp ) );
                        columnOffset = gui_fov_scale * temp;
                        continue;
                        }
                    else if ( strstr( lines[i], "@START_X" ) != NULL ) {
                        sscanf( lines[i], "@START_X=%d", &( temp ) );
                        columnStartX = gui_fov_scale * temp;
                        writePos.x = lastScreenViewCenter.x + columnStartX;
                        continue;
                        }
                    else if ( strstr( lines[i], "@START_Y" ) != NULL ) {
                        sscanf( lines[i], "@START_Y=%d", &( temp ) );
                        columnStartY = gui_fov_scale * temp;
                        writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight/2;
                        continue;
                        }
                    else if ( strstr( lines[i], "@LINEHEIGHT" ) != NULL ) {
                        sscanf( lines[i], "@LINEHEIGHT=%d", &( temp ) );
                        lineHeight = gui_fov_scale * temp;
                        continue;
                        }
                    else if ( strstr( lines[i], "warning$" ) != NULL ) {
                        int hNumLines;
                        char **holder;
                        holder = split( lines[i], "$", &hNumLines);
                        lines[i] = holder[1];
                        isComment = true;
                        }
                    else if ( strstr( lines[i], "title$" ) != NULL ) {
                        int hNumLines;
                        char **holder;
                        holder = split( lines[i], "$", &hNumLines);
                        lines[i] = holder[1];
                        isTitle = true;
                        }
                    else if ( strstr( lines[i], "sub$" ) != NULL ) {
                        int hNumLines;
                        char **holder;
                        holder = split( lines[i], "$", &hNumLines);
                        lines[i] = holder[1];
                        subString = holder[2];
                        isSub = true;
                        }
                    else if ( strstr( lines[i], "space$" ) != NULL ) {
                        float lineScale;
                        sscanf( lines[i], "space$%f", &( lineScale ) );
                        writePos.y -= lineHeight * lineScale;
                        continue;
                        }
                        
                    if ( columnNumber > 1 ) {
                        int current_columnX = columnStartX + ( abs( columnWidth ) + columnOffset ) * ( columnNumber - 1 );
                        writePos.x = lastScreenViewCenter.x + current_columnX;
                        }
                        
                    if ( isComment ) {
                        // closeMessage = lines[i];
                        }
                    else if ( isTitle ) {
                        setDrawColor( 0.1f, 0.1f, 0.1f, 1*pauseScreenFade );
                        // int titleSize = titleFont->measureString( lines[i] );
                        // titleFont->drawString( lines[i], { writePos.x + (columnWidth - titleSize)/2, writePos.y - lineHeight }, alignLeft ); // Centered
                        titleFont->drawString( lines[i], { writePos.x + 40 * gui_fov_scale, writePos.y - lineHeight }, alignLeft ); // Left-align
                        writePos.y -= lineHeight * 0.75f*3;
                        }
                    else if ( isSub ) {
                        setDrawColor( 0.2f, 0.4f, 0.6f, 1*pauseScreenFade );
                        handwritingFont->drawString( lines[i], { writePos.x + 40 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
                        int subSize = handwritingFont->measureString( lines[i] );
                        setDrawColor( 0.1f, 0.1f, 0.1f, 1*pauseScreenFade );
                        pencilFont->drawString( subString, { writePos.x + subSize + 60 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
                        writePos.y -= lineHeight;
                        }
                    else {
                        setDrawColor( 0.1f, 0.1f, 0.1f, 1*pauseScreenFade );
                        pencilFont->drawString( lines[i], { writePos.x + 40 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
                        writePos.y -= lineHeight;
                        }
                    }
                delete [] lines;
                }
            }
        }

    // Drawing the Pause screen "buttons"
    // Can't use the usual textButtons here because they don't work when game is paused
    // So have to draw and make them work "mannually"
    if( currentGamePage == livingLifePage ) {
        
        doublePair resumeButtonPos = { 460*gui_fov_scale, -112*gui_fov_scale };
        doublePair settingsButtonPos = { 460*gui_fov_scale, -192*gui_fov_scale };
        doublePair quitButtonPos = { 460*gui_fov_scale, -272*gui_fov_scale };
        

        if( 1 ) { // Resume button
            doublePair buttonPos = {
                lastScreenViewCenter.x + resumeButtonPos.x, 
                lastScreenViewCenter.y + resumeButtonPos.y
                };
                
            char *buttonText = (char*)"[RESUME GAME]";
            
            int subSize = handwritingFont->measureString( buttonText );
                
            setDrawColor( 0, 0, 0, 1*pauseScreenFade );
            
            if( abs(lastCursorPos.x - buttonPos.x) < subSize &&
                abs(lastCursorPos.y - buttonPos.y) < 40/2*gui_fov_scale ) {
                hoveringResumeButton = true;
                setDrawColor( 1, 1, 1, 1*pauseScreenFade );
                }
            else {
                hoveringResumeButton = false;
                }
                
            handwritingFont->drawString( buttonText, buttonPos, alignCenter );
            }        
        if( 1 ) { // Settings button
            doublePair buttonPos = {
                lastScreenViewCenter.x + settingsButtonPos.x, 
                lastScreenViewCenter.y + settingsButtonPos.y
                };
                
            char *buttonText = (char*)"[SETTINGS]";
            
            int subSize = handwritingFont->measureString( buttonText );
                
            setDrawColor( 0, 0, 0, 1*pauseScreenFade );
            
            if( abs(lastCursorPos.x - buttonPos.x) < subSize &&
                abs(lastCursorPos.y - buttonPos.y) < 40/2*gui_fov_scale ) {
                hoveringSettingsButton = true;
                setDrawColor( 1, 1, 1, 1*pauseScreenFade );
                }
            else {
                hoveringSettingsButton = false;
                }
                
            handwritingFont->drawString( buttonText, buttonPos, alignCenter );
            }
        if( 1 ) { // Quit button
            doublePair buttonPos = {
                lastScreenViewCenter.x + quitButtonPos.x, 
                lastScreenViewCenter.y + quitButtonPos.y
                };
                
            char *buttonText = (char*)"[QUIT GAME]";
            
            int subSize = handwritingFont->measureString( buttonText );
                
            setDrawColor( 0, 0, 0, 1*pauseScreenFade );
            
            if( abs(lastCursorPos.x - buttonPos.x) < subSize &&
                abs(lastCursorPos.y - buttonPos.y) < 40/2*gui_fov_scale ) {
                hoveringQuitButton = true;
                setDrawColor( 1, 1, 1, 1*pauseScreenFade );
                }
            else {
                hoveringQuitButton = false;
                }
                
            handwritingFont->drawString( buttonText, buttonPos, alignCenter );
            }
        
        }
        


    if( currentUserTypedMessage != NULL ) {
            
        messagePos.y -= 1.25 * (viewHeight / 15);
            
        double maxWidth = 0.95 * ( viewHeight / 1.5 );
            
        int maxLines = 9;

        SimpleVector<char *> *tokens = 
            tokenizeString( currentUserTypedMessage );


        // collect all lines before drawing them
        SimpleVector<char *> lines;
        
            
        while( tokens->size() > 0 ) {

            // build up a a line

            // always take at least first token, even if it is too long
            char *currentLineString = 
                stringDuplicate( *( tokens->getElement( 0 ) ) );
                
            delete [] *( tokens->getElement( 0 ) );
            tokens->deleteElement( 0 );
            
            

            

            
            char nextTokenIsFileSeparator = false;
                
            char *nextLongerString = NULL;
                
            if( tokens->size() > 0 ) {

                char *nextToken = *( tokens->getElement( 0 ) );
                
                if( nextToken[0] == 28 ) {
                    nextTokenIsFileSeparator = true;
                    }
                else {
                    nextLongerString =
                        autoSprintf( "%s %s ",
                                     currentLineString,
                                     *( tokens->getElement( 0 ) ) );
                    }
                
                }
                
            while( !nextTokenIsFileSeparator 
                   &&
                   nextLongerString != NULL 
                   && 
                   mainFont->measureString( nextLongerString ) 
                   < maxWidth 
                   &&
                   tokens->size() > 0 ) {
                    
                delete [] currentLineString;
                    
                currentLineString = nextLongerString;
                    
                nextLongerString = NULL;
                    
                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                    
                if( tokens->size() > 0 ) {
                    
                    char *nextToken = *( tokens->getElement( 0 ) );
                
                    if( nextToken[0] == 28 ) {
                        nextTokenIsFileSeparator = true;
                        }
                    else {
                        nextLongerString =
                            autoSprintf( "%s%s ",
                                         currentLineString,
                                         *( tokens->getElement( 0 ) ) );
                        }
                    }
                }
                
            if( nextLongerString != NULL ) {    
                delete [] nextLongerString;
                }
                
            while( mainFont->measureString( currentLineString ) > 
                   maxWidth ) {
                    
                // single token that is too long by itself
                // simply trim it and discard part of it 
                // (user typing nonsense anyway)
                    
                currentLineString[ strlen( currentLineString ) - 1 ] =
                    '\0';
                }
                
            if( currentLineString[ strlen( currentLineString ) - 1 ] 
                == ' ' ) {
                // trim last bit of whitespace
                currentLineString[ strlen( currentLineString ) - 1 ] = 
                    '\0';
                }

                
            lines.push_back( currentLineString );

            
            if( nextTokenIsFileSeparator ) {
                // file separator

                // put a paragraph separator in
                lines.push_back( stringDuplicate( "---" ) );

                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                }
            }   


        // all tokens deleted above
        delete tokens;


        double messageLineSpacing = 0.625 * (viewHeight / 15);
        
        int numLinesToSkip = lines.size() - maxLines;

        if( numLinesToSkip < 0 ) {
            numLinesToSkip = 0;
            }
        
        
        for( int i=0; i<numLinesToSkip-1; i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            delete [] currentLineString;
            }
        
        int lastSkipLine = numLinesToSkip - 1;

        if( lastSkipLine >= 0 ) {
            
            char *currentLineString = *( lines.getElement( lastSkipLine ) );

            // draw above and faded out somewhat

            doublePair lastSkipLinePos = messagePos;
            
            lastSkipLinePos.y += messageLineSpacing;

            setDrawColor( 1, 1, 0.5, 0.125 * pauseScreenFade );

            mainFont->drawString( currentLineString, 
                                   lastSkipLinePos, alignCenter );

            
            delete [] currentLineString;
            }
        

        setDrawColor( 1, 1, 0.5, pauseScreenFade );

        for( int i=numLinesToSkip; i<lines.size(); i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            
            if( false && lastSkipLine >= 0 ) {
            
                if( i == numLinesToSkip ) {
                    // next to last
                    setDrawColor( 1, 1, 0.5, 0.25 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 1 ) {
                    // next after that
                    setDrawColor( 1, 1, 0.5, 0.5 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 2 ) {
                    // rest are full fade
                    setDrawColor( 1, 1, 0.5, pauseScreenFade );
                    }
                }
            
            mainFont->drawString( currentLineString, 
                                   messagePos, alignCenter );

            delete [] currentLineString;
                
            messagePos.y -= messageLineSpacing;
            }
        }
        
        
    if( currentGamePage != livingLifePage ) {
        setDrawColor( 1, 1, 1, pauseScreenFade );

        messagePos = lastScreenViewCenter;

        //messagePos.y -= 3.75 * ( viewHeight / 15 );
        //mainFont->drawString( translate( "pauseMessage3" ), 
        //                      messagePos, alignCenter );

        messagePos.y -= 3.75 * ( viewHeight / 15 );
        
        if ( currentGamePage == livingLifePage ) {
            mainFont->drawString( translate( "pauseMessage5" ), 
                                  messagePos, alignCenter );
            }

        messagePos.y -= 0.625 * (viewHeight / 15);

        const char* quitMessageKey = "pauseMessage3";
        
        if( isQuittingBlocked() ) {
            quitMessageKey = "pauseMessage3b";
            }

        oldMainFont->drawString( translate( quitMessageKey ), 
                              messagePos, alignCenter );
        }
    }



void deleteCharFromUserTypedMessage() {
    if( currentUserTypedMessage != NULL ) {
                    
        int length = strlen( currentUserTypedMessage );
        
        char fileSeparatorDeleted = false;
        if( length > 2 ) {
            if( currentUserTypedMessage[ length - 2 ] == 28 ) {
                // file separator with spaces around it
                // delete whole thing with one keypress
                currentUserTypedMessage[ length - 3 ] = '\0';
                fileSeparatorDeleted = true;
                }
            }
        if( !fileSeparatorDeleted && length > 0 ) {
            currentUserTypedMessage[ length - 1 ] = '\0';
            }
        }
    }





static void startConnecting() {
    userReconnect = false;
    
    if( SettingsManager::getIntSetting( "useCustomServer", 0 ) ) {
        usingCustomServer = true;
        
        if( serverIP != NULL ) {
            delete [] serverIP;
            serverIP = NULL;
            }
        serverIP = SettingsManager::getStringSetting( 
            "customServerAddress" );
        if( serverIP == NULL ) {
            serverIP = stringDuplicate( "127.0.0.1" );
            }
        serverPort = SettingsManager::getIntSetting( 
            "customServerPort", 8005 );

        printf( "Using custom server address: %s:%d\n", 
                serverIP, serverPort );
                    
        currentGamePage = livingLifePage;
        currentGamePage->base_makeActive( true );
        }
    else {
        usingCustomServer = false;
        
        printf( "Starting fetching server URL from reflector %s\n",
                reflectorURL );
                
        getServerAddressPage->clearActionParameters();
        
            
        getServerAddressPage->setActionParameter( "email", 
                                                  userEmail );
        
        if( userTwinCode != NULL ) {
            char *codeHash = computeSHA1Digest( userTwinCode );
            getServerAddressPage->setActionParameter( "twin_code", 
                                                      codeHash );
            delete [] codeHash;
            }
        
                    
        currentGamePage = getServerAddressPage;
        currentGamePage->base_makeActive( true );
        }

    }



void showSettings() {
    showingInGameSettings = true;
    
    // temporarily reset fov before going into settings page
    gui_fov_scale_before_settings = gui_fov_scale;
    livingLifePage->changeFOV( 1.0f );

    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, 
                           lastScreenViewCenter.y );
    
    currentGamePage = settingsPage;
    
    currentGamePage->base_makeActive( true );
    }

void showDiedPage() {
    userReconnect = false;
    
    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, 
                           lastScreenViewCenter.y );
    
    currentGamePage = extendedMessagePage;
    
    extendedMessagePage->setMessageKey( "youDied" );
    
    char *reason = livingLifePage->getDeathReason();
    
    if( reason == NULL ) {
        extendedMessagePage->setSubMessage( "" );
        }
    else {
        extendedMessagePage->setSubMessage( reason );
        
        delete [] reason;
        }
    
    
    currentGamePage->base_makeActive( true );
    }



void showReconnectPage() {
    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, 
                           lastScreenViewCenter.y );
    
    currentGamePage = extendedMessagePage;
    
    extendedMessagePage->setMessageKey( "connectionLost" );

    extendedMessagePage->setSubMessage( translate( "willTryReconnect" ) );
    
    userReconnect = true;
    
    // don't reconnect as twin
    // that will cause them to wait for their party again.
    if( userTwinCode != NULL ) {
        delete [] userTwinCode;
        userTwinCode = NULL;
        }
    
    currentGamePage->base_makeActive( true );
    }

#ifdef USE_DISCORD // <-- vscode doesnt know it's enabled at compile time... it can be overriden with .vscode/c_cpp_properties.json, content: {"configurations":[{"defines": ["USE_DISCORD"],}],}
void discordStep() {
    if (discordController == NULL) {
        return;
        }
    if (currentGamePage == loadingPage) {
        discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::LOADING_PAGE, NULL);
        }
    else if (currentGamePage == livingLifePage) {
        if (!livingLifePage->isLivingLife()) {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::WAITING_TO_BE_BORN_PAGE, NULL);
            }
        else if (livingLifePage->isTutorial()) {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::LIVING_TUTORIAL_PAGE, livingLifePage);
            }
        else {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::LIVING_LIFE_PAGE, livingLifePage);
            }
        }
    else if (currentGamePage == settingsPage) {
            if (livingLifePage != NULL && livingLifePage->isLivingLife()){
                // allow player to see their changes without exiting the settings page.
                if (livingLifePage->isTutorial()) {
                    discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::LIVING_TUTORIAL_PAGE, livingLifePage);
                    }
                else{
                    discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::LIVING_LIFE_PAGE, livingLifePage);
                    }
                }
            else {
                discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::SETTINGS_PAGE, NULL);
                }
        }
    else if (currentGamePage == extendedMessagePage) {
        if (0 == strcmp("connectionLost", extendedMessagePage->getMessageKey())) {
            // TODO: sometimes we lose connection when we die, i think it's becuase server disconnects us before we get our death message.
            // just an age check would fix it, and show the died status instead of connection lost.
            if (livingLifePage != NULL && livingLifePage->getLastComputedAge() > 119.5) { // REVIEW: make 119.5 variable, not constant, create it from settings
                // this is an attempt to handle the previous todo.
                discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::DEATH_PAGE, livingLifePage);
                }
            else {
                discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::DISONNECTED_PAGE, NULL);
                }
            }
        else if (0 == strcmp("youDied", extendedMessagePage->getMessageKey())) {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::DEATH_PAGE, livingLifePage);
            }
        else if (0 == strcmp("connectionFailed", extendedMessagePage->getMessageKey())) {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::DISONNECTED_PAGE, NULL);
            }
        else {
            discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::MAIN_MENU_PAGE, NULL);
            }
        }
    else if (currentGamePage == getServerAddressPage) {
        discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::WAITING_TO_BE_BORN_PAGE, NULL);
        }
    else {
        discordController->lazyUpdateRichPresence(DiscordCurrentGamePage::MAIN_MENU_PAGE, NULL);
        }
    discordController->runCallbacks();
    }
#endif // USE_DISCORD

void drawFrame( char inUpdate ) {    

    if( !inUpdate ) {
        
        // because this is a networked game, we can't actually pause
        stepSpriteBank();
        
        stepSoundBank();
        
        stepMusicPlayer();
    
        if( currentGamePage != NULL ) {
            currentGamePage->base_step();
            }
        wakeUpPauseFrameRate();


        drawFrameNoUpdate( true );
            
        drawPauseScreen();
        

        // handle delete key repeat
        if( holdDeleteKeySteps > -1 ) {
            holdDeleteKeySteps ++;
            
            if( holdDeleteKeySteps > stepsBetweenDeleteRepeat ) {        
                // delete repeat

                // platform layer doesn't receive event for key held down
                // tell it we are still active so that it doesn't
                // reduce the framerate during long, held deletes
                wakeUpPauseFrameRate();
                


                // subtract from messsage
                deleteCharFromUserTypedMessage();
                
                            

                // shorter delay for subsequent repeats
                stepsBetweenDeleteRepeat = (int)( 2/ frameRateFactor );
                holdDeleteKeySteps = 0;
                }
            }

        // fade in pause screen
        if( pauseScreenFade < 1 ) {
            
            // pauseScreenFade = 1;
            
            pauseScreenFade += ( 1.0 / 15 ) * frameRateFactor;
        
            if( pauseScreenFade > 1 ) {
                pauseScreenFade = 1;
                }
            }
        
        
        // keep checking for this signal even if paused
        if( currentGamePage == livingLifePage &&
            livingLifePage->checkSignal( "died" ) ) {
            showDiedPage();
            }
        if( currentGamePage == livingLifePage &&
            livingLifePage->checkSignal( "disconnect" ) ) {
    
            showReconnectPage();
            }
#ifdef USE_DISCORD
        discordStep();// called when paused too, to change to IDLE state if player idle is detected.
#endif // USE_DISCORD
        return;
        }


    // not paused


    // fade pause screen out
    if( pauseScreenFade > 0 ) {
        pauseScreenFade -= ( 1.0 / 30 ) * frameRateFactor;
        
        if( pauseScreenFade < 0 ) {
            pauseScreenFade = 0;

            if( currentUserTypedMessage != NULL ) {

                // make sure it doesn't already end with a file separator
                // (never insert two in a row, even when player closes
                //  pause screen without typing anything)
                int lengthCurrent = strlen( currentUserTypedMessage );

                if( lengthCurrent < 2 ||
                    currentUserTypedMessage[ lengthCurrent - 2 ] != 28 ) {
                         
                        
                    // insert at file separator (ascii 28)
                    
                    char *oldMessage = currentUserTypedMessage;
                    
                    currentUserTypedMessage = autoSprintf( "%s %c ", 
                                                           oldMessage,
                                                           28 );
                    delete [] oldMessage;
                    }
                }
            }
        }    
    
    

    if( !firstDrawFrameCalled ) {
        
        // do final init step... stuff that shouldn't be done until
        // we have control of screen
        
        char *moveKeyMapping = 
            SettingsManager::getStringSetting( "upLeftDownRightKeys" );
    
        if( moveKeyMapping != NULL ) {
            char *temp = stringToLowerCase( moveKeyMapping );
            delete [] moveKeyMapping;
            moveKeyMapping = temp;
        
            if( strlen( moveKeyMapping ) == 4 &&
                strcmp( moveKeyMapping, "wasd" ) != 0 ) {
                // different assignment

                upKey = moveKeyMapping[0];
                leftKey = moveKeyMapping[1];
                downKey = moveKeyMapping[2];
                rightKey = moveKeyMapping[3];
                }
            delete [] moveKeyMapping;
            }




        firstDrawFrameCalled = true;
        }




    // updates here
    stepSpriteBank();
    
    stepSoundBank();
    
    stepMusicPlayer();
    
    stepPhotos();
    

    if( currentGamePage != NULL ) {
        currentGamePage->base_step();


        if( currentGamePage == loadingPage ) {
            
            switch( loadingPhase ) {
                case 0: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initSpriteBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initSpriteBankFinish();
                        
                        loadingPhaseStartTime = Time::getCurrentTime();
      
                        char rebuilding;

                        int numSounds = initSoundBankStart( &rebuilding );

                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                translate( "soundsRebuild" ) );
                            }
                        else {
                            loadingPage->setCurrentPhase(
                                translate( "sounds" ) );
                            }

                        loadingPage->setCurrentProgress( 0 );
                        
                            
                        loadingStepBatchSize = numSounds / numLoadingSteps;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }
                        
                        loadingPhase ++;
                        }
                    break;
                    }
                case 1: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initSoundBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initSoundBankFinish();
                        
                        loadingPhaseStartTime = Time::getCurrentTime();
                        
                        char rebuilding;
                        
                        int numAnimations = 
                            initAnimationBankStart( &rebuilding );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                translate( "animationsRebuild" ) );
                            }
                        else {
                            loadingPage->setCurrentPhase(
                                translate( "animations" ) );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numAnimations / numLoadingSteps;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }
                        
                        loadingPhase ++;
                        }
                    break;
                    }
                case 2: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initAnimationBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initAnimationBankFinish();
                        printf( "Finished loading animation bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        int numObjects = 
                            initObjectBankStart( &rebuilding, true, true );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase(
                                translate( "objectsRebuild" ) );
                            }
                        else {
                            loadingPage->setCurrentPhase( 
                                translate( "objects" ) );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numObjects / numLoadingSteps;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 3: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initObjectBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initObjectBankFinish();
                        printf( "Finished loading object bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        int numCats = 
                            initCategoryBankStart( &rebuilding );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase(
                                translate( "categoriesRebuild" ) );
                            }
                        else {
                            loadingPage->setCurrentPhase( 
                                translate( "categories" ) );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numCats / numLoadingSteps;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 4: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initCategoryBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initCategoryBankFinish();
                        printf( "Finished loading category bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        // true to auto-generate concrete transitions
                        // for all abstract category transitions
                        int numTrans = 
                            initTransBankStart( &rebuilding, true, true, true,
                                                true );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase(
                                translate( "transitionsRebuild" ) );
                            }
                        else {
                            loadingPage->setCurrentPhase( 
                                translate( "transitions" ) );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numTrans / numLoadingSteps;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 5: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initTransBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initTransBankFinish();
                        printf( "Finished loading transition bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        
                        loadingPhaseStartTime = Time::getCurrentTime();

                        loadingPage->setCurrentPhase( 
                            translate( "groundTextures" ) );

                        loadingPage->setCurrentProgress( 0 );
                        
                        initGroundSpritesStart();

                        loadingStepBatchSize = 1;
                        

                        loadingPhase ++;
                        }
                    break;
                    }
                case 6: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initGroundSpritesStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initGroundSpritesFinish();
                        printf( "Finished loading ground sprites in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        
                        loadingPhaseStartTime = Time::getCurrentTime();

                        
                        initLiveObjectSet();

                        loadingPhaseStartTime = Time::getCurrentTime();

                        livingLifePage = new LivingLifePage();
                    
                        loadingPhase ++;
                        }
                    break;
                    }
                default:
                    // NOW game engine can start measuring frame rate
                    loadingComplete();
                    

                    initEmotion();
                    initPhotos();
                    
                    initLifeTokens();
                    initFitnessScore();
                    
                    initMusicPlayer();
                    setMusicLoudness( musicLoudness );
                    
                    mapPullMode = 
                        SettingsManager::getIntSetting( "mapPullMode", 0 );
                    autoLogIn = 
                        SettingsManager::getIntSetting( "autoLogIn", 0 );

                    if( userEmail == NULL || accountKey == NULL ) {
                        autoLogIn = false;
                        }

                    currentGamePage = existingAccountPage;
                    currentGamePage->base_makeActive( true );
                }
            }
            else if( currentGamePage == settingsPage ) {
                if( settingsPage->checkSignal( "back" ) ) {
                    existingAccountPage->setStatus( NULL, false );

                    if ( showingInGameSettings ) {
                        
                        // restore fov before when we leave in-game settings page
                        livingLifePage->changeFOV( gui_fov_scale_before_settings );
                        gui_fov_scale_before_settings = 1.0f;
                        
                        currentGamePage = livingLifePage;
                        currentGamePage->base_makeActive( false );
                        showingInGameSettings = false;
                        }
                    else {
                        currentGamePage = existingAccountPage;
                        currentGamePage->base_makeActive( true );
                        }
                }
            else if( settingsPage->checkSignal( "editAccount" ) ) {
                loginEditOverride = true;
                
                existingAccountPage->setStatus( "editAccountWarning", false );
                existingAccountPage->setStatusPosition( false );
                
                currentGamePage = existingAccountPage;
                currentGamePage->base_makeActive( true );
                }
            else if( settingsPage->checkSignal( "relaunchFailed" ) ) {
                currentGamePage = finalMessagePage;
                        
                finalMessagePage->setMessageKey( "manualRestartMessage" );
                                
                currentGamePage->base_makeActive( true );
                }

            }
        else if( currentGamePage == reviewPage ) {
            if( reviewPage->checkSignal( "back" ) ) {
                existingAccountPage->setStatus( NULL, false );
                currentGamePage = existingAccountPage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == twinPage ) {
            if( twinPage->checkSignal( "cancel" ) ) {
                existingAccountPage->setStatus( NULL, false );
                currentGamePage = existingAccountPage;
                currentGamePage->base_makeActive( true );
                }
            else if( twinPage->checkSignal( "done" ) ) {
                startConnecting();
                }
            }
        else if( currentGamePage == existingAccountPage ) {    
            if( existingAccountPage->checkSignal( "quit" ) ) {
#ifdef USE_DISCORD
                delete discordController; // call deconstructor to cleanly disconnect
#endif // USE_DISCORD
                quitGame();
                }
            else if( existingAccountPage->checkSignal( "poll" ) ) {
                currentGamePage = pollPage;
                currentGamePage->base_makeActive( true );
                }
            else if( existingAccountPage->checkSignal( "genes" ) ) {
                currentGamePage = geneticHistoryPage;
                currentGamePage->base_makeActive( true );
                }
            else if( existingAccountPage->checkSignal( "settings" ) ) {
                currentGamePage = settingsPage;
                currentGamePage->base_makeActive( true );
                }
            else if( existingAccountPage->checkSignal( "review" ) ) {
                currentGamePage = reviewPage;
                currentGamePage->base_makeActive( true );
                }
            else if( existingAccountPage->checkSignal( "friends" ) ) {
                currentGamePage = twinPage;
                currentGamePage->base_makeActive( true );
                }
            else if( existingAccountPage->checkSignal( "done" )
                     || 
                     mapPullMode || autoLogIn ) {
                
                // auto-log-in one time for map pull
                // or one time for autoLogInMode
                mapPullMode = false;
                autoLogIn = false;

                startConnecting();
                }
            else if( existingAccountPage->checkSignal( "tutorial" ) ) {
                livingLifePage->runTutorial();

                // tutorial button clears twin status
                // they have to login from twin page to play as twin
                // if( userTwinCode != NULL ) {
                    // delete [] userTwinCode;
                    // userTwinCode = NULL;
                    // }

                startConnecting();
                }
            else if( autoUpdatePage->checkSignal( "relaunchFailed" ) ) {
                currentGamePage = finalMessagePage;
                        
                finalMessagePage->setMessageKey( "manualRestartMessage" );
                                
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == getServerAddressPage ) {
            if( getServerAddressPage->isResponseReady() ) {
                
                if( serverIP != NULL ) {
                    delete [] serverIP;
                    }

                serverIP = getServerAddressPage->getResponse( "serverIP" );
                
                serverPort = 
                    getServerAddressPage->getResponseInt( "serverPort" );
                
                
                if( strstr( serverIP, "NONE_FOUND" ) != NULL ) {
                    
                    currentGamePage = finalMessagePage;
                        
                    finalMessagePage->setMessageKey( "serverShutdownMessage" );
                    
                    
                    currentGamePage->base_makeActive( true );
                    }
                else {
                    

                    printf( "Got server address: %s:%d\n", 
                            serverIP, serverPort );
                
                    int requiredVersion =
                        getServerAddressPage->getResponseInt( 
                            "requiredVersionNumber" );
                    
                    if( versionNumber < requiredVersion ) {

                        if( SettingsManager::getIntSetting( 
                                "useSteamUpdate", 0 ) ) {
                            
                            // flag SteamGate that app needs update
                            FILE *f = fopen( "steamGateForceUpdate.txt", "w" );
                            if( f != NULL ) {    
                                fprintf( f, "1" );
                                fclose( f );
                                }
                            
                            // launch steamGateClient in parallel
                            // it will tell Steam that the app is dirty
                            // and needs to be updated.
                            runSteamGateClient();
                            

                            
                            currentGamePage = finalMessagePage;
                                
                            finalMessagePage->setMessageKey( 
                                "upgradeMessageSteam" );
                            
                            currentGamePage->base_makeActive( true );    
                            }
                        else {
                            char *autoUpdateURL = 
                                getServerAddressPage->getResponse( 
                                    "autoUpdateURL" );

                            char updateStarted = 
                                startUpdate( autoUpdateURL, versionNumber );
                        
                            delete [] autoUpdateURL;
                            
                            if( ! updateStarted ) {
                                currentGamePage = finalMessagePage;
                                
                                finalMessagePage->setMessageKey( 
                                    "upgradeMessage" );
                                
                                currentGamePage->base_makeActive( true );
                                }
                            else {
                                currentGamePage = autoUpdatePage;
                                currentGamePage->base_makeActive( true );
                                }
                            }
                        }
                    else {
                        // up to date, okay to connect
                        currentGamePage = livingLifePage;
                        currentGamePage->base_makeActive( true );
                        }
                    }
                }
            }
        else  if( currentGamePage == autoUpdatePage ) {
            if( autoUpdatePage->checkSignal( "failed" ) ) {
                currentGamePage = finalMessagePage;
                        
                finalMessagePage->setMessageKey( "upgradeMessage" );
                        
                currentGamePage->base_makeActive( true );
                }
            else if( autoUpdatePage->checkSignal( "writeError" ) ) {
                currentGamePage = finalMessagePage;
                
                finalMessagePage->setMessageKey( 
                    "updateWritePermissionMessage" );
                        
                currentGamePage->base_makeActive( true );
                }
            else if( autoUpdatePage->checkSignal( "relaunchFailed" ) ) {
                currentGamePage = finalMessagePage;
                        
                finalMessagePage->setMessageKey( "manualRestartMessage" );
                                
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == livingLifePage ) {
            if( livingLifePage->checkSignal( "loginFailed" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "loginFailed", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "targetFamilyFailed" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatusDirect( "TARGET FAMILY NOT FOUND##OR HAS NO FERTILES", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "noLifeTokens" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "noLifeTokens", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "connectionFailed" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "connectionFailed", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "versionMismatch" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                char *message = autoSprintf( translate( "versionMismatch" ),
                                             versionNumber,
                                             livingLifePage->
                                             getRequiredVersion() );

                if( SettingsManager::getIntSetting( "useCustomServer", 0 ) ) {
                    existingAccountPage->showDisableCustomServerButton( true );
                    }
                

                existingAccountPage->setStatusDirect( message, true );
                
                delete [] message;
                
                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "twinCancel" ) ) {
                
                existingAccountPage->setStatus( NULL, false );

                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "serverShutdown" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "serverShutdown", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "serverUpdate" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "serverUpdate", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "serverFull" ) ) {
                lastScreenViewCenter.x = 0;
                lastScreenViewCenter.y = 0;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                currentGamePage = existingAccountPage;
                
                existingAccountPage->setStatus( "serverFull", true );

                existingAccountPage->setStatusPosition( false );

                currentGamePage->base_makeActive( true );
                }
            else if( livingLifePage->checkSignal( "died" ) ) {
                showDiedPage();
                }
            else if( livingLifePage->checkSignal( "disconnect" ) ) {
                showReconnectPage();
                }
            else if( livingLifePage->checkSignal( "loadFailure" ) ) {
                currentGamePage = finalMessagePage;
                        
                finalMessagePage->setMessageKey( "loadingMapFailedMessage" );
                
                char *failedFileName = getSpriteBankLoadFailure();
                if( failedFileName == NULL ) {
                    failedFileName = getSoundBankLoadFailure();
                    }

                if( failedFileName != NULL ) {
                    
                    char *detailMessage = 
                        autoSprintf( translate( "loadingMapFailedSubMessage" ), 
                                     failedFileName );
                    finalMessagePage->setSubMessage( detailMessage );
                    delete [] detailMessage;
                    }

                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == extendedMessagePage ) {
            if( extendedMessagePage->checkSignal( "done" ) ) {
                
                extendedMessagePage->setSubMessage( "" );
                
                if( userReconnect ) {
                    currentGamePage = livingLifePage;
                    }
                else {
                    currentGamePage = pollPage;
                    }
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == pollPage ) {
            if( pollPage->checkSignal( "done" ) ) {
                currentGamePage = rebirthChoicePage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == geneticHistoryPage ) {
            if( geneticHistoryPage->checkSignal( "done" ) ) {
                if( !isHardToQuitMode() ) {
                    currentGamePage = existingAccountPage;
                    }
                else {
                    currentGamePage = rebirthChoicePage;
                    }
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == rebirthChoicePage ) {
            if( rebirthChoicePage->checkSignal( "reborn" ) ) {
                // get server address again from scratch, in case
                // the server we were on just crashed
                
                // but keep twin status, if set
                startConnecting();
                }
            else if( rebirthChoicePage->checkSignal( "tutorial" ) ) {
                livingLifePage->runTutorial();
                // heck, allow twins in tutorial too, for now, it's funny
                startConnecting();
                }
            else if( rebirthChoicePage->checkSignal( "settings" ) ) {
                currentGamePage = settingsPage;
                currentGamePage->base_makeActive( true );
                }
            else if( rebirthChoicePage->checkSignal( "review" ) ) {
                currentGamePage = reviewPage;
                currentGamePage->base_makeActive( true );
                }
            else if( rebirthChoicePage->checkSignal( "menu" ) ) {
                existingAccountPage->setStatus( NULL, false );
                currentGamePage = existingAccountPage;
                currentGamePage->base_makeActive( true );
                }
            else if( rebirthChoicePage->checkSignal( "genes" ) ) {
                currentGamePage = geneticHistoryPage;
                currentGamePage->base_makeActive( true );
                }
            else if( rebirthChoicePage->checkSignal( "quit" ) ) {
#ifdef USE_DISCORD
                delete discordController; // call deconstructor to cleanly disconnect
#endif // USE_DISCORD
                quitGame();
                }
            }
        else if( currentGamePage == finalMessagePage ) {
            if( finalMessagePage->checkSignal( "quit" ) ) {
#ifdef USE_DISCORD
                delete discordController; // call deconstructor to cleanly disconnect
#endif // USE_DISCORD
                quitGame();
                }
            }
#ifdef USE_DISCORD
        discordStep();
#endif // USE_DISCORD
    }

    // now draw stuff AFTER all updates
    drawFrameNoUpdate( true );






    // draw tail end of pause screen, if it is still visible
    if( pauseScreenFade > 0 ) {
        drawPauseScreen();
        }
    }




void drawFrameNoUpdate( char inUpdate ) {
    if( currentGamePage != NULL ) {
        currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
        }
    }




// store mouse data for use as unguessable randomizing data
// for key generation, etc.
#define MOUSE_DATA_BUFFER_SIZE 20
int mouseDataBufferSize = MOUSE_DATA_BUFFER_SIZE;
int nextMouseDataIndex = 0;
// ensure that stationary mouse data (same value over and over)
// doesn't overwrite data from actual motion
float lastBufferedMouseValue = 0;
float mouseDataBuffer[ MOUSE_DATA_BUFFER_SIZE ];


void pointerMove( float inX, float inY ) {
    
    lastCursorPos = { inX, inY };

    // save all mouse movement data for key generation
    float bufferValue = inX + inY;
    // ignore mouse positions that are the same as the last one
    // only save data when mouse actually moving
    if( bufferValue != lastBufferedMouseValue ) {
        
        mouseDataBuffer[ nextMouseDataIndex ] = bufferValue;
        lastBufferedMouseValue = bufferValue;
        
        nextMouseDataIndex ++;
        if( nextMouseDataIndex >= mouseDataBufferSize ) {
            nextMouseDataIndex = 0;
            }
        }
    

    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerMove( inX, inY );
        }
    }





void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDown( inX, inY );
        }
    }



void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDrag( inX, inY );
        }
    }



void pointerUp( float inX, float inY ) {
    
    // "Buttons" in the Pause screen in-game
    if( currentGamePage == livingLifePage && isPaused() ) {
        if( hoveringResumeButton ) pauseGame();
        if( hoveringQuitButton ) {
#ifdef USE_DISCORD
            delete discordController;// call deconstructor to cleanly disconnect
#endif // USE_DISCORD
            quitGame();
            }
        if( hoveringSettingsButton ) {
            pauseGame();
            pauseScreenFade = 0;
            showSettings();
            }
        }
    
    if( isPaused() ) {
        return;
        }
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerUp( inX, inY );
        }
    }







void keyDown( unsigned char inASCII ) {if(inASCII==27) return;

    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }
    /*
    if( inASCII == 'N' ) {
        toggleMipMapMinFilter( true );
        }
    if( inASCII == 'n' ) {
        toggleMipMapMinFilter( false );
        }
    */

    
    if( isPaused() ) {
        // block general keyboard control during pause


        switch( inASCII ) {
            case 13:  // enter
                // unpause
                pauseGame();
                break;
            case 35: // hash
                if ( currentGamePage == livingLifePage ) {
                    //unpáuse, reset fov then show settings
                    pauseGame();
                    showSettings();
                    }
                break;
            }
        
        // don't let user type on pause screen anymore
        return;

        
        if( inASCII == 127 || inASCII == 8 ) {
            // subtract from it

            deleteCharFromUserTypedMessage();

            holdDeleteKeySteps = 0;
            // start with long delay until first repeat
            stepsBetweenDeleteRepeat = (int)( 30 / frameRateFactor );
            }
        else if( inASCII >= 32 ) {
            // add to it
            if( currentUserTypedMessage != NULL ) {
                
                char *oldMessage = currentUserTypedMessage;

                currentUserTypedMessage = autoSprintf( "%s%c", 
                                                       oldMessage, inASCII );
                delete [] oldMessage;
                }
            else {
                currentUserTypedMessage = autoSprintf( "%c", inASCII );
                }
            }
        
        return;
        }
    

    if( currentGamePage != NULL ) {
        currentGamePage->base_keyDown( inASCII );
        }


    
    switch( inASCII ) {
        case 'm':
        case 'M': {
#ifdef USE_MALLINFO
            struct mallinfo meminfo = mallinfo();
            printf( "Mem alloc: %d\n",
                    meminfo.uordblks / 1024 );
#endif
            }
            break;
        }
    }



void keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // delete no longer held
        // even if pause screen no longer up, pay attention to this
        holdDeleteKeySteps = -1;
        }

    if( isPaused() ) return;

    if( currentGamePage != NULL ) {
        currentGamePage->base_keyUp( inASCII );
        }

    }







void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyDown( inKey );
        }
    }



void specialKeyUp( int inKey ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyUp( inKey );
        }
    } 




char getUsesSound() {
    
    return ! musicOff;
    }









void drawString( const char *inString, char inForceCenter ) {
    
    setDrawColor( 1, 1, 1, 0.75 );

    doublePair messagePos = lastScreenViewCenter;

    TextAlignment align = alignCenter;
    
    if( initDone && !inForceCenter ) {
        // transparent message
        setDrawColor( 1, 1, 1, 0.75 );

        // stick messages in corner
        messagePos.x -= viewWidth / 2;
        
        messagePos.x +=  20;
    

    
        messagePos.y += (viewWidth * viewHeightFraction) /  2;
    
        messagePos.y -= 32;

        align = alignLeft;
        }
    else {
        // fully opaque message
        setDrawColor( 1, 1, 1, 1 );

        // leave centered
        }
    

    int numLines;
    
    char **lines = split( inString, "\n", &numLines );
    
    for( int i=0; i<numLines; i++ ) {
        

        oldMainFont->drawString( lines[i], messagePos, align );
        messagePos.y -= 32;
        
        delete [] lines[i];
        }
    delete [] lines;
    }










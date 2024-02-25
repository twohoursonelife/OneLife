#include "ExistingAccountPage.h"

#include "message.h"
#include "buttonStyle.h"

#include "accountHmac.h"

#include "lifeTokens.h"
#include "fitnessScore.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/crc32.h"

#include "minorGems/crypto/hashes/sha1.h"


#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

static JenkinsRandomSource randSource;

int emailFieldLockedMode = 0;
int keyFieldLockedMode = 0;
int seedFieldLockedMode = 0;

bool emailUIElementsClicked = false;
bool keyUIElementsClicked = false;
bool seedUIElementsClicked = false;
bool twinUIElementsClicked = false;


extern Font *mainFont;


extern char gamePlayingBack;

extern char useSpawnSeed;
extern char useTargetFamily;
extern char *userEmail;
extern char *accountKey;
extern char *userTwinCode;
extern int userTwinCount;
bool useTwinCode = false;
int specifySpawn = 0; // 1 = targetFamily, 2 = spawnSeed, 0 = random

char *fitnessMessage = NULL;
char *tokenMessage = NULL;


extern SpriteHandle instructionsSprite;

extern char loginEditOverride;



ExistingAccountPage::ExistingAccountPage()
        : mBackground( "background.tga", 0.75f ),
          mGameLogo( "logo.tga", 1.0f, {-360, 256} ),

          // Left Pane Page 0
          mEmailField( mainFont, -360, 96, 10, false, 
                       translate( "username" ),
                       NULL,
                       // forbid only spaces and backslash and 
                       // single/double quotes 
                       // also pipe and colon, reserved as separators for spawn code and family name target
                       "\"' \\|:#" ),
          mEmailLockButton( mainFont, -108, 96, "!" ),
          mPasteEmailButton( mainFont, 0, 68, translate( "paste" ) ),
                       
          mKeyField( mainFont, -360, 0, 15, true,
                     translate( "accountKey" ),
                     // allow only ticket code characters
                     "23456789ABCDEFGHJKLMNPQRSTUVWXYZ-" ),
          mKeyLockButton( mainFont, -108, 0, "!" ),
          mPasteButton( mainFont, 0, -112, translate( "paste" ) ),
          
          mNextToGameTabButton( mainFont,  0, 0, "NEXT" ),          
          
          // Left Pane Page 1
          mFriendsButton( mainFont, 0, 0, "YES" ),
          mSoloButton( mainFont, 0, 0, "NO" ),
          mTwinCodeField( mainFont, -360, 16, 10, false, 
                                     translate( "twinCode" ),
                                     NULL,
                                     NULL ),
          mGenerateButton( mainFont, 0, 0, translate( "generate" ) ),
          mTwinCodeCopyButton( mainFont, 0, 0, translate( "copy" ) ),
          mTwinCodePasteButton( mainFont, 0, 0, translate( "paste" ) ),
          
          mSpecificButton( mainFont, 0, 0, "SPECIFIC" ),
          mRandomButton( mainFont, 0, 0, "RANDOM" ),
          mSpawnSeed( mainFont, -360, -176, 10, false, 
                                     "SPAWN CODE:",
                                     NULL,
                                     // forbid spaces and hash
                                     " #" ),
          mSpawnSeedLockButton( mainFont, -108, -176, "!" ),
          mTargetFamily( mainFont, -360, -176, 10, true, 
                                     "TARGET FAMILY NAME:",
                                     // allow only alphabets
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
          
          mBackToAccountTabButton( mainFont, 0, 0, "BACK" ),
          mLoginButton( mainFont, 0, 0, "PLAY" ),
          
          // Right Pane
          mSettingsButton( mainFont, 360, -80, translate( "settingsButton" ) ),
          mTutorialButton( mainFont, 360, 112, translate( "tutorial" ) ),
          mFamilyTreesButton( mainFont, 360, 16, translate( "familyTrees" ) ),
          mTechTreeButton( mainFont, 360, -176, translate( "techTree" ) ),
          mCancelButton( mainFont, 360, -272, translate( "quit" ) ),

          // Status
          mRetryButton( mainFont, -100, 198, translate( "retryButton" ) ),
          mRedetectButton( mainFont, 100, 198, translate( "redetectButton" ) ),
          mDisableCustomServerButton( mainFont, 0, 220, 
                                      translate( "disableCustomServer" ) ),
          
          // Not in use
          mGenesButton( mainFont, 522, 300, translate( "genesButton" ) ),
          mClearAccountButton( mainFont, -360, -80, "CLEAR" ),
          mReviewButton( mainFont, -400, -200, translate( "postReviewButton" ) ),
          mAtSignButton( mainFont, 252, 128, "@" ),
          mViewAccountButton( mainFont, 0, 64, translate( "view" ) ),

          
          mPageActiveStartTime( 0 ),
          mFramesCounted( 0 ),
          mFPSMeasureDone( false ),
          mHideAccount( false ) {
    
    

    mEmailField.setWidth( 360 );
    mEmailField.usePasteShortcut(true);
    mKeyField.setWidth( 360 );
    mKeyField.usePasteShortcut(true);
    mTwinCodeField.setWidth( 360 );
    mSpawnSeed.setWidth( 360 );
    mTargetFamily.setWidth( 360 );
    
    mFriendsButton.setSize( 175, 60 );
    mSoloButton.setSize( 175, 60 );
    mFriendsButton.setPosition( mEmailField.getLeftEdgeX() + ( mFriendsButton.getWidth()/2 ), 112 );
    mSoloButton.setPosition( mEmailField.getRightEdgeX() - ( mSoloButton.getWidth()/2 ), 112 );
    
    mGenerateButton.setPosition( 
        mTwinCodeField.getRightEdgeX() + 4 + mGenerateButton.getWidth() / 2, 
        mTwinCodeField.getPosition().y );
    mGenerateButton.setPadding( 8, 4 );


    mTwinCodeCopyButton.setPosition( 
        mGenerateButton.getPosition().x, 
        mTwinCodeField.getPosition().y - mainFont->getFontHeight() - 4 * 2 - 4 * 2 - 4 );
    mTwinCodePasteButton.setPosition( 
        mGenerateButton.getPosition().x, 
        mTwinCodeCopyButton.getPosition().y - mainFont->getFontHeight() - 4 * 2 - 4 * 2 - 4 );
    mTwinCodeCopyButton.setSize( mGenerateButton.getWidth(), mainFont->getFontHeight() + 4 * 3 );
    mTwinCodePasteButton.setSize( mGenerateButton.getWidth(), mainFont->getFontHeight() + 4 * 3 );

    
    
    
    mSpecificButton.setSize( 175, 60 );
    mRandomButton.setSize( 175, 60 );
    mSpecificButton.setPosition( mEmailField.getLeftEdgeX() + ( mSpecificButton.getWidth()/2 ), -80 );
    mRandomButton.setPosition( mEmailField.getRightEdgeX() - ( mRandomButton.getWidth()/2 ), -80 );
    
    mNextToGameTabButton.setSize( 360, 60 );
    mBackToAccountTabButton.setSize( 175, 60 );
    mLoginButton.setSize( 175, 60 );
    
    mNextToGameTabButton.setPosition( mEmailField.getRightEdgeX() - ( mNextToGameTabButton.getWidth()/2 ), -272 );
    mBackToAccountTabButton.setPosition( mEmailField.getLeftEdgeX() + ( mBackToAccountTabButton.getWidth()/2 ), -272 );
    mLoginButton.setPosition( mEmailField.getRightEdgeX() - ( mLoginButton.getWidth()/2 ), -272 );
    
    
    
    const char *choiceList[3] = { translate( "twins" ),
                                  translate( "triplets" ),
                                  translate( "quadruplets" ) };
    
    mPlayerCountRadioButtonSet = 
        new RadioButtonSet( mainFont, 0, 0,
                            3, choiceList,
                            true, 4 );
    
    
    mPlayerCountRadioButtonSet->setPosition(
        mGenerateButton.getPosition().x + 4 + 175 / 2 ,
        mGenerateButton.getPosition().y );
        
        
        
    const char *specifySpawnChoiceList[2] = { "SPECIFY FAMILY NAME",
                                              "USE SPAWN CODE" };
    
    mSeedOrFamilyButtonSet = 
        new RadioButtonSet( mainFont, 0, 0,
                            2, specifySpawnChoiceList,
                            true, 4 );
    
    
    mSeedOrFamilyButtonSet->setPosition(
        mSpawnSeedLockButton.getPosition().x + 4 + mSpawnSeedLockButton.getWidth() / 2 ,
        mSpawnSeedLockButton.getPosition().y + mainFont->getFontHeight() / 2 + 2 );
    
    

    if( userEmail != NULL && accountKey != NULL ) {
        mEmailField.setText( userEmail );
        mKeyField.setText( accountKey );
        }

    setButtonStyle( &mEmailLockButton );
    setButtonStyle( &mPasteEmailButton );
    setButtonStyle( &mKeyLockButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mNextToGameTabButton );
    
    setButtonStyle( &mFriendsButton );
    setButtonStyle( &mSoloButton );
    setButtonStyle( &mGenerateButton );
    setButtonStyle( &mTwinCodeCopyButton );
    setButtonStyle( &mTwinCodePasteButton );
    setButtonStyle( &mSpecificButton );
    setButtonStyle( &mRandomButton );
    setButtonStyle( &mSpawnSeedLockButton );
    setButtonStyle( &mBackToAccountTabButton );
    setButtonStyle( &mLoginButton );
    
    setButtonStyle( &mSettingsButton );
    setButtonStyle( &mTutorialButton );
    setButtonStyle( &mFamilyTreesButton );
    setButtonStyle( &mTechTreeButton );
    setButtonStyle( &mCancelButton );
    
    setButtonStyle( &mRetryButton );
    setButtonStyle( &mRedetectButton );
    setButtonStyle( &mDisableCustomServerButton );
    
    setButtonStyle( &mGenesButton );
    setButtonStyle( &mClearAccountButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mAtSignButton );
    setButtonStyle( &mViewAccountButton );

    
    mFields[0] = &mEmailField;
    mFields[1] = &mKeyField;

    
    addComponent( &mBackground );
    addComponent( &mGameLogo );
                                     
    
    // Adding components below in reverse order so cursor tip is drawn on top
    
    
    // Not in use
    addComponent( &mGenesButton );
    addComponent( &mClearAccountButton );
    addComponent( &mReviewButton );
    addComponent( &mAtSignButton );
    addComponent( &mViewAccountButton );
    
    // Right Pane
    addComponent( &mCancelButton );
    addComponent( &mTechTreeButton );
    addComponent( &mFamilyTreesButton );
    addComponent( &mTutorialButton );
    addComponent( &mSettingsButton );
    
    // Left Pane Page 1
    addComponent( &mLoginButton );
    addComponent( &mBackToAccountTabButton );
    
    addComponent( mSeedOrFamilyButtonSet );
    addComponent( &mTargetFamily );    
    addComponent( &mSpawnSeedLockButton );
    addComponent( &mSpawnSeed );
    addComponent( mPlayerCountRadioButtonSet );
    addComponent( &mTwinCodePasteButton );
    addComponent( &mTwinCodeCopyButton );
    addComponent( &mGenerateButton );
    addComponent( &mRandomButton );
    addComponent( &mSpecificButton );
    addComponent( &mTwinCodeField );
    addComponent( &mSoloButton );
    addComponent( &mFriendsButton );
    
    // Left Pane Page 0
    addComponent( &mNextToGameTabButton );
    
    addComponent( &mPasteButton );
    addComponent( &mKeyLockButton );
    addComponent( &mKeyField );
    
    addComponent( &mPasteEmailButton );
    addComponent( &mEmailLockButton );
    addComponent( &mEmailField );
    
    // Status
    addComponent( &mRetryButton );
    addComponent( &mRedetectButton );
    addComponent( &mDisableCustomServerButton );

    

    mSpawnSeed.useClearButton( true );

    
    // this section have all buttons with the same width
    mTutorialButton.setSize( 175, 60 );
    mSettingsButton.setSize( 175, 60 );
    mGenesButton.setSize( 175, 60 );
    mFamilyTreesButton.setSize( 175, 60 );
    mTechTreeButton.setSize( 175, 60 );
    mCancelButton.setSize( 175, 60 );
    
    mEmailField.setLabelTop( true );
    mKeyField.setLabelTop( true );
    mSpawnSeed.setLabelTop( true );
    mTargetFamily.setLabelTop( true );
    mTwinCodeField.setLabelTop( true );
    
    
    
    mEmailLockButton.addActionListener( this );
    mPasteEmailButton.addActionListener( this );
    mKeyLockButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mNextToGameTabButton.addActionListener( this );
    
    mFriendsButton.addActionListener( this );
    mSoloButton.addActionListener( this );
    mTwinCodeField.addActionListener( this );
    mGenerateButton.addActionListener( this );
    mTwinCodeCopyButton.addActionListener( this );
    mTwinCodePasteButton.addActionListener( this );
    mPlayerCountRadioButtonSet->addActionListener( this );
    
    mSpecificButton.addActionListener( this );
    mRandomButton.addActionListener( this );
    mSpawnSeed.addActionListener( this );
    mSpawnSeedLockButton.addActionListener( this );
    mTargetFamily.addActionListener( this );
    mSeedOrFamilyButtonSet->addActionListener( this );
    mBackToAccountTabButton.addActionListener( this );
    mLoginButton.addActionListener( this );
    
    mSettingsButton.addActionListener( this );
    mTutorialButton.addActionListener( this );
    mFamilyTreesButton.addActionListener( this );
    mTechTreeButton.addActionListener( this );
    mCancelButton.addActionListener( this );
    
    mRetryButton.addActionListener( this );
    mRedetectButton.addActionListener( this );
    mDisableCustomServerButton.addActionListener( this );
    
    mGenesButton.addActionListener( this );
    mClearAccountButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    mAtSignButton.addActionListener( this );
    mViewAccountButton.addActionListener( this );
    
    
    
    mRetryButton.setVisible( false );
    mRedetectButton.setVisible( false );
    mDisableCustomServerButton.setVisible( false );
    

    

    mEmailField.setCursorTip( "GET YOUR ACCOUNT FROM THE DISCORD BOT" );
    mKeyField.setCursorTip( "GET YOUR ACCOUNT FROM THE DISCORD BOT" );
    
    mNextToGameTabButton.setCursorTip( "NEXT PAGE" );
    
    mFriendsButton.setCursorTip( translate( "friendsTip" ) );
    mSoloButton.setCursorTip( translate( "friendsTip" ) );
    mTwinCodeField.setCursorTip( translate( "twinTip" ) );
    mGenerateButton.setCursorTip( "GENERATE A RANDOM TWIN CODE" );
    mPlayerCountRadioButtonSet->setCursorTip( "CHOOSE HOW MANY WILL JOIN" );
    
    mSpecificButton.setCursorTip( "BE BORN INTO SPECIFIC FAMILY OR AT A FIXED SPAWN POINT" );
    mRandomButton.setCursorTip( "BE BORN INTO A RANDOM FAMILY" );
    mSpawnSeed.setCursorTip( "SPAWN CODE CAN BE ANY STRING OF TEXT OR NUMBERS, CASE SENSITIVE" );
    mTargetFamily.setCursorTip( "ENTER THE NAME OF THE FAMILY YOU WANT TO JOIN" );
    mSeedOrFamilyButtonSet->setCursorTip( "SPECIFY THE FAMILY YOU WANT TO JOIN, OR MAKE A FIXED SPAWN POINT WITH A SPAWN CODE" );
    
    mBackToAccountTabButton.setCursorTip( "BACK TO ACCOUNT PAGE" );
    mLoginButton.setCursorTip( "BE BORN INTO THE WORLD" );
    
    mSettingsButton.setCursorTip( "CHANGE GAME SETTINGS" );
    mTutorialButton.setCursorTip( "START THE TUTORIAL" );
    mFamilyTreesButton.setCursorTip( translate( "familyTreesTip" ) );
    mTechTreeButton.setCursorTip( translate( "techTreeTip" ) );
    mCancelButton.setCursorTip( "CLOSE THE GAME" );
    
    mGenesButton.setCursorTip( translate( "genesTip" ) );
    mClearAccountButton.setCursorTip( translate( "clearAccountTip" ) );
    mAtSignButton.setCursorTip( translate( "atSignTip" ) );
    
    
    

    
    
    char *seed = 
        SettingsManager::getSettingContents( "spawnSeed", "" );

    mSpawnSeed.setList( seed );
        
    delete [] seed;
    
    
    FILE *f = fopen( "wordList.txt", "r" );
    
    if( f != NULL ) {
    
        int numRead = 1;
        
        char buff[100];
        
        while( numRead == 1 ) {
            numRead = fscanf( f, "%99s", buff );
            
            if( numRead == 1 ) {
                mWordList.push_back( stringDuplicate( buff ) );
                }
            }
        fclose( f );
        }
    
    if( mWordList.size() < 20 ) {
        mGenerateButton.setActive( false );
        }
        
    if( userEmail != NULL ) {    
        unsigned int timeSeed = 
            (unsigned int)fmod( game_getCurrentTime(), UINT_MAX );
        unsigned int emailSeed =
            crc32( (unsigned char *)userEmail, strlen( userEmail ) );
        
        mRandSource.reseed( timeSeed + emailSeed );
        }
        
    char oldSet = false;
    char *oldCode = SettingsManager::getSettingContents( "twinCode", "" );
    
    if( oldCode != NULL ) {
        if( strcmp( oldCode, "" ) != 0 ) {
            mTwinCodeField.setText( oldCode );
            oldSet = true;
            actionPerformed( &mTwinCodeField );
            }
        delete [] oldCode;
        }

    if( !oldSet ) {
        // generate first one automatically
        actionPerformed( &mGenerateButton );
        }
    

    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    

    // to dodge quit message
    setTipPosition( true );
    }

          
        
ExistingAccountPage::~ExistingAccountPage() {
    delete mPlayerCountRadioButtonSet;
    delete mSeedOrFamilyButtonSet;

    mWordList.deallocateStringElements();
    
    if( fitnessMessage != NULL ) delete [] fitnessMessage;
    }



void ExistingAccountPage::clearFields() {
    mEmailField.setText( "" );
    mKeyField.setText( "" );
    }



void ExistingAccountPage::showReviewButton( char inShow ) {
    mReviewButton.setVisible( inShow );
    }



void ExistingAccountPage::showDisableCustomServerButton( char inShow ) {
    mDisableCustomServerButton.setVisible( inShow );
    }


int leftPanePage = 0;
bool tutorialDone = true;
bool useSteamUpdate = false;
bool disableCredentialLocks = false;

void ExistingAccountPage::updatefieldsAndLockButtons() {
    
    mEmailLockButton.setLabelText( emailFieldLockedMode == 2 ? "OK" : "!" );
    mEmailLockButton.setCursorTip( emailFieldLockedMode == 2 ? "LOCK FIELD" : "UNLOCK FIELD" );
    mEmailLockButton.setPosition( 
        mEmailField.getRightEdgeX() + mEmailLockButton.getWidth() / 2 + 4, 
        mEmailField.getPosition().y );
    mEmailLockButton.setPadding( 8, 4 );
    mEmailLockButton.setVisible( mEmailField.isVisible() && emailFieldLockedMode != 0 && !disableCredentialLocks );
    mEmailField.setContentsHidden( emailFieldLockedMode != 2 && !disableCredentialLocks );
    mEmailField.setIgnoreEvents( emailFieldLockedMode != 2 && !disableCredentialLocks );
    
    mPasteEmailButton.setPosition( 
        mEmailField.getRightEdgeX() + 4 + 
        (mEmailLockButton.isVisible() ? mEmailLockButton.getWidth() : 0) + 
        12 + mPasteEmailButton.getWidth() / 2, 
        mEmailField.getPosition().y );
    mPasteEmailButton.setPadding( 8, 4 );
    mPasteEmailButton.setVisible( mEmailField.isVisible() && isClipboardSupported() && (emailFieldLockedMode == 2 || disableCredentialLocks) );

    mAtSignButton.setPosition( 
        mEmailField.getRightEdgeX() + 4 + 
        (mEmailLockButton.isVisible() ? mEmailLockButton.getWidth() : 0) + 4 + 
        mPasteEmailButton.getWidth() + 
        16 + mAtSignButton.getWidth() / 2, 
        mEmailField.getPosition().y );
    mAtSignButton.setPadding( 8, 4 );
    mAtSignButton.setVisible( mEmailField.isVisible() && isClipboardSupported() && (emailFieldLockedMode == 2 || disableCredentialLocks) && useSteamUpdate );
    
    mKeyLockButton.setLabelText( keyFieldLockedMode == 2 ? "OK" : "!" );
    mKeyLockButton.setCursorTip( keyFieldLockedMode == 2 ? "LOCK FIELD" : "UNLOCK FIELD" );
    mKeyLockButton.setPosition( 
        mKeyField.getRightEdgeX() + mKeyLockButton.getWidth() / 2 + 4, 
        mKeyField.getPosition().y );
    mKeyLockButton.setPadding( 8, 4 );
    mKeyLockButton.setVisible( mKeyField.isVisible() && keyFieldLockedMode != 0 && !disableCredentialLocks );
    mKeyField.setContentsHidden( keyFieldLockedMode != 2 && !disableCredentialLocks );
    mKeyField.setIgnoreEvents( keyFieldLockedMode != 2 && !disableCredentialLocks );
    
    mPasteButton.setPosition( 
        mKeyField.getRightEdgeX() + 4 + 
        (mKeyLockButton.isVisible() ? mKeyLockButton.getWidth() : 0) + 
        12 + mPasteButton.getWidth() / 2, 
        mKeyField.getPosition().y );
    mPasteButton.setPadding( 8, 4 );
    mPasteButton.setVisible( mKeyField.isVisible() && isClipboardSupported() && (keyFieldLockedMode == 2 || disableCredentialLocks) );
    
    mSpawnSeedLockButton.setLabelText( seedFieldLockedMode == 2 ? "OK" : "!" );
    mSpawnSeedLockButton.setCursorTip( seedFieldLockedMode == 2 ? "LOCK FIELD" : "UNLOCK FIELD" );
    mSpawnSeedLockButton.setPosition( 
        mSpawnSeed.getRightEdgeX() + mSpawnSeedLockButton.getWidth() / 2 + 4, 
        mSpawnSeed.getPosition().y );
    mSpawnSeedLockButton.setPadding( 8, 4 );
    mSpawnSeedLockButton.setVisible( mSpawnSeed.isVisible() && seedFieldLockedMode != 0 );
    mSpawnSeed.setContentsHidden( seedFieldLockedMode != 2 );
    mSpawnSeed.setIgnoreEvents( seedFieldLockedMode != 2 );
    
    }
    
    
void ExistingAccountPage::updateLeftPane() {
    
    if( !tutorialDone ) {
        leftPanePage = 0;
        
        mTutorialButton.setLabelText( "PLAY" );
        mTutorialButton.setCursorTip( "START THE GAME" );
        mTutorialButton.setSize( 360, 60 );
        mTutorialButton.setPosition( mEmailField.getRightEdgeX() - ( mNextToGameTabButton.getWidth()/2 ), -272 );
        }
    else {
        mTutorialButton.setLabelText( translate( "tutorial" ) );
        mTutorialButton.setCursorTip( "START THE TUTORIAL" );
        mTutorialButton.setSize( 175, 60 );
        // setPosition in this case is done elsewhere with the right pane buttons
        }
    
    
    mEmailField.setVisible( leftPanePage == 0 );
    mKeyField.setVisible( leftPanePage == 0 );
    
    mNextToGameTabButton.setVisible( leftPanePage == 0 && tutorialDone );
    
    mFriendsButton.setVisible( leftPanePage == 1 );
    mFriendsButton.setActive( !useTwinCode );
    mSoloButton.setVisible( leftPanePage == 1 );
    mSoloButton.setActive( useTwinCode );
    
    mTwinCodeField.setVisible( leftPanePage == 1 && useTwinCode );
    
    mSpecificButton.setVisible( leftPanePage == 1 && !useSteamUpdate );
    mSpecificButton.setActive( !specifySpawn );
    mRandomButton.setVisible( leftPanePage == 1 && !useSteamUpdate );
    mRandomButton.setActive( specifySpawn );
    
    mSpawnSeed.setVisible( leftPanePage == 1 && specifySpawn == 2 && !useSteamUpdate );
    mTargetFamily.setVisible( leftPanePage == 1 && specifySpawn == 1 && !useSteamUpdate );
    mSeedOrFamilyButtonSet->setVisible( leftPanePage == 1 && specifySpawn != 0 && !useSteamUpdate );
    
    mBackToAccountTabButton.setVisible( leftPanePage == 1 );
    mLoginButton.setVisible( leftPanePage == 1 && tutorialDone );
        
}


void ExistingAccountPage::makeActive( char inFresh ) {
    
    
    int pastSuccess = SettingsManager::getIntSetting( "loginSuccess", 0 );

    char *emailText = mEmailField.getText();
    char *keyText = mKeyField.getText();

    // don't hide field contents unless there is something to hide
    if( ! pastSuccess || 
        ( strcmp( emailText, "" ) == 0 
          &&
          strcmp( keyText, "" ) == 0 ) ) {
        disableCredentialLocks = true;
        leftPanePage = 0;
        }
    else {
        disableCredentialLocks = false;
        leftPanePage = 1;
        }
    
    delete [] emailText;
    delete [] keyText;
    
    
    tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 ) != 0;
    
    useSteamUpdate = SettingsManager::getIntSetting( "useSteamUpdate", 0 ) != 0;
    
    
    if( !mFPSMeasureDone || mRetryButton.isVisible() ) {
        
        updateLeftPane();
        
        emailFieldLockedMode = 0;
        keyFieldLockedMode = 0;
        seedFieldLockedMode = 0;
        mEmailField.unfocus();
        mKeyField.unfocus();
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        
        mBackground.setVisible( false );
        mGameLogo.setVisible( false );
        
        mEmailField.setVisible( false );
        mPasteEmailButton.setVisible( false );
        mAtSignButton.setVisible( false );
        
        mKeyField.setVisible( false );
        mPasteButton.setVisible( false );
        
        mNextToGameTabButton.setVisible( false );
        
        mFriendsButton.setVisible( false );
        mSoloButton.setVisible( false );
        mTwinCodeField.setVisible( false );
        mGenerateButton.setVisible( false );
        mTwinCodeCopyButton.setVisible( false );
        mTwinCodePasteButton.setVisible( false );
        mPlayerCountRadioButtonSet->setVisible( false );
        mSpecificButton.setVisible( false );
        mRandomButton.setVisible( false );
        mSpawnSeed.setVisible( false );
        mTargetFamily.setVisible( false );
        mSeedOrFamilyButtonSet->setVisible( false );
        mBackToAccountTabButton.setVisible( false );
        mLoginButton.setVisible( false );
        
        mSettingsButton.setVisible( false );
        mTutorialButton.setVisible( false );
        mFamilyTreesButton.setVisible( false );
        mTechTreeButton.setVisible( false );
        mCancelButton.setVisible( false );
        
        mGenesButton.setVisible( false );

        }
    else {
        mBackground.setVisible( true );
        mGameLogo.setVisible( true );
        
        mSettingsButton.setVisible( true );
        mTutorialButton.setVisible( true );
        
        char *lineageServerURL = SettingsManager::getStringSetting( "lineageServerURL", "" );
        char show = ( strcmp( lineageServerURL, "" ) != 0 ) && isURLLaunchSupported();
        mFamilyTreesButton.setVisible( show );
        delete [] lineageServerURL;
        
        char *techTreeURL = SettingsManager::getStringSetting( "techTreeURL", "" );
        mTechTreeButton.setVisible( strcmp( techTreeURL, "" ) != 0 );
        delete [] techTreeURL;
        
        mCancelButton.setVisible( true );
        
        updateLeftPane();
        
        emailFieldLockedMode = 0;
        keyFieldLockedMode = 0;
        seedFieldLockedMode = 0;
        mEmailField.unfocus();
        mKeyField.unfocus();
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        
        }


    char *seed = 
        SettingsManager::getSettingContents( "spawnSeed", "" );

    mSpawnSeed.setList( seed );
        
    delete [] seed;
    
    specifySpawn = SettingsManager::getIntSetting( "specifySpawn", 0 );
    if( specifySpawn == 1 ) {
        useTargetFamily = true;
        useSpawnSeed = false;
        mSeedOrFamilyButtonSet->setSelectedItem( 0 );
        }
    else if( specifySpawn == 2 ) {
        useTargetFamily = false;
        useSpawnSeed = true;
        mSeedOrFamilyButtonSet->setSelectedItem( 1 );
        }
    else {
        specifySpawn = 0;
        useTargetFamily = false;
        useSpawnSeed = false;
        mSeedOrFamilyButtonSet->setSelectedItem( 0 );
        }
    



    mFramesCounted = 0;
    mPageActiveStartTime = game_getCurrentTime();    
    
    // don't re-measure every time we return to this screen
    // it slows the player down too much
    // re-measure only at first-startup
    //mFPSMeasureDone = false;
    
    
    int skipFPSMeasure = SettingsManager::getIntSetting( "skipFPSMeasure", 0 );
    
    if( skipFPSMeasure ) {
        mFPSMeasureDone = true;
        mRetryButton.setVisible( false );
        mRedetectButton.setVisible( false );
        }

    if( mFPSMeasureDone && ! mRetryButton.isVisible() ) {
        // skipping measure OR we are returning to this page later
        // and not measuring again
        triggerLifeTokenUpdate();
        triggerFitnessScoreUpdate();
        }
    else if( mFPSMeasureDone && mRetryButton.isVisible() ) {
        // left screen after failing
        // need to measure again after returning
        mRetryButton.setVisible( false );
        mRedetectButton.setVisible( false );
        mFPSMeasureDone = false;
        }
    


    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    else {
        mReviewButton.setLabelText( translate( "postReviewButton" ) );
        }


    if( useSteamUpdate ) {
        
        mEmailField.setLabelText( "STEAM ID:" );
        
        // mBackground.setImage( "instructions.tga" );
        
        // no review button on Steam
        mReviewButton.setVisible( false );
        
        mClearAccountButton.setVisible( false );
        mViewAccountButton.setVisible( false );
        
        setDarkButtonStyle( &mEmailLockButton );
        setDarkButtonStyle( &mPasteEmailButton );
        setDarkButtonStyle( &mKeyLockButton );
        setDarkButtonStyle( &mPasteButton );
        setDarkButtonStyle( &mNextToGameTabButton );
        
        setDarkButtonStyle( &mFriendsButton );
        setDarkButtonStyle( &mSoloButton );
        setDarkButtonStyle( &mGenerateButton );
        setDarkButtonStyle( &mTwinCodeCopyButton );
        setDarkButtonStyle( &mTwinCodePasteButton );
        setDarkButtonStyle( &mSpecificButton );
        setDarkButtonStyle( &mRandomButton );
        setDarkButtonStyle( &mSpawnSeedLockButton );
        setDarkButtonStyle( &mBackToAccountTabButton );
        setDarkButtonStyle( &mLoginButton );
        
        setDarkButtonStyle( &mSettingsButton );
        setDarkButtonStyle( &mTutorialButton );
        setDarkButtonStyle( &mFamilyTreesButton );
        setDarkButtonStyle( &mTechTreeButton );
        setDarkButtonStyle( &mCancelButton );
        
        setDarkButtonStyle( &mRetryButton );
        setDarkButtonStyle( &mRedetectButton );
        setDarkButtonStyle( &mDisableCustomServerButton );
        
        setDarkButtonStyle( &mGenesButton );
        setDarkButtonStyle( &mClearAccountButton );
        setDarkButtonStyle( &mReviewButton );
        setDarkButtonStyle( &mAtSignButton );
        setDarkButtonStyle( &mViewAccountButton );
        
        }
    else {
        // Not in use
        mClearAccountButton.setVisible( false );
        mReviewButton.setVisible( false );
        mAtSignButton.setVisible( false );
        mViewAccountButton.setVisible( false );
        
        mHideAccount = false;
        }
    
    }



void ExistingAccountPage::makeNotActive() {
    emailFieldLockedMode = 0;
    keyFieldLockedMode = 0;
    seedFieldLockedMode = 0;
    mEmailField.unfocus();
    mKeyField.unfocus();
    mSpawnSeed.unfocus();
    updatefieldsAndLockButtons();
    }



void ExistingAccountPage::step() {
    
    mGenerateButton.setVisible( leftPanePage == 1 && useTwinCode == 1 && mTwinCodeField.isFocused() );
    mTwinCodeCopyButton.setVisible( leftPanePage == 1 && useTwinCode == 1 && mTwinCodeField.isFocused() );
    mTwinCodePasteButton.setVisible( leftPanePage == 1 && useTwinCode == 1 && mTwinCodeField.isFocused() );
    mPlayerCountRadioButtonSet->setVisible( leftPanePage == 1 && useTwinCode == 1 && mTwinCodeField.isFocused() );
    mSeedOrFamilyButtonSet->setVisible( mSpecificButton.isVisible() && leftPanePage == 1 && specifySpawn != 0 );
    
    int blockClicks = false;
    if ( mSpawnSeed.isFocused() ) { blockClicks = true; }
    
    mLoginButton.setIgnoreEvents( blockClicks );
    mBackToAccountTabButton.setIgnoreEvents( blockClicks );
    
    }
    
    
static bool insideTextField( float inX, float inY, DropdownList *targetField ) {
    doublePair pos = targetField->getPosition();
    return fabs( inX - pos.x ) < targetField->getWidth() / 2 &&
        fabs( inY - pos.y ) < 48 / 2;
    }
    
static bool insideTextField( float inX, float inY, TextField *targetField ) {
    doublePair pos = targetField->getPosition();
    return fabs( inX - pos.x ) < targetField->getWidth() / 2 &&
        fabs( inY - pos.y ) < 48 / 2;
    }


void ExistingAccountPage::pointerUp( float inX, float inY ) {
    
    if( mEmailField.isVisible() && insideTextField( inX, inY, &mEmailField ) ) {
        if( emailFieldLockedMode == 0 ) {
            keyFieldLockedMode = 0;
            seedFieldLockedMode = 0;
            emailFieldLockedMode = 1;
            mKeyField.unfocus();
            mSpawnSeed.unfocus();
            updatefieldsAndLockButtons();
            }
        }
    if( mKeyField.isVisible() && insideTextField( inX, inY, &mKeyField ) ) {
        if( keyFieldLockedMode == 0 ) {
            emailFieldLockedMode = 0;
            seedFieldLockedMode = 0;
            keyFieldLockedMode = 1;
            mEmailField.unfocus();
            mSpawnSeed.unfocus();
            updatefieldsAndLockButtons();
            }
        }
    if( mSpawnSeed.isVisible() && insideTextField( inX, inY, &mSpawnSeed ) ) {
        if( seedFieldLockedMode == 0 ) {
            emailFieldLockedMode = 0;
            keyFieldLockedMode = 0;
            seedFieldLockedMode = 1;
            mEmailField.unfocus();
            mKeyField.unfocus();
            updatefieldsAndLockButtons();
            }
        }
        
    if( mTwinCodeField.isVisible() && !insideTextField( inX, inY, &mTwinCodeField ) ) {
        if( !twinUIElementsClicked ) {
            mTwinCodeField.unfocus();
            }
        twinUIElementsClicked = false;
        }
        
    if( mEmailField.isVisible() && !insideTextField( inX, inY, &mEmailField ) ) {
        if( !emailUIElementsClicked ) {
            emailFieldLockedMode = 0;
            mEmailField.unfocus();
            updatefieldsAndLockButtons();
            }
        emailUIElementsClicked = false;
        }
    if( mKeyField.isVisible() && !insideTextField( inX, inY, &mKeyField ) ) {
        if( !keyUIElementsClicked ) {
            keyFieldLockedMode = 0;
            mKeyField.unfocus();
            updatefieldsAndLockButtons();
            }
        keyUIElementsClicked = false;
        }
    if( mSpawnSeed.isVisible() && !insideTextField( inX, inY, &mSpawnSeed ) ) {
        if( !seedUIElementsClicked ) {
            seedFieldLockedMode = 0;
            mSpawnSeed.unfocus();
            updatefieldsAndLockButtons();
            }
        seedUIElementsClicked = false;
        }
    
    }


void ExistingAccountPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget != &mSpawnSeed ) {
        //save seed setting on any action performed
        //except seed field itself cause we're still editing it
        char *seedList = mSpawnSeed.getAndUpdateList();
        SettingsManager::setSetting( "spawnSeed", seedList );
        delete [] seedList;
        }
    if( inTarget != &mTargetFamily ) {
        char *text = mTargetFamily.getText();
        char *targetFamily = trimWhitespace( text );
        SettingsManager::setSetting( "targetFamily", targetFamily );
        delete [] text;
        delete [] targetFamily;
        }

    if( inTarget == &mLoginButton ) {
        
        if( userTwinCode != NULL ) {
            delete [] userTwinCode;
            userTwinCode = NULL;
            }
        
        if( useTwinCode ) {
            
            char *text = mTwinCodeField.getText();
            
            userTwinCode = trimWhitespace( text );
            delete [] text;

            SettingsManager::setSetting( "twinCode", userTwinCode );

            userTwinCount = mPlayerCountRadioButtonSet->getSelectedItem() + 2;
            }
        
        processLogin( true, "done" );
        }
    else if( inTarget == &mSpecificButton ) {
        
        // processLogin( true, "done" );
        mSpecificButton.setActive( false );
        mRandomButton.setActive( true );
        // mSpawnSeed.setVisible( true );
            
        specifySpawn = SettingsManager::getIntSetting( "specifySpawn", 0 );

        if( specifySpawn == 2 ) {
            useTargetFamily = false;
            useSpawnSeed = true;
            mSeedOrFamilyButtonSet->setSelectedItem( 1 );
            
            mSpawnSeed.setVisible( true );
            }
        else {
            specifySpawn = 1;
            SettingsManager::setSetting( "specifySpawn", 1 );
            useTargetFamily = true;
            useSpawnSeed = false;
            mSeedOrFamilyButtonSet->setSelectedItem( 0 );
            
            mTargetFamily.setVisible( true );
            }
        
        seedUIElementsClicked = true;
        }
    else if( inTarget == &mRandomButton ) {

        mSpecificButton.setActive( true );
        mRandomButton.setActive( false );

        specifySpawn = 0;
        SettingsManager::setSetting( "specifySpawn", 0 );        
        useTargetFamily = false;
        useSpawnSeed = false;
        
        seedFieldLockedMode = 0;
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        
        mSpawnSeed.setVisible( false );
        
        mTargetFamily.unfocus();
        mTargetFamily.setVisible( false );
        }
    else if( inTarget == mSeedOrFamilyButtonSet ) {
        if( mSeedOrFamilyButtonSet->getSelectedItem() == 0 ) {
            specifySpawn = 1;
            SettingsManager::setSetting( "specifySpawn", 1 );
            useTargetFamily = true;
            useSpawnSeed = false;
            
            seedFieldLockedMode = 0;
            mSpawnSeed.unfocus();
            updatefieldsAndLockButtons();
            
            mSpawnSeed.setVisible( false );
            
            mTargetFamily.setVisible( true );
            }
        else {
            specifySpawn = 2;
            SettingsManager::setSetting( "specifySpawn", 2 );
            useTargetFamily = false;
            useSpawnSeed = true;
            
            mSpawnSeed.setVisible( true );
            
            mTargetFamily.unfocus();
            mTargetFamily.setVisible( false );
            }
        }
    else if( inTarget == &mTutorialButton ) {
        processLogin( true, "tutorial" );
        }
    else if( inTarget == &mClearAccountButton ) {
        SettingsManager::setSetting( "email", "" );
        SettingsManager::setSetting( "accountKey", "" );
        SettingsManager::setSetting( "loginSuccess", 0 );
        SettingsManager::setSetting( "twinCode", "" );

        mEmailField.setText( "" );
        mKeyField.setText( "" );
        
        if( userEmail != NULL ) {
            delete [] userEmail;
            }
        userEmail = mEmailField.getText();
        
        if( accountKey != NULL ) {
            delete [] accountKey;
            }
        accountKey = mKeyField.getText();
        
        mEmailField.setContentsHidden( false );
        mKeyField.setContentsHidden( false );
        }
    else if( inTarget == &mFriendsButton ) {
        // processLogin( true, "friends" );
        useTwinCode = true;
        mFriendsButton.setActive( false );
        mSoloButton.setActive( true );
        mTwinCodeField.setVisible( true );
        }
    else if( inTarget == &mSoloButton ) {
        useTwinCode = false;
        mSoloButton.setActive( false );
        mFriendsButton.setActive( true );
        mTwinCodeField.setVisible( false );
        }
    else if( inTarget == &mTwinCodeField ) {
        char *text = mTwinCodeField.getText();
        
        char *trimText = trimWhitespace( text );

        if( strcmp( trimText, "" ) == 0 ) {
            mLoginButton.setVisible( false );
            }
        else {
            mLoginButton.setVisible( true );
            }
        delete [] text;
        delete [] trimText;
        
        twinUIElementsClicked = true;
        }
    else if( inTarget == &mGenerateButton ) {
        
        char *pickedWord[3];
        
        for( int i=0; i<3; i++ ) {
            pickedWord[i] = 
                mWordList.getElementDirect( mRandSource.getRandomBoundedInt( 
                                                0, mWordList.size() - 1 ) );
            }
        char *code = autoSprintf( "%s %s %s",
                                  pickedWord[0],
                                  pickedWord[1],
                                  pickedWord[2] );
        
        mTwinCodeField.setText( code );
        actionPerformed( &mTwinCodeField );
        delete [] code;
        
        twinUIElementsClicked = true;
        }
    else if( inTarget == &mTwinCodeCopyButton ) {
        char *text = mTwinCodeField.getText();
        setClipboardText( text );
        delete [] text;
        
        twinUIElementsClicked = true;
        }
    else if( inTarget == &mTwinCodePasteButton ) {
        char *text = getClipboardText();

        mTwinCodeField.setText( text );
        actionPerformed( &mTwinCodeField );

        delete [] text;
        
        twinUIElementsClicked = true;
        }
    else if( inTarget == mPlayerCountRadioButtonSet ) {
        twinUIElementsClicked = true;
        }
    else if( inTarget == &mSpawnSeed ) {
        seedUIElementsClicked = true;
        }
    else if( inTarget == &mGenesButton ) {
        setSignal( "genes" );
        }
    else if( inTarget == &mFamilyTreesButton ) {
        char *url = SettingsManager::getStringSetting( "lineageServerURL", "" );

        if( strcmp( url, "" ) != 0 ) {
            char *email = mEmailField.getText();
            
            char *string_to_hash = 
                autoSprintf( "%d", 
                             randSource.getRandomBoundedInt( 0, 2000000000 ) );
             
            char *pureKey = getPureAccountKey();
            
            char *ticket_hash = hmac_sha1( pureKey, string_to_hash );

            delete [] pureKey;

            char *lowerEmail = stringToLowerCase( email );

            char *emailHash = computeSHA1Digest( lowerEmail );

            delete [] lowerEmail;

            char *fullURL = autoSprintf( "%s?action=front_page&email_sha1=%s"
                                         "&ticket_hash=%s"
                                         "&string_to_hash=%s",
                                         url, emailHash, 
                                         ticket_hash, string_to_hash );
            delete [] email;
            delete [] emailHash;
            delete [] ticket_hash;
            delete [] string_to_hash;
            
            launchURL( fullURL );
            delete [] fullURL;
            }
        delete [] url;
        }
    else if( inTarget == &mTechTreeButton ) {
        char *url = SettingsManager::getStringSetting( "techTreeURL", "" );

        if( strcmp( url, "" ) != 0 ) {
            launchURL( url );
            }
        delete [] url;
        }
    else if( inTarget == &mViewAccountButton ) {
        if( mHideAccount ) {
            mViewAccountButton.setLabelText( translate( "hide" ) );
            }
        else {
            mViewAccountButton.setLabelText( translate( "view" ) );
            }
        mHideAccount = ! mHideAccount;
        }
    else if( inTarget == &mCancelButton ) {
        setSignal( "quit" );
        }
    else if( inTarget == &mSettingsButton ) {
        setSignal( "settings" );
        }
    else if( inTarget == &mReviewButton ) {
        if( userEmail != NULL ) {
            delete [] userEmail;
            }
        userEmail = mEmailField.getText();
        
        if( accountKey != NULL ) {
            delete [] accountKey;
            }
        accountKey = mKeyField.getText();
        
        setSignal( "review" );
        }
    else if( inTarget == &mAtSignButton ) {
        mEmailField.insertCharacter( '@' );
        }
    else if( inTarget == &mPasteButton ) {
        char *clipboardText = getClipboardText();
        
        mKeyField.setText( clipboardText );
    
        delete [] clipboardText;
        
        keyUIElementsClicked = true;
        }
    else if( inTarget == &mPasteEmailButton ) {
        char *clipboardText = getClipboardText();
        
        mEmailField.setText( clipboardText );
    
        delete [] clipboardText;
        
        emailUIElementsClicked = true;
        }
    else if( inTarget == &mRetryButton ) {
        mFPSMeasureDone = false;
        mPageActiveStartTime = game_getCurrentTime();
        mFramesCounted = 0;
        
        mRetryButton.setVisible( false );
        mRedetectButton.setVisible( false );
        
        setStatus( NULL, false );
        }
    else if( inTarget == &mRedetectButton ) {
        SettingsManager::setSetting( "targetFrameRate", -1 );
        SettingsManager::setSetting( "countingOnVsync", -1 );
        
        char relaunched = relaunchGame();
        
        if( !relaunched ) {
            printf( "Relaunch failed\n" );
            setSignal( "relaunchFailed" );
            }
        else {
            printf( "Relaunched... but did not exit?\n" );
            setSignal( "relaunchFailed" );
            }
        }
    else if( inTarget == &mDisableCustomServerButton ) {
        SettingsManager::setSetting( "useCustomServer", 0 );
        mDisableCustomServerButton.setVisible( false );
        processLogin( true, "done" );
        }
    else if( inTarget == &mEmailLockButton ) {
        emailFieldLockedMode = (emailFieldLockedMode + 1) % 3;
        mEmailField.unfocus();
        updatefieldsAndLockButtons();
        
        emailUIElementsClicked = true;
        }
    else if( inTarget == &mKeyLockButton ) {
        keyFieldLockedMode = (keyFieldLockedMode + 1) % 3;
        mKeyField.unfocus();
        updatefieldsAndLockButtons();
        
        keyUIElementsClicked = true;
        }
    else if( inTarget == &mSpawnSeedLockButton ) {
        seedFieldLockedMode = (seedFieldLockedMode + 1) % 3;
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        
        seedUIElementsClicked = true;
        }
        
    else if( inTarget == &mNextToGameTabButton ) {        
        leftPanePage = 1;
        updateLeftPane();
        
        emailFieldLockedMode = 0;
        keyFieldLockedMode = 0;
        seedFieldLockedMode = 0;
        mEmailField.unfocus();
        mKeyField.unfocus();
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        }
    else if( inTarget == &mBackToAccountTabButton ) {
        leftPanePage = 0;
        updateLeftPane();
        
        emailFieldLockedMode = 0;
        keyFieldLockedMode = 0;
        seedFieldLockedMode = 0;
        mEmailField.unfocus();
        mKeyField.unfocus();
        mSpawnSeed.unfocus();
        updatefieldsAndLockButtons();
        }
        
    }



void ExistingAccountPage::nextPage() {
    
    emailFieldLockedMode = 0;
    keyFieldLockedMode = 0;
    seedFieldLockedMode = 0;
    mEmailField.unfocus();
    mKeyField.unfocus();
    mSpawnSeed.unfocus();
    updatefieldsAndLockButtons();
    
    if( leftPanePage == 0 ) {
        actionPerformed( &mNextToGameTabButton );
        }
    else if( leftPanePage == 1 ) {
        actionPerformed( &mLoginButton );
        }
    }

    

void ExistingAccountPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        
        return;
        }

    if( inASCII == 10 || inASCII == 13 ) {
        // enter key
        
        nextPage();
        }
    }



void ExistingAccountPage::specialKeyDown( int inKeyCode ) {
    if( inKeyCode == MG_KEY_DOWN ||
        inKeyCode == MG_KEY_UP ) {
        
        nextPage();
        return;
        }
    }



void ExistingAccountPage::processLogin( char inStore, const char *inSignal ) {
    if( userEmail != NULL ) {
        delete [] userEmail;
        }
    userEmail = mEmailField.getText();
        
    if( accountKey != NULL ) {
        delete [] accountKey;
        }
    accountKey = mKeyField.getText();

    if( !gamePlayingBack ) {
        
        if( inStore ) {
            SettingsManager::setSetting( "email", userEmail );
            SettingsManager::setSetting( "accountKey", accountKey );
            }
        else {
            SettingsManager::setSetting( "email", "" );
            SettingsManager::setSetting( "accountKey", "" );
            }
        }
    
                
    setSignal( inSignal );
    }



void ExistingAccountPage::draw( doublePair inViewCenter, 
                                double inViewSize ) {
    
    
    
    if( !mFPSMeasureDone ) {
        double timePassed = game_getCurrentTime() - mPageActiveStartTime;
        double settleTime = 0.1;

        if ( timePassed > settleTime ) {
            mFramesCounted ++;
            }
        
        if( timePassed > 1 + settleTime ) {
            double fps = mFramesCounted / ( timePassed - settleTime );
            int targetFPS = 
                SettingsManager::getIntSetting( "targetFrameRate", -1 );
            char fpsFailed = true;
            
            if( targetFPS != -1 ) {
                
                double diff = fabs( fps - targetFPS );
                
                if( diff / targetFPS > 0.1 ) {
                    // more than 10% off

                    fpsFailed = true;
                    }
                else {
                    // close enough
                    fpsFailed = false;
                    }
                }

            if( !fpsFailed ) {
                // mLoginButton.setVisible( true );
                // mSpecificButton.setVisible( true );
                
                int pastSuccess = 
                    SettingsManager::getIntSetting( "loginSuccess", 0 );
                // friendsButton visible whenever loginButton is visible
                if( pastSuccess || true ) {
                    // mFriendsButton.setVisible( true );
                    }
                
                triggerLifeTokenUpdate();
                triggerFitnessScoreUpdate();
                }
            else {
                // show error message
                
                char *message = autoSprintf( translate( "fpsErrorLogin" ),
                                                        fps, targetFPS );
                setStatusDirect( message, true );
                delete [] message;

                setStatusPosition( true );
                mRetryButton.setVisible( true );
                mRedetectButton.setVisible( true );
                mCancelButton.setVisible( true );
                }
            

            mFPSMeasureDone = true;
            
            if( !fpsFailed ) makeActive( true );
            }
        }


    

    setDrawColor( 1, 1, 1, 1 );
    

    doublePair pos = { 0, -175 };
    
    if( useSteamUpdate && 
        (!mTwinCodeField.isFocused() || !mTwinCodeField.isVisible()) &&
        ( keyFieldLockedMode != 2 ) ) {
        setDrawColor( 1, 1, 1, 0.75 );
        drawSprite( instructionsSprite, {0, 0} );
        }


    if( ! mEmailField.isVisible() && 0 ) {
        char *email = mEmailField.getText();
        
        const char *transString = "email";
        
        char *steamSuffixPointer = strstr( email, "@steamgames.com" );
        
        char coverChar = 'x';

        if( steamSuffixPointer != NULL ) {
            // terminate it
            steamSuffixPointer[0] ='\0';
            transString = "steamID";
            coverChar = 'X';
            }

        if( mHideAccount ) {
            int len = strlen( email );
            for( int i=0; i<len; i++ ) {
                if( email[i] != '@' &&
                    email[i] != '.' ) {
                    email[i] = coverChar;
                    }
                }   
            }
        

        char *s = autoSprintf( "%s  %s", translate( transString ), email );
        
        pos = mEmailField.getPosition();

        pos.x = -350;        
        setDrawColor( 1, 1, 1, 1.0 );
        mainFont->drawString( s, pos, alignLeft );
        
        delete [] email;
        delete [] s;
        }

    if( ! mKeyField.isVisible() && 0 ) {
        char *key = mKeyField.getText();

        if( mHideAccount ) {
            int len = strlen( key );
            for( int i=0; i<len; i++ ) {
                if( key[i] != '-' ) {
                    key[i] = 'X';
                    }
                }   
            }

        char *s = autoSprintf( "%s  %s", translate( "accountKey" ), key );
        
        pos = mKeyField.getPosition();
        
        pos.x = -350;        
        setDrawColor( 1, 1, 1, 1.0 );
        mainFont->drawString( s, pos, alignLeft );
        
        delete [] key;
        delete [] s;
        }
    
    if( leftPanePage == 1 ) {
        if( !useSteamUpdate ) {
            pos = mSpecificButton.getPosition();
            pos.x = mEmailField.getLeftEdgeX() + mainFont->getFontHeight() * 0.25 * 0.5;
            pos.y += 30 + 16;
            setDrawColor( 1, 1, 1, 1.0 );
            if( mSpecificButton.isVisible() ) mainFont->drawString( "WHERE TO SPAWN?", pos, alignLeft );
            }
        
        pos = mFriendsButton.getPosition();
        pos.x = mEmailField.getLeftEdgeX() + mainFont->getFontHeight() * 0.25 * 0.5;
        pos.y += 30 + 16;
        setDrawColor( 1, 1, 1, 1.0 );
        if( mFriendsButton.isVisible() ) mainFont->drawString( "PLAY WITH FRIENDS?", pos, alignLeft );        
        }


    pos = mEmailField.getPosition();
    pos.y += 100;

    if( mFPSMeasureDone && 
        ! mRedetectButton.isVisible() &&
        ! mDisableCustomServerButton.isVisible() ) {
        
        char *newTokenMessage = drawTokenMessage( pos, true );
        if( newTokenMessage != NULL ) {
            delete [] tokenMessage;
            tokenMessage = newTokenMessage;
            mLoginButton.setCursorTip( tokenMessage );
            }
        
        pos = mEmailField.getPosition();
        
        pos.x = 
            ( mTutorialButton.getPosition().x + 
              mLoginButton.getPosition().x )
            / 2;

        pos.x -= 32;
        
        if( fitnessMessage != NULL ) delete [] fitnessMessage;
        fitnessMessage = drawFitnessScore( pos, false, true );
        if( fitnessMessage != NULL ) mGenesButton.setCursorTip( fitnessMessage );

        if( isFitnessScoreReady() ) {
            mGenesButton.setVisible( true );
            }
        }
        
        
    int buttonsInterval = 96;
    mCancelButton.setPosition( 360, -272 );
    doublePair buttonPos = mCancelButton.getPosition();
    
    if( mTechTreeButton.isVisible() ) {
        buttonPos.y += buttonsInterval;
        mTechTreeButton.setPosition( buttonPos.x, buttonPos.y );
        }
    if( mFamilyTreesButton.isVisible() ) {
        buttonPos.y += buttonsInterval;
        mFamilyTreesButton.setPosition( buttonPos.x, buttonPos.y );
        }
    if( mGenesButton.isVisible() ) {
        buttonPos.y += buttonsInterval;
        mGenesButton.setPosition( buttonPos.x, buttonPos.y );
        }
    if( mTutorialButton.isVisible() && tutorialDone ) {
        buttonPos.y += buttonsInterval;
        mTutorialButton.setPosition( buttonPos.x, buttonPos.y );
        }
    if( mSettingsButton.isVisible() ) {
        buttonPos.y += buttonsInterval;
        mSettingsButton.setPosition( buttonPos.x, buttonPos.y );
        }
        
    }

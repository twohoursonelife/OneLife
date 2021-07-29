#include "ExistingAccountPage.h"

#include "message.h"
#include "buttonStyle.h"

#include "accountHmac.h"

#include "lifeTokens.h"
#include "fitnessScore.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/crypto/hashes/sha1.h"


#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

static JenkinsRandomSource randSource;

char fieldsLocked;

extern Font *mainFont;


extern char gamePlayingBack;

extern char useSpawnSeed;
extern char *userEmail;
extern char *accountKey;


extern SpriteHandle instructionsSprite;

extern char loginEditOverride;



ExistingAccountPage::ExistingAccountPage()
        : mEmailField( mainFont, -360, 96, 10, false, 
                       translate( "username" ),
                       NULL,
                       // forbid only spaces
                       " " ),
          mKeyField( mainFont, -360, 0, 15, true,
                     translate( "accountKey" ),
                     // allow only ticket code characters
                     "23456789ABCDEFGHJKLMNPQRSTUVWXYZ-" ),
          mSpawnSeed( mainFont, -360, -192, 10, false, 
                                     translate( "spawnSeed" ),
                                     NULL,
                                     // forbid spaces
                                     " " ),
          mAtSignButton( mainFont, 252, 128, "@" ),
          mPasteButton( mainFont, 0, -112, translate( "paste" ), 'v', 'V' ),
          mDisableCustomServerButton( mainFont, 0, 220, 
                                      translate( "disableCustomServer" ) ),
          mBackground( "background.tga", 0.75f ),
          mGameLogo( "logo.tga", 1.0f, {-360, 256} ),
          mSeedButton( mainFont, -360, -64, "SEED" ),
          mUnlockButton( mainFont, -360, -256, "UNLOCK" ),
          mLoginButton( mainFont, -360, -64, translate( "loginButton" ) ),
          mFriendsButton( mainFont, -360, -64, translate( "friendsButton" ) ),
          mGenesButton( mainFont, 522, 300, translate( "genesButton" ) ),
          mFamilyTreesButton( mainFont, 360, 16, translate( "familyTrees" ) ),
          mTechTreeButton( mainFont, 360, -176, translate( "techTree" ) ),
          mClearAccountButton( mainFont, -360, -64, 
                               translate( "clearAccount" ) ),
          mCancelButton( mainFont, 360, -272, 
                         translate( "quit" ) ),
          mSettingsButton( mainFont, 360, -80,
                           translate( "settingsButton" ) ),
          mReviewButton( mainFont, -400, -200, 
                         translate( "postReviewButton" ) ),
          mRetryButton( mainFont, -100, 198, translate( "retryButton" ) ),
          mRedetectButton( mainFont, 100, 198, translate( "redetectButton" ) ),
          mViewAccountButton( mainFont, 0, 64, translate( "view" ) ),
          mTutorialButton( mainFont, 360, 112, 
                           translate( "tutorial" ) ),
          mPageActiveStartTime( 0 ),
          mFramesCounted( 0 ),
          mFPSMeasureDone( false ),
          mHideAccount( false ) {
    
    
    // center this in free space
    
    mPasteButton.setPosition( mKeyField.getRightEdgeX() + 64,
                              47 );
    
    // align this one with the paste button
    mAtSignButton.setPosition( mEmailField.getRightEdgeX() + 48,
                               128 );
    
    mLoginButton.setPosition( mEmailField.getRightEdgeX() - ( mLoginButton.getWidth()/2 ), -64 );
    mSeedButton.setPosition( -360 + ( mFriendsButton.getWidth() - mLoginButton.getWidth() )/2, -64 );
    mFriendsButton.setPosition( mEmailField.getLeftEdgeX() + ( mFriendsButton.getWidth()/2 ), -64 );

    if( userEmail != NULL && accountKey != NULL ) {
        mEmailField.setText( userEmail );
        mKeyField.setText( accountKey );
        }

    setButtonStyle( &mLoginButton );
    setButtonStyle( &mFriendsButton );
    setButtonStyle( &mGenesButton );
    setButtonStyle( &mFamilyTreesButton );
    setButtonStyle( &mTechTreeButton );
    setButtonStyle( &mClearAccountButton );
    setButtonStyle( &mCancelButton );
    setButtonStyle( &mSettingsButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mAtSignButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mRetryButton );
    setButtonStyle( &mRedetectButton );
    setButtonStyle( &mViewAccountButton );
    setButtonStyle( &mTutorialButton );

    setButtonStyle( &mDisableCustomServerButton );
    setButtonStyle( &mUnlockButton );
    setButtonStyle( &mSeedButton );
    
    mFields[0] = &mEmailField;
    mFields[1] = &mKeyField;

    
    addComponent( &mBackground );
    addComponent( &mGameLogo );
                                     
    addComponent( &mLoginButton );
    addComponent( &mFriendsButton );
    addComponent( &mGenesButton );
    addComponent( &mFamilyTreesButton );
    addComponent( &mTechTreeButton );
    //addComponent( &mClearAccountButton );
    addComponent( &mCancelButton );
    addComponent( &mSettingsButton );
    addComponent( &mReviewButton );
    addComponent( &mAtSignButton );
    addComponent( &mPasteButton );
    addComponent( &mEmailField );
    addComponent( &mKeyField );
    addComponent( &mRetryButton );
    addComponent( &mRedetectButton );
    addComponent( &mDisableCustomServerButton );

    addComponent( &mViewAccountButton );
    addComponent( &mTutorialButton );

    addComponent( &mUnlockButton );
    addComponent( &mSeedButton );
    addComponent( &mSpawnSeed );
    
    // this section have all buttons with the same width
    mTutorialButton.setSize( 175, 60 );
    mSettingsButton.setSize( 175, 60 );
    mFamilyTreesButton.setSize( 175, 60 );
    mTechTreeButton.setSize( 175, 60 );
    mCancelButton.setSize( 175, 60 );
    
    mEmailField.setLabelTop( true );
    mKeyField.setLabelTop( true );
    mSpawnSeed.setLabelTop( true );
    
    mLoginButton.addActionListener( this );
    mFriendsButton.addActionListener( this );
    mSeedButton.addActionListener( this );
    mGenesButton.addActionListener( this );
    mFamilyTreesButton.addActionListener( this );
    mTechTreeButton.addActionListener( this );
    mClearAccountButton.addActionListener( this );
    
    mCancelButton.addActionListener( this );
    mSettingsButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    
    mAtSignButton.addActionListener( this );
    mPasteButton.addActionListener( this );

    mRetryButton.addActionListener( this );
    mRedetectButton.addActionListener( this );

    mViewAccountButton.addActionListener( this );
    mTutorialButton.addActionListener( this );
    
    mDisableCustomServerButton.addActionListener( this );
    mUnlockButton.addActionListener( this );
    
    mRetryButton.setVisible( false );
    mRedetectButton.setVisible( false );
    mDisableCustomServerButton.setVisible( false );
    
    mAtSignButton.setMouseOverTip( translate( "atSignTip" ) );

    mLoginButton.setMouseOverTip( translate( "getAccountTip" ) );
    mClearAccountButton.setMouseOverTip( translate( "clearAccountTip" ) );
    
    mFriendsButton.setMouseOverTip( translate( "friendsTip" ) );
    mGenesButton.setMouseOverTip( translate( "genesTip" ) );
    mFamilyTreesButton.setMouseOverTip( translate( "familyTreesTip" ) );
    mTechTreeButton.setMouseOverTip( translate( "techTreeTip" ) );
    

    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    

    // to dodge quit message
    setTipPosition( true );
    }

          
        
ExistingAccountPage::~ExistingAccountPage() {
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




void ExistingAccountPage::makeActive( char inFresh ) {

    

    if( SettingsManager::getIntSetting( "tutorialDone", 0 ) ) {
        mTutorialButton.setVisible( true );
        }
    else {
        // tutorial forced anyway
        mTutorialButton.setVisible( false );
        }

    useSpawnSeed = false;

    char *seed = 
        SettingsManager::getSettingContents( "spawnSeed", "" );

    mSpawnSeed.setList( seed );
        
    delete [] seed;



    

    mFramesCounted = 0;
    mPageActiveStartTime = game_getCurrentTime();    
    
    // don't re-measure every time we return to this screen
    // it slows the player down too much
    // re-measure only at first-startup
    //mFPSMeasureDone = false;
    
    mLoginButton.setVisible( false );
    mSeedButton.setVisible( false );
    mFriendsButton.setVisible( false );
    mGenesButton.setVisible( false );
    
    
    int skipFPSMeasure = SettingsManager::getIntSetting( "skipFPSMeasure", 0 );
    
    if( skipFPSMeasure ) {
        mFPSMeasureDone = true;
        mRetryButton.setVisible( false );
        mRedetectButton.setVisible( false );
        }

    if( mFPSMeasureDone && ! mRetryButton.isVisible() ) {
        // skipping measure OR we are returning to this page later
        // and not measuring again
        mLoginButton.setVisible( true );
        mSeedButton.setVisible( true );
        mFriendsButton.setVisible( true );
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
    

    int pastSuccess = SettingsManager::getIntSetting( "loginSuccess", 0 );

    char *emailText = mEmailField.getText();
    char *keyText = mKeyField.getText();

    mUnlockButton.setLabelText( "LOCK FIELDS" );
    fieldsLocked = false;

    if ( SettingsManager::getIntSetting( "streamProtection", 0 ) ) {
        mUnlockButton.setLabelText( "UNLOCK FIELDS" );
        fieldsLocked = true;
        }

    mSpawnSeed.setContentsHidden( true );
    mSpawnSeed.setIgnoreEvents( fieldsLocked );


    // don't hide field contents unless there is something to hide
    if( ! pastSuccess || 
        ( strcmp( emailText, "" ) == 0 
          &&
          strcmp( keyText, "" ) == 0 ) ) {

        mEmailField.focus();

        mFamilyTreesButton.setVisible( false );
        }
    else {
        mEmailField.unfocus();
        mKeyField.unfocus();
        
        mEmailField.setContentsHidden( true );
        mKeyField.setContentsHidden( true );
        
        mEmailField.setIgnoreEvents( fieldsLocked );
        mKeyField.setIgnoreEvents( fieldsLocked );
        
        char *url = SettingsManager::getStringSetting( "lineageServerURL", "" );

        char show = ( strcmp( url, "" ) != 0 )
            && isURLLaunchSupported();
        mFamilyTreesButton.setVisible( show );
        delete [] url;
        }
    
    delete [] emailText;
    delete [] keyText;

    
    mPasteButton.setVisible( false );
    mAtSignButton.setVisible( false );


    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    else {
        mReviewButton.setLabelText( translate( "postReviewButton" ) );
        }


    if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
        // no review button on Steam
        mReviewButton.setVisible( false );
        
        if( ! loginEditOverride ) {
            mEmailField.setVisible( false );
            mKeyField.setVisible( false );
            mEmailField.unfocus();
            mKeyField.unfocus();
            
            mClearAccountButton.setVisible( false );
            
            mViewAccountButton.setVisible( true );
            mHideAccount = true;
            }
        else {
            mEmailField.setVisible( true );
            mKeyField.setVisible( true );       
     
            mClearAccountButton.setVisible( true );

            mEmailField.setContentsHidden( false );
            mKeyField.setContentsHidden( false );
            mEmailField.focus();
            
            loginEditOverride = false;
            
            mViewAccountButton.setVisible( false );
            mHideAccount = false;
            }
        }
    else {
        mHideAccount = false;
        mReviewButton.setVisible( true );
        mViewAccountButton.setVisible( false );
        }
		
	mReviewButton.setVisible( false );
    }



void ExistingAccountPage::makeNotActive() {
    for( int i=0; i<2; i++ ) {
        mFields[i]->unfocus();
        }
    }



void ExistingAccountPage::step() {
    mPasteButton.setVisible( isClipboardSupported() &&
                             mKeyField.isFocused() );
    //mAtSignButton.setVisible( mEmailField.isFocused() );
    
    int blockClicks = fieldsLocked;
    if ( mSpawnSeed.isFocused() ) { blockClicks = true; }
    
    mLoginButton.setIgnoreEvents( blockClicks );
    mSeedButton.setIgnoreEvents( blockClicks );
    mFriendsButton.setIgnoreEvents( blockClicks );
    
    mEmailField.setIgnoreEvents( blockClicks );
    mKeyField.setIgnoreEvents( blockClicks );
    }



void ExistingAccountPage::actionPerformed( GUIComponent *inTarget ) {
    //saves seed setting on any action performed
    char *seedList = mSpawnSeed.getAndUpdateList();

    SettingsManager::setSetting( "spawnSeed", seedList );
    delete [] seedList;

    if( inTarget == &mLoginButton ) {
        processLogin( true, "done" );
        }
    else if( inTarget == &mSeedButton ) {
        useSpawnSeed = true;
        
        processLogin( true, "done" );
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
        processLogin( true, "friends" );
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
    else if( inTarget == &mUnlockButton ) {
        if ( fieldsLocked ) {
            mUnlockButton.setLabelText( "LOCK FIELDS" );

            SettingsManager::setSetting( "streamProtection", 0 );
            fieldsLocked = false;
            }
        else {
            mUnlockButton.setLabelText( "UNLOCK FIELDS" );

            SettingsManager::setSetting( "streamProtection", 1 );
            fieldsLocked = true;
            }
                    
        mEmailField.setContentsHidden( true );
        mKeyField.setContentsHidden( true );
        mSpawnSeed.setContentsHidden( true );
        
        mEmailField.setIgnoreEvents( fieldsLocked );
        mKeyField.setIgnoreEvents( fieldsLocked );
        mSpawnSeed.setIgnoreEvents( fieldsLocked );
        
        mEmailField.unfocus();
        mKeyField.unfocus();
        mSpawnSeed.unfocus();
        }
    }



void ExistingAccountPage::switchFields() {
    if( mFields[0]->isFocused() ) {
        mFields[1]->focus();
        }
    else if( mFields[1]->isFocused() ) {
        mFields[0]->focus();
        }
    }

    

void ExistingAccountPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }

    if( inASCII == 10 || inASCII == 13 ) {
        // enter key
        
        if( mKeyField.isFocused() ) {

            processLogin( true, "done" );
            
            return;
            }
        else if( mEmailField.isFocused() ) {
            switchFields();
            }
        }
    }



void ExistingAccountPage::specialKeyDown( int inKeyCode ) {
    if( inKeyCode == MG_KEY_DOWN ||
        inKeyCode == MG_KEY_UP ) {
        
        switchFields();
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
                mLoginButton.setVisible( true );
                mSeedButton.setVisible( true );
                
                int pastSuccess = 
                    SettingsManager::getIntSetting( "loginSuccess", 0 );
                // friendsButton visible whenever loginButton is visible
                if( pastSuccess || true ) {
                    mFriendsButton.setVisible( true );
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

                setStatusPositiion( true );
                mRetryButton.setVisible( true );
                mRedetectButton.setVisible( true );
                }
            

            mFPSMeasureDone = true;
            }
        }
    

    setDrawColor( 1, 1, 1, 1 );
    

    doublePair pos = { 0, -175 };
    
    //drawSprite( instructionsSprite, pos );


    if( ! mEmailField.isVisible() ) {
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

    if( ! mKeyField.isVisible() ) {
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
    


    pos = mEmailField.getPosition();
    pos.y += 100;

    if( mFPSMeasureDone && 
        ! mRedetectButton.isVisible() &&
        ! mDisableCustomServerButton.isVisible() ) {
        
        drawTokenMessage( pos );
        
        pos = mEmailField.getPosition();
        
        pos.x = 
            ( mTutorialButton.getPosition().x + 
              mLoginButton.getPosition().x )
            / 2;

        pos.x -= 32;
        
        drawFitnessScore( pos );

        if( isFitnessScoreReady() ) {
            mGenesButton.setVisible( true );
            }
        }
    }



Background::Background( const char *inImageName, float inOpacity, doublePair inPosition )
        : PageComponent( 0, 0 ),
          mImage( loadSprite( inImageName, false ) ),
          mOpacity( inOpacity ),
          mPosition( inPosition ) {
    }

   
void Background::draw() {
    setDrawColor( 1, 1, 1, mOpacity );
    
    if (! isPaused() && mImage != NULL ) {
        drawSprite( mImage, mPosition );
        }
    }
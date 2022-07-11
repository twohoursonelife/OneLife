#include "SettingsPage.h"


#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"
#include "minorGems/system/Time.h"

#include "musicPlayer.h"
#include "soundBank.h"
#include "objectBank.h"
#include "buttonStyle.h"

#include "DropdownList.h"


extern Font *mainFont;

extern float musicLoudness;

extern bool showingInGameSettings;


SettingsPage::SettingsPage()
        : mBackground( "background.tga", 0.75f ),
          
          // Left Pane
          mRestartButton( mainFont, 360, -272, translate( "restartButton" ) ),
          
          mGameplayButton( mainFont, -452.5, 208, "GAMEPLAY" ),
          mControlButton( mainFont, -452.5, 112, "CONTROL" ),
          mScreenButton( mainFont, -452.5, 16, "SCREEN" ),
          mSoundButton( mainFont, -452.5, -80, "SOUND" ),
          mBackButton( mainFont, -452.5, -272, translate( "backButton" ) ),
          
          mEditAccountButton( mainFont, -463, 129, translate( "editAccount" ) ),

          // Gameplay
		  mEnableFOVBox( 561, 128, 4 ),
		  mEnableCenterCameraBox( 561, 52, 4 ),
          mEnableNudeBox( -335, 148, 4 ),
          
          // Control
		  mEnableKActionsBox( 561, 90, 4 ),
          mCursorScaleSlider( mainFont, 297, 155, 4, 200, 30,
                                       1.0, 10.0, 
                                       translate( "scale" ) ),
          
          // Screen
          mRedetectButton( mainFont, 153, 249, translate( "redetectButton" ) ),
          mFullscreenBox( 0, 128, 4 ),
          mBorderlessBox( 0, 168, 4 ),
          
          // Sound
          mMusicLoudnessSlider( mainFont, 0, 40, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -48, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ) {
                            

    
    // Adding components below in reverse order so cursor tip is drawn on top
    
    addComponent( &mBackground );
    
    
    // Sound
    addComponent( &mSoundEffectsLoudnessSlider );
    mSoundEffectsLoudnessSlider.addActionListener( this );
    addComponent( &mMusicLoudnessSlider );
    mMusicLoudnessSlider.addActionListener( this );
    
    // Screen
    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );
    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );
    setButtonStyle( &mRedetectButton );
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );
    
    // Control
    addComponent( &mCursorScaleSlider );
    mCursorScaleSlider.addActionListener( this );
    mCursorScaleSlider.toggleField( false );
    
    const char *choiceList[3] = { translate( "system" ),
                                  translate( "drawn" ),
                                  translate( "both" ) };
    
    mCursorModeSet = 
        new RadioButtonSet( mainFont, 561, 275,
                            3, choiceList,
                            false, 4 );
    addComponent( mCursorModeSet );
    mCursorModeSet->addActionListener( this );
    
	addComponent( &mEnableKActionsBox );
    mEnableKActionsBox.addActionListener( this );
    
    // Gameplay
    addComponent( &mEnableNudeBox );
    mEnableNudeBox.addActionListener( this );
	addComponent( &mEnableCenterCameraBox );
    mEnableCenterCameraBox.addActionListener( this );
	addComponent( &mEnableFOVBox );
    mEnableFOVBox.addActionListener( this );
    
    // Left pane
    setButtonStyle( &mBackButton );
    mBackButton.setSize( 175, 60 );
    addComponent( &mBackButton );
    mBackButton.addActionListener( this );
    
    setButtonStyle( &mSoundButton );
    mSoundButton.setSize( 175, 60 );
    addComponent( &mSoundButton );
    mSoundButton.addActionListener( this );
    
    setButtonStyle( &mScreenButton );
    mScreenButton.setSize( 175, 60 );
    addComponent( &mScreenButton );
    mScreenButton.addActionListener( this );
    
    setButtonStyle( &mControlButton );
    mControlButton.setSize( 175, 60 );
    addComponent( &mControlButton );
    mControlButton.addActionListener( this );
    
    setButtonStyle( &mGameplayButton );
    mGameplayButton.setSize( 175, 60 );
    addComponent( &mGameplayButton );
    mGameplayButton.addActionListener( this );
    
    
    setButtonStyle( &mRestartButton );
    mRestartButton.setSize( 175, 60 );
    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    mRestartButton.setVisible( false );

    

    // Not in use
    setButtonStyle( &mEditAccountButton );
    // addComponent( &mEditAccountButton );
    mEditAccountButton.addActionListener( this );

    
    mRestartButton.setCursorTip( "RESTART THE GAME" );
    
    mGameplayButton.setCursorTip( "GAMEPLAY SETTINGS" );
    mControlButton.setCursorTip( "CONTROL SETTINGS" );
    mScreenButton.setCursorTip( "SCREEN SETTINGS" );
    mSoundButton.setCursorTip( "SOUND SETTINGS" );
    mBackButton.setCursorTip( "GO BACK" );
    
    mEnableFOVBox.setCursorTip( "ENABLE ZOOM-IN AND ZOOM-OUT WITH MOUSE WHEEL SCROLLING" );
    mEnableCenterCameraBox.setCursorTip( "ALWAYS CENTER THE CAMERA VIEW ON YOUR CHARACTER" );
    mEnableNudeBox.setCursorTip( "ENABLE NUDITY" );
    
    mEnableKActionsBox.setCursorTip( "ENABLE WASD MOVEMENT AND ACTION" );
    mCursorModeSet->setCursorTip( "USE DRAWN CURSOR FOR ULTRAWIDE SCREEN" );
    
    mRedetectButton.setCursorTip( "RESTART THE GAME TO REDETECT FRAME RATE" );
    mFullscreenBox.setCursorTip( "TOGGLE BETWEEN FULLSCREEN AND WINDOWED MODE" );
    mBorderlessBox.setCursorTip( "ALLOW CURSOR TO MOVE OUTSIDE THE GAME WINDOW" );

    
    mOldFullscreenSetting = 
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mTestSound = blankSoundUsage;

    mMusicStartTime = 0;

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    mBorderlessBox.setVisible( mOldFullscreenSetting );


    mOldBorderlessSetting = 
        SettingsManager::getIntSetting( "borderless", 0 );

    mBorderlessBox.setToggled( mOldBorderlessSetting );

    mEnableNudeSetting =
        SettingsManager::getIntSetting( "nudeEnabled", 1 );

    mEnableNudeBox.setToggled( mEnableNudeSetting );
	
	mEnableFOVSetting =
        SettingsManager::getIntSetting( "fovEnabled", 0 );
	
	mEnableFOVBox.setToggled( mEnableFOVSetting );
    
	mEnableKActionsSetting =
        SettingsManager::getIntSetting( "keyboardActions", 0 );
	
	mEnableKActionsBox.setToggled( mEnableKActionsSetting );
        
	mEnableCenterCameraSetting =
        SettingsManager::getIntSetting( "centerCamera", 0 );
	
	mEnableCenterCameraBox.setToggled( mEnableCenterCameraSetting );
    
    mPage = 0;
    




    }



SettingsPage::~SettingsPage() {
    clearSoundUsage( &mTestSound );

    delete mCursorModeSet;
    }




void SettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mEditAccountButton ) {
        setSignal( "editAccount" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        }
    else if( inTarget == &mBorderlessBox ) {
        int newSetting = mBorderlessBox.getToggled();
        
        SettingsManager::setSetting( "borderless", newSetting );
        }
	else if( inTarget == &mEnableNudeBox ) {
        int newSetting = mEnableNudeBox.getToggled();
        
        SettingsManager::setSetting( "nudeEnabled", newSetting );
        
        //extern variable in objectBank removes the need to restart the game
        //we just need to set the new value to the variable and the game will
        //start ignoring the nudity sprites every times it draws a new object
        //mRestartButton.setVisible( mEnableNudeSetting != newSetting );
        NudeToggle = newSetting;
        }
	else if( inTarget == &mEnableFOVBox ) {
        int newSetting = mEnableFOVBox.getToggled();
        
        SettingsManager::setSetting( "fovEnabled", newSetting );
        }
	else if( inTarget == &mEnableKActionsBox ) {
        int newSetting = mEnableKActionsBox.getToggled();
        
        SettingsManager::setSetting( "keyboardActions", newSetting );
        }
	else if( inTarget == &mEnableCenterCameraBox ) {
        int newSetting = mEnableCenterCameraBox.getToggled();
        
        SettingsManager::setSetting( "centerCamera", newSetting );
        }
    else if( inTarget == &mRestartButton ||
             inTarget == &mRedetectButton ) {
        // always re-measure frame rate after relaunch
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
    else if( inTarget == &mSoundEffectsLoudnessSlider ) {
        setMusicLoudness( 0 );
        mMusicStartTime = 0;
        
        if( ! mSoundEffectsLoudnessSlider.isPointerDown() ) {
            
            setSoundEffectsLoudness( 
                mSoundEffectsLoudnessSlider.getValue() );
        
            if( mTestSound.numSubSounds > 0 ) {
                doublePair pos = { 0, 0 };
                
                playSound( mTestSound, pos );
                }
            }
        }    
    else if( inTarget == &mMusicLoudnessSlider ) {
            

        if( ! mSoundEffectsLoudnessSlider.isPointerDown() ) {
            musicLoudness = mMusicLoudnessSlider.getValue();
            SettingsManager::setSetting( "musicLoudness", musicLoudness );
            }
            
        
        if( Time::getCurrentTime() - mMusicStartTime > 25 ) {

            instantStopMusic();
            

            restartMusic( 9.0, 1.0/60.0, true );
            
            setMusicLoudness( mMusicLoudnessSlider.getValue(), true );


            mMusicStartTime = Time::getCurrentTime();
            }
        else {
            setMusicLoudness( mMusicLoudnessSlider.getValue(), true );
            }
        }
    else if( inTarget == mCursorModeSet ) {
        setCursorMode( mCursorModeSet->getSelectedItem() );
        
        // int mode = getCursorMode();
        
        // mCursorScaleSlider.setVisible( mode > 0 );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );
        }
    else if( inTarget == &mCursorScaleSlider ) {
        setEmulatedCursorScale( mCursorScaleSlider.getValue() );
        }
    else if( inTarget == &mGameplayButton ) {
        mPage = 0;
        }
    else if( inTarget == &mControlButton ) {
        mPage = 1;
        }
    else if( inTarget == &mScreenButton ) {
        mPage = 2;
        }
    else if( inTarget == &mSoundButton ) {
        mPage = 3;
        }
    
    checkRestartRequired();
    updatePage();
    }


extern int targetFramesPerSecond;


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    if( mFullscreenBox.isVisible() ) {
        doublePair pos = mFullscreenBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;
        
        mainFont->drawString( translate( "fullscreen" ), pos, alignRight );


        if( mBorderlessBox.isVisible() ) {
            pos = mBorderlessBox.getPosition();
        
            pos.x -= 30;
            pos.y -= 2;
            
            mainFont->drawString( translate( "borderless" ), pos, alignRight );
            }
        

        pos = mFullscreenBox.getPosition();
        
        pos.y += 52;
        pos.x -= 16;
        
        if( getCountingOnVsync() ) {
            mainFont->drawString( translate( "vsyncYes" ), pos, alignLeft );
            }
        else {
            mainFont->drawString( translate( "vsyncNo" ), pos, alignLeft );
            }
        
        pos.y += 52;

        char *fpsString = autoSprintf( "%d", targetFramesPerSecond );
        
        mainFont->drawString( fpsString, pos, alignLeft );
        delete [] fpsString;


        pos.y += 52;

        char *currentFPSString = autoSprintf( "%.2f", getRecentFrameRate() );
        
        mainFont->drawString( currentFPSString, pos, alignLeft );
        delete [] currentFPSString;
        

        pos = mFullscreenBox.getPosition();
        pos.x -= 30;

        pos.y += 52;
        mainFont->drawString( translate( "vsyncOn" ), pos, alignRight );
        pos.y += 52;
        mainFont->drawString( translate( "targetFPS" ), pos, alignRight );
        pos.y += 52;
        mainFont->drawString( translate( "currentFPS" ), pos, alignRight );
        }


    if( mEnableNudeBox.isVisible() ) {
        doublePair pos = mEnableNudeBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        mainFont->drawString( "ENABLE NUDITY", pos, alignRight );
        }
	
    if( mEnableFOVBox.isVisible() ) {
        doublePair pos = mEnableFOVBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        mainFont->drawString( "ENABLE ZOOM", pos, alignRight );
        }
	
    if( mEnableKActionsBox.isVisible() ) {
        doublePair pos = mEnableKActionsBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        mainFont->drawString( "KEYBOARD ACTIONS:", pos, alignRight );
        }
	
    if( mEnableCenterCameraBox.isVisible() ) {
        doublePair pos = mEnableCenterCameraBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        mainFont->drawString( "CENTER CAMERA", pos, alignRight );
        }


    if( mCursorModeSet->isVisible() ) {
        
        double xPos = mEnableCenterCameraBox.getPosition().x - 30;
        
        doublePair pos = mCursorModeSet->getPosition();
        
        pos.x = xPos;
        pos.y += 37;
        
        mainFont->drawString( translate( "cursor"), pos, alignRight );
        
        if( mCursorScaleSlider.isVisible() ) {
            
            pos = mCursorScaleSlider.getPosition();
            
            pos.x = xPos;
            pos.y -= 2;
            
            mainFont->drawString( "DRAWN CURSOR SIZE:", pos, alignRight );
            }
        }
    
    }



void SettingsPage::step() {
    if( mTestSound.numSubSounds > 0 ) {
        markSoundUsageLive( mTestSound );
        }
    stepMusicPlayer();
    }





void SettingsPage::makeActive( char inFresh ) {
    if( inFresh ) {        

        int mode = getCursorMode();
        
        mCursorModeSet->setSelectedItem( mode );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );
        


        mMusicLoudnessSlider.setValue( musicLoudness );
        mSoundEffectsLoudnessSlider.setValue( getSoundEffectsLoudness() );
        setMusicLoudness( 0 );
        mMusicStartTime = 0;
        
        int tryCount = 0;
        
        while( mTestSound.numSubSounds == 0 && tryCount < 10 ) {

            int oID = getRandomPersonObject();

            if( oID > 0 ) {
                ObjectRecord *r = getObject( oID );
                if( r->usingSound.numSubSounds >= 1 ) {
                    mTestSound = copyUsage( r->usingSound );
                    // constrain to only first subsound                    
                    mTestSound.numSubSounds = 1;
                    // play it at full volume
                    mTestSound.volumes[0] = 1.0;
                    }
                }
            tryCount ++;
            }
        
        if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
            mEditAccountButton.setVisible( true );
            }
        else {
            mEditAccountButton.setVisible( false );
            }
            
        updatePage();

        }
    }



void SettingsPage::makeNotActive() {
    }


void SettingsPage::updatePage() {

    doublePair pos = { 0, 0 };
    double lineSpacing = 52;
    
    mEnableFOVBox.setPosition( 0, lineSpacing );
    mEnableCenterCameraBox.setPosition( 0, 0 );
    mEnableNudeBox.setPosition( 0, -lineSpacing );
    
    mEnableKActionsBox.setPosition( 0, lineSpacing * 2 );
    mCursorModeSet->setPosition( 0, 0 );
    mCursorScaleSlider.setPosition( -80, -lineSpacing * 3 );
    
    mMusicLoudnessSlider.setPosition( 0, lineSpacing / 2 );
    mSoundEffectsLoudnessSlider.setPosition( 0, - lineSpacing / 2 );
    
    mFullscreenBox.setPosition( 0, -lineSpacing );
    mBorderlessBox.setPosition( 0, -lineSpacing * 2 );
    mRedetectButton.setPosition( 160, lineSpacing * 2 );
    mRedetectButton.setPadding( 8, 4 );
    
    mEnableFOVBox.setVisible( mPage == 0 );
    mEnableCenterCameraBox.setVisible( mPage == 0 );
    mEnableNudeBox.setVisible( mPage == 0 );

    mEnableKActionsBox.setVisible( mPage == 1 );
    mCursorModeSet->setVisible( mPage == 1 );
    int mode = getCursorMode();
    mCursorScaleSlider.setVisible( mode > 0 && mCursorModeSet->isVisible() );
    

    mFullscreenBox.setVisible( mPage == 2 );
    mBorderlessBox.setVisible( mPage == 2 && mFullscreenBox.getToggled() );
    mRedetectButton.setVisible( mPage == 2 );

    mMusicLoudnessSlider.setVisible( mPage == 3 );
    mSoundEffectsLoudnessSlider.setVisible( mPage == 3 );


    mGameplayButton.setActive( mPage != 0 );
    mControlButton.setActive( mPage != 1 );
    mScreenButton.setActive( mPage != 2 );
    mSoundButton.setActive( mPage != 3 );
  

}

bool SettingsPage::checkRestartRequired() {
    if( mOldFullscreenSetting != mFullscreenBox.getToggled() ||
        mOldBorderlessSetting != mBorderlessBox.getToggled()
        ) {
        setStatusDirect( "RESTART REQUIRED##FOR NEW SETTINGS TO TAKE EFFECT", true );
        // Do not show RESTART button when setting page is accessed mid-game
        if ( ! showingInGameSettings )
        mRestartButton.setVisible( true );
        }
    else {
        setStatus( NULL, false );
        mRestartButton.setVisible( false );
        }
    }
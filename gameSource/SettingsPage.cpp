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


SettingsPage::SettingsPage()
        : mBackButton( mainFont, -542, -280, translate( "backButton" ) ),
          mEditAccountButton( mainFont, -463, 129, translate( "editAccount" ) ),
          mRestartButton( mainFont, -200, 110, translate( "restartButton" ) ),
          mRedetectButton( mainFont, 75, 265, translate( "redetectButton" ) ),
          mFullscreenBox( -335, 90, 4 ),
          mBorderlessBox( -335, 130, 4 ),
          mEnableNudeBox( -561, 0, 3 ),
          mEnableFOVBox( -561, -40, 3),
          mEnableKActionsBox( -561, -80, 3),
          mEnableCenterCameraBox( -561, -120, 3),
          mMusicLoudnessSlider( mainFont, 220, 75, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 220, 25, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ),
          mCursorScaleSlider( mainFont, 300, 145, 4, 200, 30,
                                       1.0, 10.0, 
                                       translate( "scale" ) ) {
                            

    
    const char *choiceList[3] = { translate( "system" ),
                                  translate( "drawn" ),
                                  translate( "both" ) };
    
    mCursorModeSet = 
        new RadioButtonSet( mainFont, 561, 275,
                            3, choiceList,
                            false, 4 );
    addComponent( mCursorModeSet );
    mCursorModeSet->addActionListener( this );

    addComponent( &mCursorScaleSlider );
    mCursorScaleSlider.addActionListener( this );

    mCursorScaleSlider.toggleField( false );


    setButtonStyle( &mBackButton );
    setButtonStyle( &mEditAccountButton );
    setButtonStyle( &mRestartButton );
    setButtonStyle( &mRedetectButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mEditAccountButton );
    mEditAccountButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );

    addComponent( &mEnableNudeBox );
    mEnableNudeBox.addActionListener( this );
	
	addComponent( &mEnableFOVBox );
    mEnableFOVBox.addActionListener( this );
	
	addComponent( &mEnableKActionsBox );
    mEnableKActionsBox.addActionListener( this );
	
	addComponent( &mEnableCenterCameraBox );
    mEnableCenterCameraBox.addActionListener( this );

    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );

    //addComponent( &mSpawnSeed );
    mRestartButton.setVisible( false );
    
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
    

    addComponent( &mMusicLoudnessSlider );
    mMusicLoudnessSlider.toggleField( false );
    addComponent( &mSoundEffectsLoudnessSlider );
    mSoundEffectsLoudnessSlider.toggleField( false );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );

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
        
        mRestartButton.setVisible( mOldFullscreenSetting != newSetting );
        
        mBorderlessBox.setVisible( newSetting );
        }
    else if( inTarget == &mBorderlessBox ) {
        int newSetting = mBorderlessBox.getToggled();
        
        SettingsManager::setSetting( "borderless", newSetting );
        
        mRestartButton.setVisible( mOldBorderlessSetting != newSetting );
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
        
        int mode = getCursorMode();
        
        mCursorScaleSlider.setVisible( mode > 0 );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );
        }
    else if( inTarget == &mCursorScaleSlider ) {
        setEmulatedCursorScale( mCursorScaleSlider.getValue() );
        }
    
    
    }


extern int targetFramesPerSecond;


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= mainFont->measureString( translate( "fullscreen" ) ) + 45;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignLeft );


    if( mBorderlessBox.isVisible() ) {
        pos = mBorderlessBox.getPosition();
    
        pos.x -= mainFont->measureString( translate( "fullscreen" ) ) + 45;
        pos.y -= 2;
        
        mainFont->drawString( translate( "borderless" ), pos, alignLeft );
        }
    

    pos = mFullscreenBox.getPosition();
    
    pos.y += 130;
    pos.x += 160;
    
    if( getCountingOnVsync() ) {
        mainFont->drawString( translate( "vsyncYes" ), pos, alignLeft );
        }
    else {
        mainFont->drawString( translate( "vsyncNo" ), pos, alignLeft );
        }
    
    pos.y += 44;

    char *fpsString = autoSprintf( "%d", targetFramesPerSecond );
    
    mainFont->drawString( fpsString, pos, alignLeft );
    delete [] fpsString;


    pos.y += 44;

    char *currentFPSString = autoSprintf( "%.2f", getRecentFrameRate() );
    
    mainFont->drawString( currentFPSString, pos, alignLeft );
    delete [] currentFPSString;
    

    pos = mFullscreenBox.getPosition();
    pos.x -= mainFont->measureString( translate( "fullscreen" ) ) + 45;

    pos.y += 130;
    mainFont->drawString( translate( "vsyncOn" ), pos, alignLeft );
    pos.y += 44;
    mainFont->drawString( translate( "targetFPS" ), pos, alignLeft );
    pos.y += 44;
    mainFont->drawString( translate( "currentFPS" ), pos, alignLeft );


    pos = mEnableNudeBox.getPosition();
    
    pos.x += 45;
    pos.y -= 2;

    mainFont->drawString( "Enable Nudity", pos, alignLeft );

    pos = mEnableFOVBox.getPosition();
    
    pos.x += 45;
    pos.y -= 2;

    mainFont->drawString( "Adjustable FOV", pos, alignLeft );

    pos = mEnableKActionsBox.getPosition();
    
    pos.x += 45;
    pos.y -= 2;

    mainFont->drawString( "Keyboard Actions", pos, alignLeft );
    
    pos = mEnableCenterCameraBox.getPosition();
    
    pos.x += 45;
    pos.y -= 2;

    mainFont->drawString( "Center Camera", pos, alignLeft );

    pos = mMusicLoudnessSlider.getPosition();

    pos.x += 52;
    pos.y -= 2;

    mainFont->drawString( translate( "musicLoudness"), pos, alignRight );

    pos = mSoundEffectsLoudnessSlider.getPosition();

    pos.x += 52;
    pos.y -= 2;

    mainFont->drawString( translate( "soundLoudness"), pos, alignRight );


    pos = mCursorModeSet->getPosition();
    
    pos.y += 37;
    mainFont->drawString( translate( "cursor"), pos, alignRight );
    
    if( mCursorScaleSlider.isVisible() ) {
        
        pos = mCursorScaleSlider.getPosition();
        
        pos.x += 52;
        pos.y -= 2;
        
        mainFont->drawString( translate( "scale"), pos, alignRight );
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
        
        mCursorScaleSlider.setVisible( mode > 0 );
        
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

        }
    }



void SettingsPage::makeNotActive() {
    }


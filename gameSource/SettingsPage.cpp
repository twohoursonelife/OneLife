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

#ifdef USE_DISCORD
#include "DiscordController.h"
#endif // USE_DISCORD

#include "DropdownList.h"

extern Font * mainFont;

extern float musicLoudness;

extern int targetFramesPerSecond;

extern bool showingInGameSettings;

extern float gui_fov_target_scale_hud;

// defined in LivingLifePage.cpp
extern bool ShowUseOnObjectHoverSettingToggle;
extern bool isShowUseOnObjectHoverKeybindEnabled;

extern char coordinatesEnabled;
extern char persistentEmoteEnabled;
extern char yumFinderEnabled;
extern char objectSearchEnabled;
extern char familyDisplayEnabled;
extern char dangerousTileEnabled;
extern char showPipsOfFoodHeld;

extern char *commandShortcutsRawText;

#ifdef USE_DISCORD
// extern from DiscordController.h
DiscordController *discordControllerInstance; 
#include <ctime>
time_t last_discord_setting_change = 0;
#endif // USE_DISCORD

SettingsPage::SettingsPage()
        : mBackground( "background.tga", 0.75f ),
          
          // Left Pane
          mRestartButton( mainFont, 360, -272, translate( "restartButton" ) ),
          
          mGameplayButton( mainFont, -452.5, 288, "GAMEPLAY" ),
          mControlButton( mainFont, -452.5, 192, "CONTROL" ),
          mScreenButton( mainFont, -452.5, 96, "SCREEN" ),
          mSoundButton( mainFont, -452.5, 0, "SOUND" ),
#ifdef USE_DISCORD
          mDiscordButton( mainFont, -452.5, -96, "DISCORD" ),
#endif // USE_DISCORD
          mAdvancedButton( mainFont, -452.5, -192, "ADVANCED" ),

          mBackButton( mainFont, -452.5, -288, translate( "backButton" ) ),
          
          mEditAccountButton( mainFont, -463, 129, translate( "editAccount" ) ),

          // Gameplay
          mEnableFOVBox( 561, 128, 4 ),
          mEnableCenterCameraBox( 561, 52, 4 ),
          mEnableNudeBox( -335, 148, 4 ),
          mUISizeSlider( mainFont, -335, 148, 4, 200, 30,
                                       0.5, 1.0, 
                                       "" ),
          
          mUseCustomServerBox( -168, -148, 4 ),
          mCustomServerAddressField( mainFont, 306, -150, 14, false, 
                                     "",
                                     NULL,
                                     // forbid spaces
                                     " " ),
          mCustomServerPortField( mainFont, 84, -208, 4, false, 
                                  "",
                                  "0123456789", NULL ),
          mCopyButton( mainFont, 381, -216, translate( "copy" ) ),
          mPasteButton( mainFont, 518, -216, translate( "paste" ) ),
          
          // Control
          mEnableKActionsBox( 561, 90, 4 ),
          mCursorScaleSlider( mainFont, 297, 155, 4, 200, 30,
                                       1.0, 10.0, 
                                       translate( "scale" ) ),
          
          // Screen
          mRedetectButton( mainFont, 153, 249, translate( "redetectButton" ) ),
          mVsyncBox( 0, 208, 4 ),
          mFullscreenBox( 0, 128, 4 ),
          mBorderlessBox( 0, 168, 4 ),
          mTargetFrameRateField( mainFont, -16, 232, 3, false, 
                                 "", // no label
                                 "0123456789",
                                 NULL ),
          mTrippingEffectDisabledBox( 0, 168, 4 ),
          
          // Sound
          mMusicLoudnessSlider( mainFont, 0, 40, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ), true ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -48, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ), true )
#ifdef USE_DISCORD
        , mEnableDiscordRichPresence(0, 168, 4),
          mEnableDiscordRichPresenceStatus(0, 128, 4), 
          mEnableDiscordShowAgeInStatus(0, 88, 4), 
          mEnableDiscordRichPresenceDetails(0, 48, 4),
          mDiscordHideFirstNameInDetails(0, 8, 4) 
#endif // USE_DISCORD
        , mCommandShortcuts( mainFont, -360, -176, 10, true, 
                                     "",
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?!/ 0123456789",
                                     NULL, 4 ),
        mEnableAdvancedShowUseOnObjectHoverKeybind(0, 168, 4),
        mEnableCoordinatesBox(0, 128, 4),
        mEnablePersistentEmoteBox(0, 88, 4),
        mEnableYumFinderBox(0, 48, 4),
        mEnableObjectSearchBox(0, 8, 4),
        mEnableFamilyDisplayBox(0, -32, 4),
        mEnableDangerousTileBox(0, -72, 4),
        mGenerateTownPlannerMapsBox( 561, 52, 4 ),
        mEnableShowingHeldFoodPips( 561, 52, 4 ) {
                            

    
    // Adding components below in reverse order so cursor tip is drawn on top
    
    addComponent( &mBackground );
    
    // Advanced
    addComponent( &mEnableShowingHeldFoodPips );
    mEnableShowingHeldFoodPips.addActionListener( this );
    addComponent( &mGenerateTownPlannerMapsBox );
    mGenerateTownPlannerMapsBox.addActionListener( this );
    addComponent(&mEnableDangerousTileBox);
    mEnableDangerousTileBox.addActionListener(this);
    addComponent(&mEnableFamilyDisplayBox);
    mEnableFamilyDisplayBox.addActionListener(this);
    addComponent(&mEnableObjectSearchBox);
    mEnableObjectSearchBox.addActionListener(this);
    addComponent(&mEnableYumFinderBox);
    mEnableYumFinderBox.addActionListener(this);
    addComponent(&mEnablePersistentEmoteBox);
    mEnablePersistentEmoteBox.addActionListener(this);
    addComponent(&mEnableCoordinatesBox);
    mEnableCoordinatesBox.addActionListener(this);
    addComponent(&mEnableAdvancedShowUseOnObjectHoverKeybind);
    mEnableAdvancedShowUseOnObjectHoverKeybind.addActionListener(this);
    addComponent(&mCommandShortcuts);
    mCommandShortcuts.addActionListener(this);
    mCommandShortcuts.setWidth( 360 );
    mCommandShortcuts.useClearButton( true );
    mCommandShortcuts.setFireOnLoseFocus( true );
    
#ifdef USE_DISCORD
    // Discord
    addComponent(&mDiscordHideFirstNameInDetails);
    mDiscordHideFirstNameInDetails.addActionListener(this);
    addComponent(&mEnableDiscordRichPresenceDetails);
    mEnableDiscordRichPresenceDetails.addActionListener(this);
    addComponent(&mEnableDiscordShowAgeInStatus);
    mEnableDiscordShowAgeInStatus.addActionListener(this);
    addComponent(&mEnableDiscordRichPresenceStatus);
    mEnableDiscordRichPresenceStatus.addActionListener(this);
    addComponent(&mEnableDiscordRichPresence);
    mEnableDiscordRichPresence.addActionListener(this);
#endif // USE_DISCORD
    
    // Sound
    addComponent( &mSoundEffectsLoudnessSlider );
    mSoundEffectsLoudnessSlider.addActionListener( this );
    addComponent( &mMusicLoudnessSlider );
    mMusicLoudnessSlider.addActionListener( this );
    
    // Screen
    addComponent( &mTrippingEffectDisabledBox );
    mTrippingEffectDisabledBox.addActionListener( this );
    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );
    addComponent( &mTargetFrameRateField );
    mTargetFrameRateField.addActionListener( this );
    mTargetFrameRateField.setFireOnAnyTextChange( true );
    addComponent( &mVsyncBox );
    mVsyncBox.addActionListener( this );
    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );
    setButtonStyle( &mRedetectButton );
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );
    
    mTargetFrameRateField.setInt( targetFramesPerSecond );

    if( getCountingOnVsync() ) {
        mTargetFrameRateField.setVisible( false );
        }
    
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
                            false, 4, true );
    addComponent( mCursorModeSet );
    mCursorModeSet->addActionListener( this );
    
    addComponent( &mEnableKActionsBox );
    mEnableKActionsBox.addActionListener( this );
    
    // Gameplay
    setButtonStyle( &mPasteButton );
    addComponent( &mPasteButton );
    mPasteButton.addActionListener( this );
    setButtonStyle( &mCopyButton );
    addComponent( &mCopyButton );
    mCopyButton.addActionListener( this );
    addComponent( &mCustomServerPortField );
    mCustomServerPortField.setWidth( 360 );
    mCustomServerPortField.setHigh( 40 );
    addComponent( &mCustomServerAddressField );    
    mCustomServerAddressField.setWidth( 360 );
    mCustomServerAddressField.setHigh( 40 );
    addComponent( &mUseCustomServerBox );
    mUseCustomServerBox.addActionListener( this );
    
    addComponent( &mUISizeSlider );
    mUISizeSlider.addActionListener( this );
    addComponent( &mEnableNudeBox );
    mEnableNudeBox.addActionListener( this );
    addComponent( &mEnableCenterCameraBox );
    mEnableCenterCameraBox.addActionListener( this );
    addComponent( &mEnableFOVBox );
    mEnableFOVBox.addActionListener( this );
    
    // Left pane
    setButtonStyle(&mAdvancedButton);
    mAdvancedButton.setSize(175, 60);
    addComponent(&mAdvancedButton);
    mAdvancedButton.addActionListener(this);
    
#ifdef USE_DISCORD
    setButtonStyle(&mDiscordButton);
    mDiscordButton.setSize(175, 60);
    addComponent(&mDiscordButton);
    mDiscordButton.addActionListener(this);
#endif // USE_DISCORD
    
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
#ifdef USE_DISCORD
    mDiscordButton.setCursorTip("DISCORD RICH PRESENCE SETTINGS");
#endif // USE_DISCORD
    mAdvancedButton.setCursorTip("ADVANCED GAMEPLAY SETTINGS");

    mBackButton.setCursorTip( "GO BACK" );
    
    mEnableFOVBox.setCursorTip( "ENABLE ZOOM-IN AND ZOOM-OUT WITH MOUSE WHEEL SCROLLING" );
    mEnableCenterCameraBox.setCursorTip( "ALWAYS CENTER THE CAMERA VIEW ON YOUR CHARACTER" );
    mEnableNudeBox.setCursorTip( "ENABLE NUDITY" );
    
    mUseCustomServerBox.setCursorTip( "CONNECT TO A CUSTOM SERVER" );
    mCustomServerAddressField.setCursorTip( "CUSTOM SERVER ADDRESS" );
    mCustomServerPortField.setCursorTip( "CUSTOM SERVER PORT" );
    
    mEnableKActionsBox.setCursorTip( "ENABLE WASD MOVEMENT AND ACTION" );
    mCursorModeSet->setCursorTip( "USE DRAWN CURSOR FOR ULTRAWIDE SCREEN" );
    
    mRedetectButton.setCursorTip( "RESTART THE GAME TO REDETECT FRAME RATE" );
    mFullscreenBox.setCursorTip( "TOGGLE BETWEEN FULLSCREEN AND WINDOWED MODE" );
    mBorderlessBox.setCursorTip( "ALLOW CURSOR TO MOVE OUTSIDE THE GAME WINDOW" );
    mTrippingEffectDisabledBox.setCursorTip( "DISABLE THE TRIPPING VISUAL EFFECT" );

#ifdef USE_DISCORD
    // Discord
    mEnableDiscordRichPresence.setCursorTip("SHOW \"PLAYING A GAME\" STATUS IN YOUR DISCORD PROFILE STATUS");
    mEnableDiscordRichPresenceStatus.setCursorTip("SHOW AGE, GENDER AND IDLE STATUS OF CURRENT LIFE");
    mEnableDiscordShowAgeInStatus.setCursorTip("SELECTIVELY TOGGLE ON OR OFF AGE DISPLAY IN THE STATUS HERE");
    mEnableDiscordRichPresenceDetails.setCursorTip("SHOW FIRST NAME AND FAMILY NAME OF CURRENT LIFE");
    mDiscordHideFirstNameInDetails.setCursorTip("HIDE FIRST NAME IN THE STATUS");
#endif // USE_DISCORD

    mCommandShortcuts.setCursorTip( "SAVED COMMANDS OR SPEECH, TO BE ACCESSED WITH NUM KEYS OR ALT + NUM KEYS." );
    mEnableAdvancedShowUseOnObjectHoverKeybind.setCursorTip(
      "SHOW OBJECT REMAINING USE ON CURSOR HOVER. SHIFT+B TO ENABLE/DISABLE IN-GAME");
    mEnableCoordinatesBox.setCursorTip( "ENABLE COORDINATES DISPLAY AND SAVING. PRESS G TO TOGGLE PANEL." );
    mEnablePersistentEmoteBox.setCursorTip( "ENABLE PERMANENT EMOTE" );
    mEnableYumFinderBox.setCursorTip( "ENABLE YUM FINDER. PRESS Y TO SHOW YUM" );
    mEnableObjectSearchBox.setCursorTip( "ENABLE OBJECT FINDER. PRESS J TO TOGGLE PANEL." );
    mEnableFamilyDisplayBox.setCursorTip( "ENABLE DISPLAY OF LIST OF FAMILIES. PRESS P TO TOGGLE PANEL." );
    mEnableDangerousTileBox.setCursorTip( "HIGHLIGHT DANGEROUS TILES AND BLOCK PATHING INTO THEM ON KEYBOARD." );
    mGenerateTownPlannerMapsBox.setCursorTip( "SAVE MAP FILES TO BE USED IN TOWN PLANNER" );
    mEnableShowingHeldFoodPips.setCursorTip( "SHOW FOOD PIPS OF THE FOOD YOU'RE HOLDING" );
    
    mOldFullscreenSetting = 
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mTestSound = blankSoundUsage;

    mMusicStartTime = 0;

    
    mVsyncBox.setToggled( getCountingOnVsync() );

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    mBorderlessBox.setVisible( mOldFullscreenSetting );


    mOldBorderlessSetting = 
        SettingsManager::getIntSetting( "borderless", 0 );

    mBorderlessBox.setToggled( mOldBorderlessSetting );
    
    mTrippingEffectDisabledSetting =
        SettingsManager::getIntSetting( "trippingEffectDisabled", 1 );

    mTrippingEffectDisabledBox.setToggled( mTrippingEffectDisabledSetting );

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

#ifdef USE_DISCORD
    mDiscordRichPresenceSetting =
        SettingsManager::getIntSetting("discordRichPresence", 1);

    mEnableDiscordRichPresence.setToggled(mDiscordRichPresenceSetting);

    mDiscordRichPresenceStatusSetting =
        SettingsManager::getIntSetting("discordRichPresenceStatus", 1);

    mDiscordShowAgeInStatusSetting =
        SettingsManager::getIntSetting("discordRichPresenceShowAge", 1);

    mEnableDiscordShowAgeInStatus.setToggled(mDiscordShowAgeInStatusSetting);

    mEnableDiscordRichPresenceStatus.setToggled(mDiscordRichPresenceStatusSetting);

    mDiscordRichPresenceDetailsSetting =
        SettingsManager::getIntSetting("discordRichPresenceDetails", 0);

    mEnableDiscordRichPresenceDetails.setToggled(mDiscordRichPresenceDetailsSetting);

    mDiscordHideFirstNameInDetailsSetting =
        SettingsManager::getIntSetting("discordRichPresenceHideFirstName", 1);

    mDiscordHideFirstNameInDetails.setToggled(mDiscordHideFirstNameInDetailsSetting);
#endif // USE_DISCORD

    commandShortcutsRawText = 
        SettingsManager::getSettingContents( "commandShortcuts", "" );

    mCommandShortcuts.setListByRawText( commandShortcutsRawText );

    // the UI is used to view and edit a list
    // the textbox is merely a way to input text
    // keep it empty when the UI is not focused
    mCommandShortcuts.setText( "" );
    
    mAdvancedShowUseOnObjectHoverKeybindSetting = 
        SettingsManager::getIntSetting("showUseOnObjectHoverKeybind", 0);

    mEnableAdvancedShowUseOnObjectHoverKeybind.setToggled(
        mAdvancedShowUseOnObjectHoverKeybindSetting);

    coordinatesEnabled = 
        SettingsManager::getIntSetting("coordinatesEnabled", 0);

    mEnableCoordinatesBox.setToggled(
        coordinatesEnabled);

    persistentEmoteEnabled = 
        SettingsManager::getIntSetting("persistentEmoteEnabled", 0);

    mEnablePersistentEmoteBox.setToggled(
        persistentEmoteEnabled);

    yumFinderEnabled = SettingsManager::getIntSetting("yumFinderEnabled", 0);

    mEnableYumFinderBox.setToggled( yumFinderEnabled );

    objectSearchEnabled = SettingsManager::getIntSetting("objectSearchEnabled", 0);

    mEnableObjectSearchBox.setToggled( objectSearchEnabled );

    familyDisplayEnabled = SettingsManager::getIntSetting("familyDisplayEnabled", 0);

    mEnableFamilyDisplayBox.setToggled( familyDisplayEnabled );

    dangerousTileEnabled = SettingsManager::getIntSetting("dangerousTileEnabled", 0);

    mEnableDangerousTileBox.setToggled( dangerousTileEnabled );

    mOldGenerateTownPlannerMapsSetting = SettingsManager::getIntSetting( "generateTownPlannerMaps", 0 );

    mGenerateTownPlannerMapsBox.setToggled( mOldGenerateTownPlannerMapsSetting );

    showPipsOfFoodHeld = SettingsManager::getIntSetting( "showPipsOfFoodHeldEnabled", 0 );

    mEnableShowingHeldFoodPips.setToggled( showPipsOfFoodHeld );
    
    

    mPage = 0;
    




    }



SettingsPage::~SettingsPage() {
    clearSoundUsage( &mTestSound );

    delete mCursorModeSet;
    }



void SettingsPage::checkRestartButtonVisibility() {
    int newVsyncSetting = 
        SettingsManager::getIntSetting( "countingOnVsync", -1 );
    
    int newFullscreenSetting =
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    int newBorderlessSetting =
        SettingsManager::getIntSetting( "borderless", 0 );

    if( getCountingOnVsync() != newVsyncSetting
        ||
        mOldFullscreenSetting != newFullscreenSetting
        ||
        mOldBorderlessSetting != newBorderlessSetting 
        ||
        ( mTargetFrameRateField.isVisible() && 
          mTargetFrameRateField.getInt() != targetFramesPerSecond ) ) {

        mRestartButton.setVisible( true );
        }
    else {
        mRestartButton.setVisible( false );
        }
    }


void SettingsPage::actionPerformed( GUIComponent *inTarget ) {

    if( inTarget == &mBackButton ) {
        
        int useCustomServer = 0;
        if( mUseCustomServerBox.getToggled() ) {
            useCustomServer = 1;
            }
        
        SettingsManager::setSetting( "useCustomServer", useCustomServer );
        char *address = mCustomServerAddressField.getText();
        
        SettingsManager::setSetting( "customServerAddress", address );
        delete [] address;
        
        SettingsManager::setSetting( "customServerPort",
                                     mCustomServerPortField.getInt() );
        
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mEditAccountButton ) {
        setSignal( "editAccount" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mTargetFrameRateField ) {
        
        int newValue = mTargetFrameRateField.getInt();
        
        if( newValue > 0 ) {
            
            // if we're manually manipulating target frame rate
            // any tweaked halfFrameRate setting should be cleared
            SettingsManager::setSetting( "halfFrameRate", 0 );

            SettingsManager::setSetting( "targetFrameRate", newValue );

            // checkRestartButtonVisibility();
            }
        }
    else if( inTarget == &mVsyncBox ) {
        int newSetting = mVsyncBox.getToggled();
        
        SettingsManager::setSetting( "countingOnVsync", newSetting );
        
        // checkRestartButtonVisibility();

        if( ! newSetting ) {
            mTargetFrameRateField.setVisible( true );
            mTargetFrameRateField.setInt( targetFramesPerSecond );
            }
        else {
            mTargetFrameRateField.setVisible( false );
            
            // if we're manually manipulating target frame rate
            // any tweaked halfFrameRate setting should be cleared
            SettingsManager::setSetting( "halfFrameRate", 0 );
            SettingsManager::setSetting( "targetFramesPerSecond",
                                         targetFramesPerSecond );
            }
        }
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        
        if( ! newSetting ) {
            // can't be borderless if not fullscreen
            SettingsManager::setSetting( "borderless", 0 );
            mBorderlessBox.setToggled( false );
            }
        
            
        // checkRestartButtonVisibility();
        }
    else if( inTarget == &mBorderlessBox ) {
        int newSetting = mBorderlessBox.getToggled();
        
        SettingsManager::setSetting( "borderless", newSetting );
        
        // checkRestartButtonVisibility();
        }
    else if( inTarget == &mTrippingEffectDisabledBox ) {
        int newSetting = mTrippingEffectDisabledBox.getToggled();
        
        SettingsManager::setSetting( "trippingEffectDisabled", newSetting );
        
        trippingEffectDisabled = newSetting;
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
    else if( inTarget == &mUISizeSlider ) {
            
        gui_fov_target_scale_hud = 1 / mUISizeSlider.getValue();
        SettingsManager::setSetting( "fovScaleHUD", gui_fov_target_scale_hud );

        }
    else if( inTarget == &mUseCustomServerBox ) {
        mCustomServerAddressField.setVisible( mPage == 0 && mUseCustomServerBox.getToggled() );
        mCustomServerPortField.setVisible( mPage == 0 && mUseCustomServerBox.getToggled() );
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
    else if( inTarget == &mRedetectButton ) {
        // redetect means start from scratch, detect vsync, etc.
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
    else if( inTarget == &mRestartButton ) {
             
        int newVsyncSetting = 
            SettingsManager::getIntSetting( "countingOnVsync", -1 );
        
        int newFullscreenSetting =
            SettingsManager::getIntSetting( "fullscreen", 1 );
        
        int newBorderlessSetting =
            SettingsManager::getIntSetting( "borderless", 0 );
        

        if( ( newVsyncSetting == 1 && ! getCountingOnVsync() )
            ||
            ( newVsyncSetting == 1 && 
              newFullscreenSetting != mOldFullscreenSetting )
            ||
            ( newVsyncSetting == 1 && 
              newBorderlessSetting != mOldBorderlessSetting ) ) {
            
            // re-measure frame rate after relaunch
            // but only if counting on vsync AND if one of these things changed
            
            // 1.  We weren't counting on vsync before
            // 2.  We've toggled fullscreen mode
            // 3.  We've toggled borderless mode
            
            // if we're not counting on vsync, we shouldn't re-measure
            // because measuring is only sensible if we're trying to detect
            // our vsync-enforced frame rate
            SettingsManager::setSetting( "targetFrameRate", -1 );
            }
        

        // now that end-user can toggle vsync here, never clear
        // the vsync setting when we re-launch


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
    else if( inTarget == &mCopyButton ) {
        char *address = mCustomServerAddressField.getText();
        
        char *fullAddress = autoSprintf( "%s:%d", address,
                                         mCustomServerPortField.getInt() );
        delete [] address;
        
        setClipboardText( fullAddress );
        
        delete [] fullAddress;
        }
    else if( inTarget == &mPasteButton ) {
        char *text = getClipboardText();

        char *trimmed = trimWhitespace( text );
        
        delete [] text;
        

        char setWithPort = false;
        
        if( strstr( trimmed, ":" ) != NULL ) {
            char addressBuff[100];
            int port = 0;
            
            int numRead = sscanf( trimmed, "%99[^:]:%d", addressBuff, &port );
            
            if( numRead == 2 ) {
                setWithPort = true;
                
                char *trimmedAddr = trimWhitespace( addressBuff );
                
                // terminate at first space, if any
                char *spacePos = strstr( trimmedAddr, " " );
                if( spacePos != NULL ) {
                    spacePos[0] = '\0';
                    }

                mCustomServerAddressField.setText( trimmedAddr );

                delete [] trimmedAddr;
                
                mCustomServerPortField.setInt( port );
                }
            }
        
        if( ! setWithPort ) {
            // treat the whole thing as an address
            
            // terminate at first space, if any
            char *spacePos = strstr( trimmed, " " );
            
            if( spacePos != NULL ) {
                spacePos[0] = '\0';
                }
            mCustomServerAddressField.setText( trimmed );
            }
        delete [] trimmed;
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

#ifdef USE_DISCORD
    else if( inTarget == &mDiscordButton ) {
        mPage = 4;
        }
    else if( inTarget == &mEnableDiscordRichPresence ) {
        int newSetting = mEnableDiscordRichPresence.getToggled();
        last_discord_setting_change = time(0);
        mDiscordRichPresenceSetting = newSetting;
        SettingsManager::setSetting("discordRichPresence", newSetting);
        if(discordControllerInstance != NULL) {
            discordControllerInstance->updateDisplayGame((char)newSetting);
            }
        }
    else if( inTarget == &mEnableDiscordRichPresenceStatus ) {
        last_discord_setting_change = time(0);
        int newSetting = mEnableDiscordRichPresenceStatus.getToggled();
        mDiscordRichPresenceStatusSetting = newSetting;
        SettingsManager::setSetting("discordRichPresenceStatus", newSetting);
        if(discordControllerInstance != NULL) {
            discordControllerInstance->updateDisplayStatus((char)newSetting);
            }
        }
    else if( inTarget == &mEnableDiscordShowAgeInStatus ) {
        last_discord_setting_change = time(0);
        int newSetting = mEnableDiscordShowAgeInStatus.getToggled();
        mDiscordShowAgeInStatusSetting = newSetting;
        SettingsManager::setSetting("discordRichPresenceShowAge", newSetting);
        if(discordControllerInstance != NULL) {
            discordControllerInstance->updateDisplayAge((char)newSetting);
            }
        }
    else if( inTarget == &mEnableDiscordRichPresenceDetails ) {
        last_discord_setting_change = time(0);
        int newSetting = mEnableDiscordRichPresenceDetails.getToggled();
        mDiscordRichPresenceDetailsSetting = newSetting;
        SettingsManager::setSetting("discordRichPresenceDetails", newSetting);
        if(discordControllerInstance != NULL) {
            discordControllerInstance->updateDisplayDetails((char)newSetting);
            }
        }
    else if( inTarget == &mDiscordHideFirstNameInDetails ) {
        last_discord_setting_change = time(0);
        int newSetting = mDiscordHideFirstNameInDetails.getToggled();
        mDiscordHideFirstNameInDetailsSetting = newSetting;
        SettingsManager::setSetting("discordRichPresenceHideFirstName", newSetting);
        if(discordControllerInstance != NULL) {
            discordControllerInstance->updateDisplayFirstName((char)!newSetting);
            }
        }        
#endif // USE_DISCORD

    else if ( inTarget == &mAdvancedButton ) {
        mPage = 5;
        }
    else if( inTarget == &mCommandShortcuts ) {
        commandShortcutsRawText = mCommandShortcuts.getAndUpdateRawText();
        SettingsManager::setSetting( "commandShortcuts", commandShortcutsRawText );
        if( !mCommandShortcuts.isFocused() ) {
            // the UI is used to view and edit a list
            // the textbox is merely a way to input text
            // keep it empty when the UI is not focused
            mCommandShortcuts.setText( "" );
            }
        }
    else if ( inTarget == &mEnableAdvancedShowUseOnObjectHoverKeybind ) {
        int newSetting = mEnableAdvancedShowUseOnObjectHoverKeybind.getToggled();
        mAdvancedShowUseOnObjectHoverKeybindSetting = newSetting;
        SettingsManager::setSetting("showUseOnObjectHoverKeybind",
                                    newSetting);
        ShowUseOnObjectHoverSettingToggle = (bool)newSetting;
        if( ShowUseOnObjectHoverSettingToggle ) isShowUseOnObjectHoverKeybindEnabled = true;
        }
    else if ( inTarget == &mEnableCoordinatesBox ) {
        int newSetting = mEnableCoordinatesBox.getToggled();
        coordinatesEnabled = false;
        if( newSetting ) coordinatesEnabled = true;
        SettingsManager::setSetting("coordinatesEnabled",
                                    newSetting);
        }
    else if ( inTarget == &mEnablePersistentEmoteBox ) {
        int newSetting = mEnablePersistentEmoteBox.getToggled();
        persistentEmoteEnabled = false;
        if( newSetting ) persistentEmoteEnabled = true;
        SettingsManager::setSetting("persistentEmoteEnabled",
                                    newSetting);
        }
    else if ( inTarget == &mEnableYumFinderBox ) {
        int newSetting = mEnableYumFinderBox.getToggled();
        yumFinderEnabled = false;
        if( newSetting ) yumFinderEnabled = true;
        SettingsManager::setSetting("yumFinderEnabled",
                                    newSetting);
        }
    else if ( inTarget == &mEnableObjectSearchBox ) {
        int newSetting = mEnableObjectSearchBox.getToggled();
        objectSearchEnabled = false;
        if( newSetting ) objectSearchEnabled = true;
        SettingsManager::setSetting("objectSearchEnabled",
                                    newSetting);
        }
    else if ( inTarget == &mEnableFamilyDisplayBox ) {
        int newSetting = mEnableFamilyDisplayBox.getToggled();
        familyDisplayEnabled = false;
        if( newSetting ) familyDisplayEnabled = true;
        SettingsManager::setSetting("familyDisplayEnabled",
                                    newSetting);
        }
    else if ( inTarget == &mEnableDangerousTileBox ) {
        int newSetting = mEnableDangerousTileBox.getToggled();
        dangerousTileEnabled = false;
        if( newSetting ) dangerousTileEnabled = true;
        SettingsManager::setSetting("dangerousTileEnabled",
                                    newSetting);
        }
    else if( inTarget == &mGenerateTownPlannerMapsBox ) {
        int newSetting = mGenerateTownPlannerMapsBox.getToggled();
        
        SettingsManager::setSetting( "generateTownPlannerMaps", newSetting );
        }
    else if( inTarget == &mEnableShowingHeldFoodPips ) {
        int newSetting = mEnableShowingHeldFoodPips.getToggled();
        showPipsOfFoodHeld = false;
        if( newSetting ) showPipsOfFoodHeld = true;
        SettingsManager::setSetting( "showPipsOfFoodHeldEnabled", newSetting );
        }

    checkRestartRequired();
    updatePage();
    }


void drawTextWithShadow( const char *text, doublePair pos, TextAlignment alignment ) {
    doublePair shadowOffset = {-2, 2};
    setDrawColor( 0, 0, 0, 1 );
    mainFont->drawString( text, add(pos, shadowOffset), alignment );
    setDrawColor( 1, 1, 1, 1 );
    mainFont->drawString( text, pos, alignment );
    }


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    if( mFullscreenBox.isVisible() ) {
        
        doublePair pos = mVsyncBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;
        
        drawTextWithShadow( translate( "vsyncOn" ), pos, alignRight );
        
        pos = mFullscreenBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;
        
        drawTextWithShadow( translate( "fullscreen" ), pos, alignRight );


        if( mBorderlessBox.isVisible() ) {
            pos = mBorderlessBox.getPosition();
        
            pos.x -= 30;
            pos.y -= 2;
            
            drawTextWithShadow( translate( "borderless" ), pos, alignRight );
            }


        if( mTrippingEffectDisabledBox.isVisible() ) {
            pos = mTrippingEffectDisabledBox.getPosition();
        
            pos.x -= 30;
            pos.y -= 2;
            
            drawTextWithShadow( "DISABLE FLASH EFFECT", pos, alignRight );
            }
        

        pos = mFullscreenBox.getPosition();
        
        pos.y += 52;
        pos.x -= 16;
        
        pos.y += 52;


        if( ! mTargetFrameRateField.isVisible() ) {
            char *fpsString = autoSprintf( "%d", targetFramesPerSecond );
            
            drawTextWithShadow( fpsString, pos, alignLeft );
            delete [] fpsString;
            }


        pos.y += 52;

        char *currentFPSString = autoSprintf( "%.2f", getRecentFrameRate() );
        
        drawTextWithShadow( currentFPSString, pos, alignLeft );
        delete [] currentFPSString;
        

        pos = mFullscreenBox.getPosition();
        pos.x -= 30;

        pos.y += 52;
        
        pos.y += 52;
        drawTextWithShadow( translate( "targetFPS" ), pos, alignRight );
        pos.y += 52;
        drawTextWithShadow( translate( "currentFPS" ), pos, alignRight );
        }


    if( mEnableNudeBox.isVisible() ) {
        doublePair pos = mEnableNudeBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "ENABLE NUDITY", pos, alignRight );
        }
    if( mUISizeSlider.isVisible() ) {
        doublePair pos = mEnableNudeBox.getPosition();
        
        pos.x -= 30;
        pos.y = mUISizeSlider.getPosition().y;
        pos.y -= 2;

        drawTextWithShadow( "UI SIZE", pos, alignRight );
        }
        
    if( mUseCustomServerBox.isVisible() ) {
        doublePair pos = mUseCustomServerBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( translate( "useCustomServer" ), pos, alignRight );
        
        if( mUseCustomServerBox.getToggled() ) {
            pos.y += 2;
            pos.y -= 52 + 52/4;
            drawTextWithShadow( translate( "address" ), pos, alignRight );
            pos.y -= 52 + 52/4;
            drawTextWithShadow( translate( "port" ), pos, alignRight );
            }
        }
    
    if( mEnableFOVBox.isVisible() ) {
        doublePair pos = mEnableFOVBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "ENABLE ZOOM", pos, alignRight );
        }
    
    if( mEnableKActionsBox.isVisible() ) {
        doublePair pos = mEnableKActionsBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "KEYBOARD ACTIONS", pos, alignRight );
        }
    
    if( mEnableCenterCameraBox.isVisible() ) {
        doublePair pos = mEnableCenterCameraBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "CENTER CAMERA", pos, alignRight );
        }


    if( mCursorModeSet->isVisible() ) {
        
        double xPos = mEnableCenterCameraBox.getPosition().x - 30;
        
        doublePair pos = mCursorModeSet->getPosition();
        
        pos.x = xPos;
        pos.y += 37;
        
        drawTextWithShadow( translate( "cursor"), pos, alignRight );
        
        if( mCursorScaleSlider.isVisible() ) {
            
            pos = mCursorScaleSlider.getPosition();
            
            pos.x = xPos;
            pos.y -= 2;
            
            drawTextWithShadow( translate( "scale"), pos, alignRight );
            }
        }
#ifdef USE_DISCORD
    if( mEnableDiscordRichPresence.isVisible() ) {
        doublePair pos = mEnableDiscordRichPresence.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "RICH PRESENCE ", pos, alignRight );
        }
    if( mEnableDiscordRichPresenceStatus.isVisible() ) {
        doublePair pos = mEnableDiscordRichPresenceStatus.getPosition();

        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "RICH PRESENCE STATUS", pos, alignRight );
        }
    if( mEnableDiscordShowAgeInStatus.isVisible() ) {
        doublePair pos = mEnableDiscordShowAgeInStatus.getPosition();

        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "SHOW AGE", pos, alignRight );
        }
    if( mEnableDiscordRichPresenceDetails.isVisible() ) {
        doublePair pos = mEnableDiscordRichPresenceDetails.getPosition();

        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "RICH PRESENCE DETAILS", pos, alignRight );
        }
    if( mDiscordHideFirstNameInDetails.isVisible() ) {
        doublePair pos = mDiscordHideFirstNameInDetails.getPosition();

        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "HIDE FIRST NAME", pos, alignRight );
        }    
    // prevent someone from making spamming requests to update their status, discord may block our key, due to spamming requests!
    // allow for at least 2 seconds to pass until the user is allowed to change the discord setting again.
    mEnableDiscordRichPresence.setActive(time(0) - last_discord_setting_change > 2);
    mEnableDiscordRichPresenceStatus.setActive(time(0) - last_discord_setting_change > 2);
    mEnableDiscordShowAgeInStatus.setActive(time(0) - last_discord_setting_change > 2);
    mEnableDiscordRichPresenceDetails.setActive(time(0) - last_discord_setting_change > 2);
    mDiscordHideFirstNameInDetails.setActive(time(0) - last_discord_setting_change > 2);
#endif // USE_DISCORD

    if (mCommandShortcuts.isVisible()) {
        doublePair pos = mCommandShortcuts.getPosition();
        pos.x = mEnableAdvancedShowUseOnObjectHoverKeybind.getPosition().x - 30;
        pos.y -= 2;

        drawTextWithShadow("COMMAND SHORTCUTS", pos, alignRight);
        }
    if (mEnableAdvancedShowUseOnObjectHoverKeybind.isVisible()) {
        doublePair pos = mEnableAdvancedShowUseOnObjectHoverKeybind.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("SHOW USE ON HOVER", pos, alignRight);
        }
    if (mEnableCoordinatesBox.isVisible()) {
        doublePair pos = mEnableCoordinatesBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("SHOW COORDINATES", pos, alignRight);
        }
    if (mEnablePersistentEmoteBox.isVisible()) {
        doublePair pos = mEnablePersistentEmoteBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("USE PERMANENT EMOTE", pos, alignRight);
        }
    if (mEnableYumFinderBox.isVisible()) {
        doublePair pos = mEnableYumFinderBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("ENABLE YUM FINDER", pos, alignRight);
        }
    if (mEnableObjectSearchBox.isVisible()) {
        doublePair pos = mEnableObjectSearchBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("ENABLE OBJECT FINDER", pos, alignRight);
        }
    if (mEnableFamilyDisplayBox.isVisible()) {
        doublePair pos = mEnableFamilyDisplayBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("ENABLE FAMILY DISPLAY", pos, alignRight);
        }
    if (mEnableDangerousTileBox.isVisible()) {
        doublePair pos = mEnableDangerousTileBox.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("ENABLE DANGER HIGHLIGHT", pos, alignRight);
        }
    if( mGenerateTownPlannerMapsBox.isVisible() ) {
        doublePair pos = mGenerateTownPlannerMapsBox.getPosition();
        
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow( "SAVE MAP FILES", pos, alignRight );
        }
    if (mEnableShowingHeldFoodPips.isVisible()) {
        doublePair pos = mEnableShowingHeldFoodPips.getPosition();
        pos.x -= 30;
        pos.y -= 2;

        drawTextWithShadow("SHOW HELD FOOD PIPS", pos, alignRight);
        }
    }



void SettingsPage::step() {
    if( mTestSound.numSubSounds > 0 ) {
        markSoundUsageLive( mTestSound );
        }
    stepMusicPlayer();

    int blockClicks = false;
    if( mCommandShortcuts.isFocused() ) blockClicks = true;
    mEnableAdvancedShowUseOnObjectHoverKeybind.setIgnoreEvents( blockClicks );
    mEnableCoordinatesBox.setIgnoreEvents( blockClicks );
    mEnablePersistentEmoteBox.setIgnoreEvents( blockClicks );
    mEnableYumFinderBox.setIgnoreEvents( blockClicks );
    // mEnableObjectSearchBox.setIgnoreEvents( blockClicks );
    // mEnableFamilyDisplayBox.setIgnoreEvents( blockClicks );
    // mEnableDangerousTileBox.setIgnoreEvents( blockClicks );
    // mGenerateTownPlannerMapsBox.setIgnoreEvents( blockClicks );
    
    }





void SettingsPage::makeActive( char inFresh ) {
    
    checkRestartRequired();
    
    if( inFresh ) {        

        int mode = getCursorMode();
        
        mCursorModeSet->setSelectedItem( mode );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );


        int useCustomServer = 
            SettingsManager::getIntSetting( "useCustomServer", 0 );
        
        mUseCustomServerBox.setToggled( useCustomServer );
        

        char *address = 
            SettingsManager::getStringSetting( "customServerAddress",
                                               "localhost" );
        
        int port = SettingsManager::getIntSetting( "customServerPort", 8005 );
        
        mCustomServerAddressField.setText( address );
        mCustomServerPortField.setInt( port );
        
        delete [] address;
        


        mMusicLoudnessSlider.setValue( musicLoudness );
        mSoundEffectsLoudnessSlider.setValue( getSoundEffectsLoudness() );
        setMusicLoudness( 0 );
        mMusicStartTime = 0;

        mUISizeSlider.setValue( 1 / gui_fov_target_scale_hud );
        
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

    double lineSpacing = 52;
    
    mEnableFOVBox.setPosition( 0, lineSpacing * 4 );
    mEnableCenterCameraBox.setPosition( 0, lineSpacing * 3 );
    mEnableNudeBox.setPosition( 0, lineSpacing * 2 );
    mUISizeSlider.setPosition( 28, lineSpacing * 1 );
    mUseCustomServerBox.setPosition( 0, -lineSpacing );
    mCustomServerAddressField.setPosition( 180 - 16, -lineSpacing * 2 - lineSpacing/4 );
    mCustomServerPortField.setPosition( 180 - 16, -lineSpacing * 3  - lineSpacing/2 );
    mCopyButton.setPadding( 8, 4 );
    mCopyButton.setPosition( 
        mCustomServerAddressField.getRightEdgeX() + mCopyButton.getWidth() / 2 + 12, 
        mCustomServerAddressField.getPosition().y );
    mPasteButton.setPadding( 8, 4 );
    mPasteButton.setPosition(
        mCustomServerAddressField.getRightEdgeX() + 12 + 
        mCopyButton.getWidth() + 
        12 + mPasteButton.getWidth() / 2, 
        mCustomServerAddressField.getPosition().y );
    
    mEnableKActionsBox.setPosition( 0, lineSpacing * 2 );
    mCursorModeSet->setPosition( 0, 0 );
    mCursorScaleSlider.setPosition( -80, -lineSpacing * 3 );
    
    mMusicLoudnessSlider.setPosition( 0, lineSpacing / 2 );
    mSoundEffectsLoudnessSlider.setPosition( 0, - lineSpacing / 2 );
    
    mTargetFrameRateField.setPosition( 5, lineSpacing );
    mVsyncBox.setPosition( 0, 0 );
    mFullscreenBox.setPosition( 0, -lineSpacing );
    mBorderlessBox.setPosition( 0, -lineSpacing * 2 );
    mTrippingEffectDisabledBox.setPosition( 0, -lineSpacing * 3 );
    mRedetectButton.setPosition( 161, lineSpacing * 2 );
    mRedetectButton.setPadding( 8, 4 );

#ifdef USE_DISCORD
    mEnableDiscordRichPresence.setPosition(0, 3 * lineSpacing);
    mEnableDiscordRichPresenceStatus.setPosition(0, 2 * lineSpacing);
    mEnableDiscordShowAgeInStatus.setPosition(0, lineSpacing);
    mEnableDiscordRichPresenceDetails.setPosition(0, 0);
    mDiscordHideFirstNameInDetails.setPosition(0, -lineSpacing);
#endif // USE_DISCORD

    mCommandShortcuts.setPosition(180 - 16, lineSpacing * 4);
    mEnableAdvancedShowUseOnObjectHoverKeybind.setPosition(0, lineSpacing * 3);
    mEnableCoordinatesBox.setPosition(0, lineSpacing * 2);
    mEnablePersistentEmoteBox.setPosition(0, lineSpacing * 1);
    mEnableYumFinderBox.setPosition(0, lineSpacing * 0);
    mEnableObjectSearchBox.setPosition(0, lineSpacing * -1);
    mEnableFamilyDisplayBox.setPosition(0, lineSpacing * -2);
    mEnableDangerousTileBox.setPosition(0, lineSpacing * -3);
    mGenerateTownPlannerMapsBox.setPosition(0, lineSpacing * -4);
    mEnableShowingHeldFoodPips.setPosition(0, lineSpacing * -5);
    
    mEnableFOVBox.setVisible( mPage == 0 );
    mEnableCenterCameraBox.setVisible( mPage == 0 );
    mEnableNudeBox.setVisible( mPage == 0 );
    mUISizeSlider.setVisible( mPage == 0 );
    mUseCustomServerBox.setVisible( mPage == 0 );
    mCustomServerAddressField.setVisible( mPage == 0 && mUseCustomServerBox.getToggled() );
    mCustomServerPortField.setVisible( mPage == 0 && mUseCustomServerBox.getToggled() );
    mCopyButton.setVisible( isClipboardSupported() && mPage == 0 && mUseCustomServerBox.getToggled() );
    mPasteButton.setVisible( isClipboardSupported() && mPage == 0 && mUseCustomServerBox.getToggled() );

    mEnableKActionsBox.setVisible( mPage == 1 );
    mCursorModeSet->setVisible( mPage == 1 );
    int mode = getCursorMode();
    mCursorScaleSlider.setVisible( mode > 0 && mCursorModeSet->isVisible() );
    

    mTargetFrameRateField.setVisible( mPage == 2 && !mVsyncBox.getToggled() );
    mVsyncBox.setVisible( mPage == 2 );
    mFullscreenBox.setVisible( mPage == 2 );
    mBorderlessBox.setVisible( mPage == 2 && mFullscreenBox.getToggled() );
    mTrippingEffectDisabledBox.setVisible( mPage == 2 );
    mRedetectButton.setVisible( mPage == 2 );

    mMusicLoudnessSlider.setVisible( mPage == 3 );
    mSoundEffectsLoudnessSlider.setVisible( mPage == 3 );

#ifdef USE_DISCORD
    mEnableDiscordRichPresence.setVisible(mPage == 4);
    mEnableDiscordRichPresenceStatus.setVisible(mPage == 4 && mEnableDiscordRichPresence.getToggled());
    mEnableDiscordShowAgeInStatus.setVisible(mPage == 4 
                                        && mEnableDiscordRichPresence.getToggled()
                                        && mEnableDiscordRichPresenceStatus.getToggled());
    mEnableDiscordRichPresenceDetails.setVisible(mPage == 4 
                                        && mEnableDiscordRichPresence.getToggled() 
                                        && mEnableDiscordRichPresenceStatus.getToggled());
    mDiscordHideFirstNameInDetails.setVisible(mPage == 4 
                                        && mEnableDiscordRichPresenceDetails.getToggled() 
                                        && mEnableDiscordRichPresence.getToggled() 
                                        && mEnableDiscordRichPresenceStatus.getToggled());
#endif // USE_DISCORD

    mCommandShortcuts.setVisible(mPage == 5);
    mEnableAdvancedShowUseOnObjectHoverKeybind.setVisible(mPage == 5);
    mEnableCoordinatesBox.setVisible(mPage == 5);
    mEnablePersistentEmoteBox.setVisible(mPage == 5);
    mEnableYumFinderBox.setVisible(mPage == 5);
    mEnableObjectSearchBox.setVisible(mPage == 5);
    mEnableFamilyDisplayBox.setVisible(mPage == 5);
    mEnableDangerousTileBox.setVisible(mPage == 5);
    mGenerateTownPlannerMapsBox.setVisible(mPage == 5);
    mEnableShowingHeldFoodPips.setVisible(mPage == 5);
    
    mGameplayButton.setActive( mPage != 0 );
    mControlButton.setActive( mPage != 1 );
    mScreenButton.setActive( mPage != 2 );
    mSoundButton.setActive( mPage != 3 );

#ifdef USE_DISCORD
    mDiscordButton.setActive( mPage != 4 );
#endif // USE_DISCORD

    mAdvancedButton.setActive( mPage != 5 );

}

void SettingsPage::checkRestartRequired() {
    if( mOldFullscreenSetting != mFullscreenBox.getToggled() ||
        mOldBorderlessSetting != mBorderlessBox.getToggled() ||
        getCountingOnVsync() != mVsyncBox.getToggled() ||
        ( mTargetFrameRateField.isVisible() && mTargetFrameRateField.getInt() != targetFramesPerSecond ) ||
        mOldGenerateTownPlannerMapsSetting != mGenerateTownPlannerMapsBox.getToggled()
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

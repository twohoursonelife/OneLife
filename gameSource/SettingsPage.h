#include "GamePage.h"

#include "TextButton.h"
#include "CheckboxButton.h"
#include "RadioButtonSet.h"
#include "ValueSlider.h"
#include "SoundUsage.h"
#include "DropdownList.h"
#include "Background.h"


#include "minorGems/ui/event/ActionListener.h"




class SettingsPage : public GamePage, public ActionListener {
        
    public:
        
        SettingsPage();
        ~SettingsPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();
		virtual void updatePage();
        virtual void checkRestartRequired();

    protected:
        
        int mOldFullscreenSetting;
        int mOldBorderlessSetting;
        int mTrippingEffectDisabledSetting;
        int mEnableNudeSetting;
        int mEnableFOVSetting;
        int mEnableKActionsSetting;
        int mEnableCenterCameraSetting;
#ifdef USE_DISCORD
        int mDiscordRichPresenceSetting;
        int mDiscordRichPresenceStatusSetting;
        int mDiscordShowAgeInStatusSetting;
        int mDiscordRichPresenceDetailsSetting;
        int mDiscordHideFirstNameInDetailsSetting;
#endif // USE_DISCORD
        int mPage;
        int mAdvancedShowUseOnObjectHoverKeybindSetting;
        
        SoundUsage mTestSound;

        double mMusicStartTime;


        Background mBackground;

        // Left Pane
        TextButton mRestartButton;
        
        TextButton mGameplayButton;
        TextButton mControlButton;
        TextButton mScreenButton;
        TextButton mSoundButton;
#ifdef USE_DISCORD
        TextButton mDiscordButton;
#endif // USE_DISCORD
        TextButton mAdvancedButton;

        TextButton mBackButton;
        
        TextButton mEditAccountButton;

        // Gameplay
		CheckboxButton mEnableFOVBox;
		CheckboxButton mEnableCenterCameraBox;
		CheckboxButton mEnableNudeBox;
        
        CheckboxButton mUseCustomServerBox;
        TextField mCustomServerAddressField;
        TextField mCustomServerPortField;
        TextButton mCopyButton;
        TextButton mPasteButton;
        
        // Control
		CheckboxButton mEnableKActionsBox;
        RadioButtonSet *mCursorModeSet;
        ValueSlider mCursorScaleSlider;

        // Screen
        TextButton mRedetectButton;
        CheckboxButton mVsyncBox;
        CheckboxButton mFullscreenBox;
        CheckboxButton mBorderlessBox;
        TextField mTargetFrameRateField;
        CheckboxButton mTrippingEffectDisabledBox;
        
        // Sound
        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;

#ifdef USE_DISCORD
        // Discord
        CheckboxButton mEnableDiscordRichPresence;
        CheckboxButton mEnableDiscordRichPresenceStatus;
        CheckboxButton mEnableDiscordShowAgeInStatus;
        CheckboxButton mEnableDiscordRichPresenceDetails;
        CheckboxButton mDiscordHideFirstNameInDetails;
#endif // USE_DISCORD

        CheckboxButton mEnableAdvancedShowUseOnObjectHoverKeybind;
        
        
        void checkRestartButtonVisibility();
        
    };

#include "GamePage.h"

#include "TextField.h"
#include "TextButton.h"
#include "KeyEquivalentTextButton.h"
#include "DropdownList.h"
#include "RadioButtonSet.h"

#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "PageComponent.h"
#include "Background.h"



class ExistingAccountPage : public GamePage, public ActionListener {
        
    public:
        
        ExistingAccountPage();
        
        virtual ~ExistingAccountPage();
        
        void clearFields();


        // defaults to true
        void showReviewButton( char inShow );
        
        // defaults to false
        void showDisableCustomServerButton( char inShow );
        

        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

        virtual void step();
        

        // for TAB and ENTER (switch fields and start login)
        virtual void keyDown( unsigned char inASCII );
        
        virtual void pointerUp( float inX, float inY );
        
        // for arrow keys (switch fields)
        virtual void specialKeyDown( int inKeyCode );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );


    protected:
        
        Background mBackground;
        Background mGameLogo;
        
        // Left Pane Page 0
        TextField mEmailField;
        TextButton mEmailLockButton;
        KeyEquivalentTextButton mPasteEmailButton;
        
        TextField mKeyField;
        TextButton mKeyLockButton;
        KeyEquivalentTextButton mPasteButton;

        TextField *mFields[2];
        
        TextButton mNextToGameTabButton;
        
        // Left Pane Page 1
        TextButton mFriendsButton;
        TextButton mSoloButton;
        TextField mTwinCodeField;
        TextButton mGenerateButton;
        TextButton mTwinCodeCopyButton;
        TextButton mTwinCodePasteButton;
        RadioButtonSet *mPlayerCountRadioButtonSet;
        JenkinsRandomSource mRandSource;
        SimpleVector<char*> mWordList;
        
        TextButton mSpecificButton;
        TextButton mRandomButton;
        DropdownList mSpawnSeed;
        TextButton mSpawnSeedLockButton;
        TextField mTargetFamily;
        RadioButtonSet *mSeedOrFamilyButtonSet;
        
        TextButton mBackToAccountTabButton;
        TextButton mLoginButton;
        
        // Right Pane
        TextButton mSettingsButton;
        TextButton mTutorialButton;
        TextButton mFamilyTreesButton;
        TextButton mTechTreeButton;
        TextButton mCancelButton;
        
        // Status
        TextButton mRetryButton;
        TextButton mRedetectButton;
        TextButton mDisableCustomServerButton;
        
        // Not in use
        TextButton mGenesButton;
        TextButton mClearAccountButton;
        TextButton mReviewButton;
        TextButton mAtSignButton;
        TextButton mViewAccountButton;
        

        double mPageActiveStartTime;
        int mFramesCounted;
        char mFPSMeasureDone;

        char mHideAccount;

        void nextPage();
        
        void updatefieldsAndLockButtons();
        void updateLeftPane();
        
        void processLogin( char inStore, const char *inSignal );

    };


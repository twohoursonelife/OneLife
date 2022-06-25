#include "RebirthChoicePage.h"

#include "buttonStyle.h"
#include "message.h"

#include "lifeTokens.h"
#include "fitnessScore.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;


static doublePair tutorialButtonPos = { 522, 300 };



RebirthChoicePage::RebirthChoicePage()
        : mBackground( "background.tga", 0.75f ),
        
          mQuitButton( mainFont, 0, -224, 
                       translate( "quit" ) ),
          mReviewButton( mainFont, 150, 64, 
                       translate( "postReviewButton" ) ),
          mRebornButton( mainFont, 0, 64, 
                         translate( "reborn" ) ),
          mGenesButton( mainFont, 0, -32, 
                        translate( "geneticHistoryButton" ) ),
          mSettingsButton( mainFont, 0, -32,
                           translate( "settingsButton" ) ),
          mTutorialButton( mainFont, 0, 64, 
                           translate( "tutorial" ) ),
          mMenuButton( mainFont, 0, -128, 
                       "BACK" ){
    
    addComponent( &mBackground );
    
    if( !isHardToQuitMode() ) {
        // addComponent( &mQuitButton );
        // addComponent( &mReviewButton );
        addComponent( &mMenuButton );
        addComponent( &mTutorialButton );
        addComponent( &mGenesButton );
    
        }
    else {
        // mRebornButton.setPosition( 0, -200 );
        // mGenesButton.setPosition( 0, 0 );
        }
        
    // mQuitButton.setSize( 260, 60 );
    mRebornButton.setSize( 260, 60 );
    mSettingsButton.setSize( 260, 60 );
    mTutorialButton.setSize( 260, 60 );
    mMenuButton.setSize( 260, 60 );
    // mGenesButton.setSize( 260, 60 );
        
    mGenesButton.setVisible( false );
    
    addComponent( &mRebornButton );
    addComponent( &mTutorialButton );
    addComponent( &mGenesButton );
    addComponent( &mSettingsButton );

    setButtonStyle( &mQuitButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mRebornButton );
    setButtonStyle( &mGenesButton );
    
    setButtonStyle( &mTutorialButton );
    setButtonStyle( &mSettingsButton );
    setButtonStyle( &mMenuButton );
    
    mQuitButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    mRebornButton.addActionListener( this );
    mGenesButton.addActionListener( this );
    mTutorialButton.addActionListener( this );
    mSettingsButton.addActionListener( this );
    mMenuButton.addActionListener( this );


    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
        

    mQuitButton.setCursorTip( "CLOSE THE GAME" );
    mRebornButton.setCursorTip( "BE BORN AGAIN WITH THE SAME SPAWN SETTINGS" );
    mGenesButton.setCursorTip( "CHECK YOUR GENETIC HISTORY" );
    
    mTutorialButton.setCursorTip( "START THE TUTORIAL AGAIN" );
    mSettingsButton.setCursorTip( "CHANGE GAME SETTINGS" );
    mMenuButton.setCursorTip( "RETURN TO MAIN MENU" );
    
    }



void RebirthChoicePage::showReviewButton( char inShow ) {
    mReviewButton.setVisible( inShow );
    }



void RebirthChoicePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mQuitButton ) {
        setSignal( "quit" );
        }
    else if( inTarget == &mReviewButton ) {
        setSignal( "review" );
        }
    else if( inTarget == &mRebornButton ) {
        setSignal( "reborn" );
        }
    else if( inTarget == &mGenesButton ) {
        setSignal( "genes" );
        }
    else if( inTarget == &mTutorialButton ) {
        setSignal( "tutorial" );
        }
    else if( inTarget == &mSettingsButton ) {
        setSignal( "settings" );
        }
    else if( inTarget == &mMenuButton ) {
        setSignal( "menu" );
        }
    }



void RebirthChoicePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );

    drawTokenMessage( pos );

    pos.y += 104;
    drawFitnessScore( pos );
    }



void RebirthChoicePage::makeActive( char inFresh ) {
    triggerLifeTokenUpdate();
    triggerFitnessScoreUpdate();
    
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
        }
    else {
        // mReviewButton.setVisible( true );
        }


    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    

    if( !tutorialDone ) {
        mRebornButton.setVisible( false );
        mTutorialButton.setVisible( true );
        
        doublePair rebornPos = mRebornButton.getPosition();
        mTutorialButton.setPosition( rebornPos.x, rebornPos.y );
        mTutorialButton.setLabelText( translate( "restartTutorial" ) );
        }
    else {
        mRebornButton.setVisible( true );
        mTutorialButton.setVisible( false );
        
        mTutorialButton.setLabelText( translate( "tutorial" ) );
        }
    }

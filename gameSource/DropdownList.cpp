#include "DropdownList.h"
#include "TextField.h"

#include <string.h>

#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"



// start:  none focused
DropdownList *DropdownList::sFocusedDropdownList = NULL;

extern double frameRateFactor;

int DropdownList::sDeleteFirstDelaySteps = 30 / frameRateFactor;
int DropdownList::sDeleteNextDelaySteps = 2 / frameRateFactor;



DropdownList::DropdownList( Font *inDisplayFont, 
                      double inX, double inY, int inCharsWide,
                      char inForceCaps,
                      const char *inLabelText,
                      const char *inAllowedChars,
                      const char *inForbiddenChars,
                      int inListLenDisplayed )
        : PageComponent( inX, inY ),
          mActive( true ),
          mContentsHidden( false ),
           
          mHiddenSprite( loadSprite( "hiddenFieldTexture.tga", false ) ),
          mFont( inDisplayFont ), 
          mCharsWide( inCharsWide ),
          mMaxLength( -1 ),
          mFireOnAnyChange( false ),
          mFireOnLeave( false ),
          mForceCaps( inForceCaps ),
          mLabelText( NULL ),
          mAllowedChars( NULL ), mForbiddenChars( NULL ),
          
          mHover( false ),
          
          mRawText( new char[1] ),
          hoverIndex( -1 ),
          startIndex( 0 ),
          listLenDisplayed( inListLenDisplayed ), 
          nearRightEdge( 0 ),
          mUseClearButton( false ),
          onClearButton( false ),
          
          mFocused( false ), mText( new char[1] ),
          mTextLen( 0 ),
          mCursorPosition( 0 ),
          mIgnoreArrowKeys( false ),
          mIgnoreMouse( false ),
          mDrawnText( NULL ),
          mCursorDrawPosition( 0 ),
          mHoldDeleteSteps( -1 ), mFirstDeleteRepeatDone( false ),
          mLabelOnRight( false ),
          mLabelOnTop( false ),
          mSelectionStart( -1 ),
          mSelectionEnd( -1 ),
          mShiftPlusArrowsCanSelect( false ),
          mCursorFlashSteps( 0 ),
          mUsePasteShortcut( false ) {
    
    if( inLabelText != NULL ) {
        mLabelText = stringDuplicate( inLabelText );
        }
    
    if( inAllowedChars != NULL ) {
        mAllowedChars = stringDuplicate( inAllowedChars );
        }
    if( inForbiddenChars != NULL ) {
        mForbiddenChars = stringDuplicate( inForbiddenChars );
        }

    clearArrowRepeat();
        

    mCharWidth = mFont->getFontHeight();

    mBorderWide = mCharWidth * 0.25;

    mHigh = mFont->getFontHeight() + 2 * mBorderWide;

    char *fullString = new char[ mCharsWide + 1 ];

    unsigned char widestChar = 0;
    double width = 0;

    for( int c=32; c<256; c++ ) {
        unsigned char pc = processCharacter( c );

        if( pc != 0 ) {
            char s[2];
            s[0] = pc;
            s[1] = '\0';

            double thisWidth = mFont->measureString( s );
            
            if( thisWidth > width ) {
                width = thisWidth;
                widestChar = pc;    
                }
            }
        }
    
    


    for( int i=0; i<mCharsWide; i++ ) {
        fullString[i] = widestChar;
        }
    fullString[ mCharsWide ] = '\0';
    
    double fullStringWidth = mFont->measureString( fullString );

    delete [] fullString;

    mWide = fullStringWidth + 2 * mBorderWide;
    
    mDrawnTextX = - ( mWide / 2 - mBorderWide );

    mText[0] = '\0';
    }



DropdownList::~DropdownList() {
    if( this == sFocusedDropdownList ) {
        // we're focused, now nothing is focused
        sFocusedDropdownList = NULL;
        }

    delete [] mText;

    if( mLabelText != NULL ) {
        delete [] mLabelText;
        }

    if( mAllowedChars != NULL ) {
        delete [] mAllowedChars;
        }
    if( mForbiddenChars != NULL ) {
        delete [] mForbiddenChars;
        }

    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }

    if( mHiddenSprite != NULL ) {
        freeSprite( mHiddenSprite );
        }
    }



void DropdownList::setContentsHidden( char inHidden ) {
    mContentsHidden = inHidden;
    }




char *DropdownList::processRawText( const char *inRawText ) {

    // obeys same rules as typing (skip blocked characters)
    SimpleVector<char> filteredText;
    
    int length = strlen( inRawText );
    for( int i=0; i<length; i++ ) {
        unsigned char processedChar = processCharacter( inRawText[i] );

        // newline is allowed, that's our delimiter of items in the list
        if( inRawText[i] == '\n' ) processedChar = inRawText[i];
        
        if( processedChar != 0 ) {
            filteredText.push_back( processedChar );
            }
        }
    
    char *rawStringWithEmptyLines = filteredText.getElementString();
    int numLines;
    char **lines = split( rawStringWithEmptyLines, "\n", &numLines );
    delete [] rawStringWithEmptyLines;
    
    char *processedRawText = stringDuplicate("");
    
    for( int i=0; i<numLines; i++ ) {
        
        if( i == 0 ) setText( lines[i] );
        
        if( strcmp( lines[i], "" ) != 0 ) {
            if( strcmp( processedRawText, "" ) != 0 )
                processedRawText = concatonate( processedRawText, "\n" );
            processedRawText = concatonate( processedRawText, lines[i] );
            }
        
        delete [] lines[i];
        }
    delete [] lines;
    
    for( int i=0; i<numLines; i++ ) {
        delete [] lines[i];
        }
    delete [] lines;

    return processedRawText;
    
    }




void DropdownList::setListByRawText( const char *inText ) {
    
    if( mRawText != NULL ) delete [] mRawText;
    mRawText = processRawText( inText );

    if( strcmp( mRawText, "" ) != 0 ) {
        int numLines;
        char **lines = split( mRawText, "\n", &numLines );

        listLen = numLines;
        setText( lines[0] );

        for( int i=0; i<numLines; i++ ) {
            delete [] lines[i];
            }
        delete [] lines;
        }
    else {
        listLen = 0;
        setText( "" );
        }

    }




char *DropdownList::updateRawText( char *inRawText, char *inText ) {

    char *newRawText = stringDuplicate( "" );

    if( strcmp( inRawText, "" ) != 0 ) {
        // see whether text is already in rawText, and remove if so
        int numLines;
        char **lines = split( inRawText, "\n", &numLines );
        
        for( int i=0; i<numLines; i++ ) {
            if( strcmp( inText, lines[i] ) != 0 ) {
                if( strcmp( newRawText, "" ) != 0 )
                    newRawText = concatonate( newRawText, "\n" );
                newRawText = concatonate( newRawText, lines[i] );
                }
            delete [] lines[i];
            }
        delete [] lines;
        }

    if( strcmp( newRawText, "" ) != 0 && strcmp( inText, "" ) != 0 ) {
        newRawText = concatonate( "\n", newRawText );
        newRawText = concatonate( inText, newRawText );
        }
    else if( strcmp( inText, "" ) != 0 ) {
        if( newRawText != NULL ) delete [] newRawText;
        newRawText = stringDuplicate( inText );
        }

    return newRawText;
    }



char *DropdownList::getAndUpdateRawText() {

    char *newRawText = updateRawText( mRawText, mText );
    if( mRawText != NULL ) delete [] mRawText;
    mRawText = newRawText;

    if( strcmp( mRawText, "" ) != 0 ) {
        int numLines;
        char **lines = split( mRawText, "\n", &numLines );
        
        listLen = numLines;

        for( int i=0; i<numLines; i++ ) {
            delete [] lines[i];
            }
        delete [] lines;
        }
    else {
        listLen = 0;
        }
    
    return mRawText;
    }
    
    
void DropdownList::setText( const char *inText ) {
    delete [] mText;
    
    mSelectionStart = -1;
    mSelectionEnd = -1;
    
    mText = stringDuplicate( inText );
    
    mTextLen = strlen( mText );
    
    mCursorPosition = strlen( mText );

    // hold-downs broken
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();
    }
    
char *DropdownList::getText() {
    return stringDuplicate( mText );
    }
    
void DropdownList::selectOption( int index ) {    
    if( index < 0 || index >= listLen ) return;
    
    int numLines;
    char **lines = split( mRawText, "\n", &numLines );
    setText( lines[index] );

    char *newRawText = updateRawText( mRawText, mText );
    if( mRawText != NULL ) delete [] mRawText;
    mRawText = newRawText;
    
    for( int i=0; i<numLines; i++ ) {
        delete [] lines[i];
        }
    delete [] lines;
    }
    
void DropdownList::deleteOption( int index ) {
    if( index < 0 || index >= listLen ) return;
    
    int numLines;
    char **lines = split( mRawText, "\n", &numLines );
    
    char *newRawText = stringDuplicate("");
    
    for( int i=0; i<numLines; i++ ) {
        
        if( i != index ) {
            if( strcmp( newRawText, "" ) != 0 )
                    newRawText = concatonate( newRawText, "\n" );
            newRawText = concatonate( newRawText, lines[i] );
            }
        
        delete [] lines[i];
        }
    delete [] lines;

    if( mRawText != NULL ) delete [] mRawText;
    mRawText = newRawText;
    
    listLen = listLen - 1;
    
    }



void DropdownList::setMaxLength( int inLimit ) {
    mMaxLength = inLimit;
    }



int DropdownList::getMaxLength() {
    return mMaxLength;
    }



char DropdownList::isAtLimit() {
    if( mMaxLength == -1 ) {
        return false;
        }
    else {
        return ( mTextLen == mMaxLength );
        }
    }
    



void DropdownList::setActive( char inActive ) {
    mActive = inActive;
    }



char DropdownList::isActive() {
    return mActive;
    }
        


void DropdownList::step() {

    mCursorFlashSteps ++;

    if( mHoldDeleteSteps > -1 ) {
        mHoldDeleteSteps ++;

        int stepsBetween = sDeleteFirstDelaySteps;
        
        if( mFirstDeleteRepeatDone ) {
            stepsBetween = sDeleteNextDelaySteps;
            }
        
        if( mHoldDeleteSteps > stepsBetween ) {
            // delete repeat
            mHoldDeleteSteps = 0;
            mFirstDeleteRepeatDone = true;
            
            deleteHit();
            }
        }


    for( int i=0; i<2; i++ ) {
        
        if( mHoldArrowSteps[i] > -1 ) {
            mHoldArrowSteps[i] ++;

            int stepsBetween = sDeleteFirstDelaySteps;
        
            if( mFirstArrowRepeatDone[i] ) {
                stepsBetween = sDeleteNextDelaySteps;
                }
        
            if( mHoldArrowSteps[i] > stepsBetween ) {
                // arrow repeat
                mHoldArrowSteps[i] = 0;
                mFirstArrowRepeatDone[i] = true;
            
                switch( i ) {
                    case 0:
                        leftHit();
                        break;
                    case 1:
                        rightHit();
                        break;
                    }
                }
            }
        }


    }

        
        
void DropdownList::draw() {
    
    if( mFocused ) {    
        setDrawColor( 1, 1, 1, 1 );
        }
    else {
        setDrawColor( 0.5, 0.5, 0.5, 1 );
        }
    

    drawRect( - mWide / 2, - mHigh / 2, 
              mWide / 2, mHigh / 2 );
    
    setDrawColor( 0.25, 0.25, 0.25, 1 );
    double pixWidth = mCharWidth / 8;


    double rectStartX = - mWide / 2 + pixWidth;
    double rectStartY = - mHigh / 2 + pixWidth;

    double rectEndX = mWide / 2 - pixWidth;
    double rectEndY = mHigh / 2 - pixWidth;

    double middleWidth = mWide - 2 * pixWidth;
    
    drawRect( rectStartX, rectStartY,
              rectEndX, rectEndY );
    
    setDrawColor( 1, 1, 1, 1 );

    if( mContentsHidden && mHiddenSprite != NULL ) {
        startAddingToStencil( false, true );

        drawRect( rectStartX, rectStartY,
                  rectEndX, rectEndY );
        startDrawingThroughStencil();
        
        doublePair pos = { 0, 0 };
        
        drawSprite( mHiddenSprite, pos );
        
        stopStencil();
        }
    


    
    if( mLabelText != NULL ) {
        TextAlignment a = alignRight;
        double xPos = -mWide/2 - mBorderWide;
        
        double yPos = 0;
        
        if( mLabelOnTop ) {
            xPos += mBorderWide + pixWidth;
            yPos = mHigh / 2 + 2 * mBorderWide;
            }

        if( mLabelOnRight ) {
            a = alignLeft;
            xPos = -xPos;
            }
        
        if( mLabelOnTop ) {
            // reverse align if on top
            if( a == alignLeft ) {
                a = alignRight;
                }
            else {
                a = alignLeft;
                }
            }
        
        doublePair labelPos = { xPos, yPos };
        
        mFont->drawString( mLabelText, labelPos, a );
        }
    
    
    if( mContentsHidden ) {
        return;
        }




    doublePair textPos = { - mWide/2 + mBorderWide, 0 };


    char tooLongFront = false;
    char tooLongBack = false;
    
    mCursorDrawPosition = mCursorPosition;


    char *textBeforeCursorBase = stringDuplicate( mText );
    char *textAfterCursorBase = stringDuplicate( mText );
    
    char *textBeforeCursor = textBeforeCursorBase;
    char *textAfterCursor = textAfterCursorBase;

    textBeforeCursor[ mCursorPosition ] = '\0';
    
    textAfterCursor = &( textAfterCursor[ mCursorPosition ] );

    if( mFont->measureString( mText ) > mWide - 2 * mBorderWide ) {
        
        if( mFont->measureString( textBeforeCursor ) > 
            mWide / 2 - mBorderWide
            &&
            mFont->measureString( textAfterCursor ) > 
            mWide / 2 - mBorderWide ) {

            // trim both ends

            while( mFont->measureString( textBeforeCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                }
        
            while( mFont->measureString( textAfterCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ strlen( textAfterCursor ) - 1 ] = '\0';
                }
            }
        else if( mFont->measureString( textBeforeCursor ) > 
                 mWide / 2 - mBorderWide ) {

            // just trim front
            char *sumText = concatonate( textBeforeCursor, textAfterCursor );
            
            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }    
        else if( mFont->measureString( textAfterCursor ) > 
                 mWide / 2 - mBorderWide ) {
            
            // just trim back
            char *sumText = concatonate( textBeforeCursor, textAfterCursor );

            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ strlen( textAfterCursor ) - 1 ] = '\0';
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }
        }

    
    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }
    
    mDrawnText = concatonate( textBeforeCursor, textAfterCursor );

    char leftAlign = true;
    char cursorCentered = false;
    doublePair centerPos = { 0, 0 };
    
    if( ! tooLongFront ) {
        mFont->drawString( mDrawnText, textPos, alignLeft );
        mDrawnTextX = textPos.x;
        }
    else if( tooLongFront && ! tooLongBack ) {
        
        leftAlign = false;

        doublePair textPos2 = { mWide/2 - mBorderWide, 0 };

        mFont->drawString( mDrawnText, textPos2, alignRight );
        mDrawnTextX = textPos2.x - mFont->measureString( mDrawnText );
        }
    else {
        // text around perfectly centered cursor
        cursorCentered = true;
        
        double beforeLength = mFont->measureString( textBeforeCursor );
        
        double xDiff = centerPos.x - ( textPos.x + beforeLength );
        
        doublePair textPos2 = textPos;
        textPos2.x += xDiff;

        mFont->drawString( mDrawnText, textPos2, alignLeft );
        mDrawnTextX = textPos2.x;
        }
        
        
    if ( mFocused ) {

        float pixWidth = mCharWidth / 8;
        float buttonWidth = mFont->measureString( "x" ) + pixWidth * 2;
        float buttonRightOffset = buttonWidth / 2 + mBorderWide;
        
        if( mUseClearButton && strcmp( mText, "" ) != 0 ) {
            doublePair lineDeleteButtonPos = { mWide / 2 - buttonRightOffset, 0 };
            if ( onClearButton ) {
                setDrawColor( 0, 0, 0, 0.5 );
                drawRect( 
                    lineDeleteButtonPos.x - buttonWidth / 2, 
                    lineDeleteButtonPos.y - buttonWidth / 2, 
                    lineDeleteButtonPos.x + buttonWidth / 2, 
                    lineDeleteButtonPos.y + buttonWidth / 2 );
            }
            setDrawColor( 1, 1, 1, 1 );
            mFont->drawString( "x", lineDeleteButtonPos, alignCenter );
        }
        
        if( strcmp( mRawText, "" ) != 0 ) {

            int numLines = 0;
            char **lines = split( mRawText, "\n", &numLines );
            
            for( int i=0; i<numLines; i++ ) {

                if( i < startIndex || i >= startIndex + listLenDisplayed ) {
                    delete [] lines[i];
                    continue;
                    }

                int relativeIndex = i - startIndex;
                
                doublePair linePos = { centerPos.x, centerPos.y - (relativeIndex + 1) * mHigh };
                float backgroundAlpha = 0.3;
                if( hoverIndex == i ) backgroundAlpha = 0.7;
                setDrawColor( 0, 0, 0, 1 );
                drawRect( - mWide / 2, linePos.y - mHigh / 2, 
                    mWide / 2, linePos.y + mHigh / 2 );
                setDrawColor( 1, 1, 1, backgroundAlpha );
                drawRect( - mWide / 2, linePos.y - mHigh / 2, 
                    mWide / 2, linePos.y + mHigh / 2 );
                doublePair lineTextPos = { textPos.x, textPos.y - (relativeIndex + 1) * mHigh };
                    
                setDrawColor( 0, 0, 0, 0.5 );
                doublePair lineDeleteButtonPos = { mWide / 2 - buttonRightOffset, lineTextPos.y };
                if( hoverIndex == i && nearRightEdge ) {
                    drawRect( 
                        lineDeleteButtonPos.x - buttonWidth / 2, 
                        lineDeleteButtonPos.y - buttonWidth / 2, 
                        lineDeleteButtonPos.x + buttonWidth / 2, 
                        lineDeleteButtonPos.y + buttonWidth / 2 );
                    }
                
                setDrawColor( 1, 1, 1, 1 );
                char *lineText = stringDuplicate( lines[i] );
                
                if( mFont->measureString( lineText ) 
                    + mFont->measureString( "...   " ) 
                    > mWide - 2 * mBorderWide ) {

                    while( mFont->measureString( lineText ) + mFont->measureString( "...   " ) > 
                           mWide - 2 * mBorderWide ) {
                        
                        lineText[ strlen( lineText ) - 1 ] = '\0';
                        
                        }
                        
                    lineText = concatonate( lineText, "...   " );
                    
                    }
                
                mFont->drawString( lineText, lineTextPos, alignLeft );
                
                setDrawColor( 1, 1, 1, 1 );
                mFont->drawString( "x", lineDeleteButtonPos, alignCenter );
                
                
                
                delete [] lines[i];
                }
            delete [] lines;
            
            if( startIndex > 0 ) {
                doublePair morePrevHintPos = { centerPos.x, centerPos.y - 0.5 * mHigh };
                setDrawColor( 1, 1, 1, 1 );
                mFont->drawString( "...", morePrevHintPos, alignCenter );
                }

            if( startIndex + listLenDisplayed < numLines ) {
                doublePair moreAfterHintPos = { centerPos.x, centerPos.y - (listLenDisplayed + 0.25) * mHigh };
                setDrawColor( 1, 1, 1, 1 );
                mFont->drawString( "...", moreAfterHintPos, alignCenter );
                }

            }
        }
    

    double shadeWidth = 4 * mCharWidth;
    
    if( shadeWidth > middleWidth / 2 ) {
        shadeWidth = middleWidth / 2;
        }

    if( tooLongFront ) {
        // draw shaded overlay over left of string
        
        double verts[] = { rectStartX, rectStartY,
                           rectStartX, rectEndY,
                           rectStartX + shadeWidth, rectEndY,
                           rectStartX + shadeWidth, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0 };

        drawQuads( 1, verts , vertColors );
        }
    if( tooLongBack ) {
        // draw shaded overlay over right of string
        
        double verts[] = { rectEndX - shadeWidth, rectStartY,
                           rectEndX - shadeWidth, rectEndY,
                           rectEndX, rectEndY,
                           rectEndX, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1 };

        drawQuads( 1, verts , vertColors );
        }
    
    if( mFocused && mCursorDrawPosition > -1 ) {            
        // make measurement to draw cursor

        char *beforeCursorText = stringDuplicate( mDrawnText );
        
        beforeCursorText[ mCursorDrawPosition ] = '\0';
        
        
        double cursorXOffset;

        if( cursorCentered ) {
            cursorXOffset = mWide / 2 - mBorderWide;
            }
        else if( leftAlign ) {
            cursorXOffset = mFont->measureString( textBeforeCursor );
            if( cursorXOffset == 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        else {
            double afterLength = mFont->measureString( textAfterCursor );
            cursorXOffset = ( mWide - 2 * mBorderWide ) - afterLength;

            if( afterLength > 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        

        
        delete [] beforeCursorText;
        
        setDrawColor( 0, 0, 0, 0.5 );
        
        drawRect( textPos.x + cursorXOffset, 
                  rectStartY - pixWidth,
                  textPos.x + cursorXOffset + pixWidth, 
                  rectEndY + pixWidth );
        }
    
    
    if( ! mActive ) {
        setDrawColor( 0, 0, 0, 0.5 );
        // dark overlay
        drawRect( - mWide / 2, - mHigh / 2, 
                  mWide / 2, mHigh / 2 );
        }
        

    delete [] textBeforeCursorBase;
    delete [] textAfterCursorBase;
    }
    
    
int DropdownList::insideIndex( float inX, float inY ) {
    if( !mFocused ) return -1;
    if( fabs( inX ) >= mWide / 2 ) return -1;
    int index = - ( inY - mHigh / 2 ) / mHigh;
    index = index - 1;
    if( index >= listLenDisplayed ) return -1;
    index = index + startIndex;
    if( index < 0 ) return -1;
    if( index >= listLen ) return -1;
    return index;
    }
    
char DropdownList::isInsideTextBox( float inX, float inY ) {
    return fabs( inX ) < mWide / 2 &&
        fabs( inY ) < mHigh / 2;
    }
    
char DropdownList::isNearRightEdge( float inX, float inY ) {
    float pixWidth = mCharWidth / 8;
    float buttonWidth = mFont->measureString( "x" ) + pixWidth * 2;
    float buttonRightOffset = buttonWidth / 2 + mBorderWide;
    return inX > 0 && 
        fabs( inX - ( mWide / 2 - buttonRightOffset ) ) < buttonWidth / 2;
    }



void DropdownList::pointerMove( float inX, float inY ) {
    hoverIndex = insideIndex( inX, inY );
    nearRightEdge = isNearRightEdge( inX, inY );
    onClearButton = isInsideTextBox( inX, inY ) && nearRightEdge;
    mHover = hoverIndex != -1 || isInsideTextBox( inX, inY );
    }


void DropdownList::pointerDown( float inX, float inY ) {
    
    int mouseButton = getLastMouseButton();

    if ( mouseButton == MouseButton::WHEELDOWN ) {
        startIndex++;
        if( startIndex > listLen - listLenDisplayed ) {
            startIndex = listLen - listLenDisplayed;
            }
        if( startIndex < 0 ) {
            startIndex = 0;
            }
        return;
        }
    else if ( mouseButton == MouseButton::WHEELUP ) { 
        startIndex--;
        if( startIndex < 0 ) {
            startIndex = 0;
            }
        return;
        }

    
    
    hoverIndex = insideIndex( inX, inY );
    if( !mHover ) {
        unfocus();
        }
    if( onClearButton && mFocused ) setText( "" );
    if( hoverIndex == -1 ) return;
    if( isInsideTextBox( inX, inY ) ) return;
    if( !nearRightEdge ) {
        selectOption( hoverIndex );
        fireActionPerformed( this );
    } else {
        deleteOption( hoverIndex );
        if( startIndex > listLen - listLenDisplayed ) {
            startIndex = listLen - listLenDisplayed;
            }
        if( startIndex < 0 ) {
            startIndex = 0;
            }
        }
    }


void DropdownList::pointerUp( float inX, float inY ) {
        
    int mouseButton = getLastMouseButton();
    if ( mouseButton == MouseButton::WHEELUP || mouseButton == MouseButton::WHEELDOWN ) { return; }
    
    if( mIgnoreMouse || mIgnoreEvents ) {
        return;
        }
    
    if( inX > - mWide / 2 &&
        inX < + mWide / 2 &&
        inY > - mHigh / 2 &&
        inY < + mHigh / 2 ) {

        char wasHidden = mContentsHidden;

        focus();

        if( wasHidden ) {
            // don't adjust cursor from where it was
            }
        else {
            
            int bestCursorDrawPosition = mCursorDrawPosition;
            double bestDistance = mWide * 2;
            
            int drawnTextLength = strlen( mDrawnText );
            
            // find gap between drawn letters that is closest to clicked x
            
            for( int i=0; i<=drawnTextLength; i++ ) {
                
                char *textCopy = stringDuplicate( mDrawnText );
                
                textCopy[i] = '\0';
                
                double thisGapX = 
                    mDrawnTextX + 
                    mFont->measureString( textCopy ) +
                    mFont->getCharSpacing() / 2;
                
                delete [] textCopy;
                
                double thisDistance = fabs( thisGapX - inX );
                
                if( thisDistance < bestDistance ) {
                    bestCursorDrawPosition = i;
                    bestDistance = thisDistance;
                    }
                }
            
            int cursorDelta = bestCursorDrawPosition - mCursorDrawPosition;
            
            mCursorPosition += cursorDelta;
            }
        }
    }




unsigned char DropdownList::processCharacter( unsigned char inASCII ) {

    unsigned char processedChar = inASCII;
        
    if( mForceCaps ) {
        processedChar = toupper( inASCII );
        }

    if( mForbiddenChars != NULL ) {
        int num = strlen( mForbiddenChars );
            
        for( int i=0; i<num; i++ ) {
            if( mForbiddenChars[i] == processedChar ) {
                return 0;
                }
            }
        }
        

    if( mAllowedChars != NULL ) {
        int num = strlen( mAllowedChars );
            
        char allowed = false;
            
        for( int i=0; i<num; i++ ) {
            if( mAllowedChars[i] == processedChar ) {
                allowed = true;
                break;
                }
            }

        if( !allowed ) {
            return 0;
            }
        }
    else {
        // no allowed list specified 
        
        if( processedChar == '\r' ) {
            // \r only permitted if it is listed explicitly
            return 0;
            }
        }
        

    return processedChar;
    }



void DropdownList::insertCharacter( unsigned char inASCII ) {
    
    if( isAnythingSelected() ) {
        // delete selected first
        deleteHit();
        }

    // add to it
    char *oldText = mText;
    
    if( mMaxLength != -1 &&
        strlen( oldText ) >= (unsigned int) mMaxLength ) {
        // max length hit, don't add it
        return;
        }
    

    char *preCursor = stringDuplicate( mText );
    preCursor[ mCursorPosition ] = '\0';
    char *postCursor = &( mText[ mCursorPosition ] );
    
    mText = autoSprintf( "%s%c%s", 
                         preCursor, inASCII, postCursor );
    mTextLen = strlen( mText );

    delete [] preCursor;
    
    delete [] oldText;
    
    mCursorPosition++;
    }



void DropdownList::insertString( char *inString ) {
    if( isAnythingSelected() ) {
        // delete selected first
        deleteHit();
        }
    
    // add to it
    char *oldText = mText;
    

    char *preCursor = stringDuplicate( mText );
    preCursor[ mCursorPosition ] = '\0';
    char *postCursor = &( mText[ mCursorPosition ] );
    
    mText = autoSprintf( "%s%s%s", 
                         preCursor, inString, postCursor );
    
    mTextLen = strlen( mText );

    if( mMaxLength != -1 &&
        mTextLen > mMaxLength ) {
        // truncate
        mText[ mMaxLength ] = '\0';
        
        char *longString = mText;
        mText = stringDuplicate( mText );
        delete [] longString;
        
        mTextLen = strlen( mText );
        }
    

    delete [] preCursor;
    
    delete [] oldText;
    
    mCursorPosition += strlen( inString );

    if( mCursorPosition > mTextLen ) {
        mCursorPosition = mTextLen;
        }
    }



int DropdownList::getCursorPosition() {
    return mCursorPosition;
    }


void DropdownList::cursorReset() {
    mCursorPosition = 0;
    }



void DropdownList::setIgnoreArrowKeys( char inIgnore ) {
    mIgnoreArrowKeys = inIgnore;
    }



void DropdownList::setIgnoreMouse( char inIgnore ) {
    mIgnoreMouse = inIgnore;
    }



double DropdownList::getRightEdgeX() {
    
    return mX + mWide / 2;
    }



double DropdownList::getLeftEdgeX() {
    
    return mX - mWide / 2;
    }



double DropdownList::getWidth() {
    
    return mWide;
    }



void DropdownList::setWidth( double inWide ) {
    
    mWide = inWide;
    }



void DropdownList::setFireOnAnyTextChange( char inFireOnAny ) {
    mFireOnAnyChange = inFireOnAny;
    }


void DropdownList::setFireOnLoseFocus( char inFireOnLeave ) {
    mFireOnLeave = inFireOnLeave;
    }




void DropdownList::keyDown( unsigned char inASCII ) {
    if( !mFocused ) {
        return;
        }
    mCursorFlashSteps = 0;
    
    if( isCommandKeyDown() ) {
        // not a normal key stroke (command key)
        // ignore it as input

        if( mUsePasteShortcut && ( inASCII == 'v' || inASCII == 22 ) ) {
            // ctrl-v is SYN on some platforms
            
            // paste!
            if( isClipboardSupported() ) {
                char *clipboardText = getClipboardText();
        
                int len = strlen( clipboardText );
                
                for( int i=0; i<len; i++ ) {
                    
                    unsigned char processedChar = 
                        processCharacter( clipboardText[i] );    

                    if( processedChar != 0 ) {
                        
                        insertCharacter( processedChar );
                        }
                    }
                delete [] clipboardText;
                
                mHoldDeleteSteps = -1;
                mFirstDeleteRepeatDone = false;
                
                clearArrowRepeat();
                
                if( mFireOnAnyChange ) {
                    fireActionPerformed( this );
                    }
                }
            }
            
        if( mUsePasteShortcut && inASCII + 64 == toupper('c') )  {
            char *text = getText();
            setClipboardText( text );
            delete [] text;
            }

        // but ONLY if it's an alphabetical key (A-Z,a-z)
        // Some international keyboards use ALT to type certain symbols

        if( ( inASCII >= 'A' && inASCII <= 'Z' )
            ||
            ( inASCII >= 'a' && inASCII <= 'z' ) ) {
            
            return;
            }
        
        }
    

    if( inASCII == 127 || inASCII == 8 ) {
        // delete
        deleteHit();
        
        mHoldDeleteSteps = 0;

        clearArrowRepeat();
        }
    else if( inASCII == 13 ) {
        // enter hit in field
        unsigned char processedChar = processCharacter( inASCII );    

        if( processedChar != 0 ) {
            // newline is allowed
            insertCharacter( processedChar );
            
            mHoldDeleteSteps = -1;
            mFirstDeleteRepeatDone = false;
            
            clearArrowRepeat();
            
            if( mFireOnAnyChange ) {
                fireActionPerformed( this );
                }
            }
        else {
            // newline not allowed in this field

            if( hoverIndex >= 0 ) {
                selectOption( hoverIndex );
                
                // usually we want the text field to be unfocused here
                // but it may conflict with how the page handles Enter key
                // unfocus the text field in the page instead
                
                // unfocus();
                
                }

            fireActionPerformed( this );
            }
        }
    else if( inASCII >= 32 ) {

        unsigned char processedChar = processCharacter( inASCII );    

        if( processedChar != 0 ) {
            
            insertCharacter( processedChar );
            }
        
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;

        clearArrowRepeat();

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
            }
        }    
    }



void DropdownList::keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // end delete hold down
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;
        }
    }



void DropdownList::deleteHit() {
    if( mCursorPosition > 0 || isAnythingSelected() ) {
        mCursorFlashSteps = 0;
    
        int newCursorPos = mCursorPosition - 1;


        if( isAnythingSelected() ) {
            // selection delete
            
            mCursorPosition = mSelectionEnd;
            
            newCursorPos = mSelectionStart;

            mSelectionStart = -1;
            mSelectionEnd = -1;
            }
        else if( isCommandKeyDown() ) {
            // word delete 

            newCursorPos = mCursorPosition;

            // skip non-space, non-newline characters
            while( newCursorPos > 0 &&
                   mText[ newCursorPos - 1 ] != ' ' &&
                   mText[ newCursorPos - 1 ] != '\r' ) {
                newCursorPos --;
                }
        
            // skip space and newline characters
            while( newCursorPos > 0 &&
                   ( mText[ newCursorPos - 1 ] == ' ' ||
                     mText[ newCursorPos - 1 ] == '\r' ) ) {
                newCursorPos --;
                }
            }
        
        // section cleared no matter what when delete is hit
        mSelectionStart = -1;
        mSelectionEnd = -1;


        char *oldText = mText;
        
        char *preCursor = stringDuplicate( mText );
        preCursor[ newCursorPos ] = '\0';
        char *postCursor = &( mText[ mCursorPosition ] );

        mText = autoSprintf( "%s%s", preCursor, postCursor );
        mTextLen = strlen( mText );
        
        delete [] preCursor;

        delete [] oldText;

        mCursorPosition = newCursorPos;

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
            }
        }
    }



void DropdownList::clearArrowRepeat() {
    for( int i=0; i<2; i++ ) {
        mHoldArrowSteps[i] = -1;
        mFirstArrowRepeatDone[i] = false;
        }
    }



void DropdownList::leftHit() {
    mCursorFlashSteps = 0;
    
    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionStart;
            }
        else {
            mCursorPosition = *mSelectionAdjusting;
            }
        }

    if( ! isShiftKeyDown() ) {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionStart + 1;
            }

        mSelectionStart = -1;
        mSelectionEnd = -1;
        }

    if( isCommandKeyDown() ) {
        // word jump 

        // skip non-space, non-newline characters
        while( mCursorPosition > 0 &&
               mText[ mCursorPosition - 1 ] != ' ' &&
               mText[ mCursorPosition - 1 ] != '\r' ) {
            mCursorPosition --;
            }
        
        // skip space and newline characters
        while( mCursorPosition > 0 &&
               ( mText[ mCursorPosition - 1 ] == ' ' ||
                 mText[ mCursorPosition - 1 ] == '\r' ) ) {
            mCursorPosition --;
            }
        
        }
    else {    
        mCursorPosition --;
        if( mCursorPosition < 0 ) {
            mCursorPosition = 0;
            }
        }

    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
        }

    }



void DropdownList::rightHit() {
    mCursorFlashSteps = 0;
    
    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionEnd;
            }
        else {
            mCursorPosition = *mSelectionAdjusting;
            }
        }
    
    if( ! isShiftKeyDown() ) {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionEnd - 1;
            }
            
        mSelectionStart = -1;
        mSelectionEnd = -1;
        }

    if( isCommandKeyDown() ) {
        // word jump 
        int textLen = strlen( mText );
        
        // skip space and newline characters
        while( mCursorPosition < textLen &&
               ( mText[ mCursorPosition ] == ' ' ||
                 mText[ mCursorPosition ] == '\r'  ) ) {
            mCursorPosition ++;
            }

        // skip non-space and non-newline characters
        while( mCursorPosition < textLen &&
               mText[ mCursorPosition ] != ' ' &&
               mText[ mCursorPosition ] != '\r' ) {
            mCursorPosition ++;
            }
        
        
        }
    else {
        mCursorPosition ++;
        if( mCursorPosition > (int)strlen( mText ) ) {
            mCursorPosition = strlen( mText );
            }
        }

    if( isShiftKeyDown() && mShiftPlusArrowsCanSelect ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
        }
    
    }




void DropdownList::specialKeyDown( int inKeyCode ) {
    if( !mFocused ) {
        return;
        }
    
    mCursorFlashSteps = 0;
    
    switch( inKeyCode ) {
        case MG_KEY_DOWN:
            hoverIndex++;
            if( hoverIndex >= listLen ) hoverIndex--;
            if( hoverIndex - startIndex >= listLenDisplayed ) {
                startIndex++;
                }
            if( startIndex > listLen - listLenDisplayed ) {
                startIndex = listLen - listLenDisplayed;
                }
            if( startIndex < 0 ) {
                startIndex = 0;
                }
            break;
        case MG_KEY_UP:
            hoverIndex--;
            if( hoverIndex < 0 ) hoverIndex++;
            if( hoverIndex - startIndex < 0 ) {
                startIndex--;
                }
            if( startIndex < 0 ) {
                startIndex = 0;
                }
            break;
        case MG_KEY_LEFT:
            if( ! mIgnoreArrowKeys ) {    
                leftHit();
                clearArrowRepeat();
                mHoldArrowSteps[0] = 0;
                }
            break;
        case MG_KEY_RIGHT:
            if( ! mIgnoreArrowKeys ) {
                rightHit(); 
                clearArrowRepeat();
                mHoldArrowSteps[1] = 0;
                }
            break;
        default:
            break;
        }
    
    }



void DropdownList::specialKeyUp( int inKeyCode ) {
    if( inKeyCode == MG_KEY_LEFT ) {
        mHoldArrowSteps[0] = -1;
        mFirstArrowRepeatDone[0] = false;
        }
    else if( inKeyCode == MG_KEY_RIGHT ) {
        mHoldArrowSteps[1] = -1;
        mFirstArrowRepeatDone[1] = false;
        }
    }



void DropdownList::focus() {
    
    if( sFocusedDropdownList != NULL && sFocusedDropdownList != this ) {
        // unfocus last focused
        sFocusedDropdownList->unfocus();
        }
        
    TextField::unfocusAll();

    mFocused = true;
    sFocusedDropdownList = this;

    mContentsHidden = false;
    }



void DropdownList::unfocus() {
    mFocused = false;

    startIndex = 0;
 
    // hold-down broken if not focused
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();

    if( sFocusedDropdownList == this ) {
        sFocusedDropdownList = NULL;
        if( mFireOnLeave ) {
            fireActionPerformed( this );
            }
        }    
    }



char DropdownList::isFocused() {
    return mFocused;
    }



void DropdownList::setDeleteRepeatDelays( int inFirstDelaySteps,
                                       int inNextDelaySteps ) {
    sDeleteFirstDelaySteps = inFirstDelaySteps;
    sDeleteNextDelaySteps = inNextDelaySteps;
    }



char DropdownList::isAnyFocused() {
    if( sFocusedDropdownList != NULL ) {
        return true;
        }
    return false;
    }


        
void DropdownList::unfocusAll() {
    
    if( sFocusedDropdownList != NULL ) {
        // unfocus last focused
        sFocusedDropdownList->unfocus();
        }

    sFocusedDropdownList = NULL;
    }




void DropdownList::setLabelSide( char inLabelOnRight ) {
    mLabelOnRight = inLabelOnRight;
    }



void DropdownList::setLabelTop( char inLabelOnTop ) {
    mLabelOnTop = inLabelOnTop;
    }


        
char DropdownList::isAnythingSelected() {
    return 
        ( mSelectionStart != -1 && 
          mSelectionEnd != -1 &&
          mSelectionStart != mSelectionEnd );
    }



char *DropdownList::getSelectedText() {

    if( ! isAnythingSelected() ) {
        return NULL;
        }
    
    char *textCopy = stringDuplicate( mText );

    textCopy[ mSelectionEnd ] = '\0';
    
    char *startPointer = &( textCopy[ mSelectionStart ] );
    
    char *returnVal = stringDuplicate( startPointer );
    
    delete [] textCopy;
    
    return returnVal;
    }



void DropdownList::fixSelectionStartEnd() {
    if( mSelectionEnd < mSelectionStart ) {
        int temp = mSelectionEnd;
        mSelectionEnd = mSelectionStart;
        mSelectionStart = temp;

        if( mSelectionAdjusting == &mSelectionStart ) {
            mSelectionAdjusting = &mSelectionEnd;
            }
        else if( mSelectionAdjusting == &mSelectionEnd ) {
            mSelectionAdjusting = &mSelectionStart;
            }
        }
    else if( mSelectionEnd == mSelectionStart ) {
        mSelectionAdjusting = &mSelectionEnd;
        }
    
    }



void DropdownList::setShiftArrowsCanSelect( char inCanSelect ) {
    mShiftPlusArrowsCanSelect = inCanSelect;
    }



void DropdownList::usePasteShortcut( char inShortcutOn ) {
    mUsePasteShortcut = inShortcutOn;
    }
    
    
void DropdownList::useClearButton( char inClearButtonOn ) {
    mUseClearButton = inClearButtonOn;
    }


char DropdownList::isMouseOver() {
    return mHover;
    }
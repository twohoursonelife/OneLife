#include "ClickableComponent.h"

#include "minorGems/game/drawUtils.h"

#include "minorGems/game/game.h"

extern float gui_fov_scale_hud;
extern doublePair lastScreenViewCenter;


ClickableComponent::ClickableComponent(
        )
        : 
        mHover( false ),
        mActive( false ),

        // mTestPosition( {0, 0} ),

        mTopLeft( {0, 0} ),
        mBottomRight( {0, 0} )

        {
    

    }

char ClickableComponent::isInside( float inX, float inY ) {
    doublePair TL = add(mTopLeft, lastScreenViewCenter);
    doublePair BR = add(mBottomRight, lastScreenViewCenter);
    return TL.x <= inX && inX <= BR.x &&
           TL.y >= inY && inY >= BR.y;
    }

void ClickableComponent::pointerMove( float inX, float inY ) {
    if( isCommandKeyDown() ) {
        // mTestPosition = {inX, inY};
        // printf( " ========================= cursor at %.2f %.2f\n", inX, inY );
        }
    if( mActive && isInside( inX, inY ) ) {
        mHover = true;
        }
    else {
        mHover = false;
        }
    }

// return true if captured
char ClickableComponent::pointerDown( float inX, float inY ) {
    if( !mActive ) return false;
    if( isInside( inX, inY ) ) {
        return true;
        }
    return false;
    }

void ClickableComponent::setClickableArea( doublePair inTopLeft, doublePair inBottomRight ) {
    mTopLeft = inTopLeft;
    mBottomRight = inBottomRight;
    }

void ClickableComponent::drawClickableArea() {
    doublePair TL = add(mTopLeft, lastScreenViewCenter);
    doublePair BR = add(mBottomRight, lastScreenViewCenter);
    setDrawColor( 0, 0, 1, 0.5 );
    drawRect(TL.x, BR.y, BR.x, TL.y);
    }







#include "ClickableComponent.h"

#include "minorGems/game/drawUtils.h"

#include "minorGems/game/game.h"

extern float gui_fov_scale_hud;


ClickableComponent::ClickableComponent(
        )
        : 
        mHover( false ),
        mActive( false ),

        mTestPosition( {0, 0} ),

        mTopLeft( {0, 0} ),
        mBottomRight( {0, 0} )

        {
    

    }

char ClickableComponent::isInside( float inX, float inY ) {
    return mTopLeft.x <= inX && inX <= mBottomRight.x &&
           mTopLeft.y >= inY && inY >= mBottomRight.y;
    }

void ClickableComponent::pointerMove( float inX, float inY ) {
    if( isCommandKeyDown() ) {
        mTestPosition = {inX, inY};
        printf( " ========================= cursor at %.2f %.2f\n", inX, inY );
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
    setDrawColor( 0, 0, 1, 0.5 );
    drawRect(mTopLeft.x, mBottomRight.y, mBottomRight.x, mTopLeft.y);
    }







#ifndef CLICKABLECOMPONENT_INCLUDED
#define CLICKABLECOMPONENT_INCLUDED

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"



class ClickableComponent {
    public:

        ClickableComponent( 
            );
        
        ~ClickableComponent() {
            };        

        char mHover;
        char mActive;

        void pointerMove( float inX, float inY );
        char pointerDown( float inX, float inY );
        void setClickableArea( doublePair inTopLeft, doublePair inBottomRight );
        void drawClickableArea();

        // doublePair mTestPosition;
        
    protected:
        doublePair mTopLeft;
        doublePair mBottomRight;

        char isInside( float inX, float inY );

    };


#endif
        

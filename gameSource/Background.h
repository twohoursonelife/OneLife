#ifndef BACKGROUND_INCLUDED
#define BACKGROUND_INCLUDED


#include "PageComponent.h"

#include "minorGems/game/gameGraphics.h"
#include "minorGems/ui/event/ActionListenerList.h"


class Background : public PageComponent, public ActionListenerList {
        
    public:

        Background( const char *inImageName, float inOpacity = 1.0f, doublePair inPosition = {0, 0} );
        
        
        virtual void setImage( const char *inImageName ) {
            mImage = loadSprite( inImageName, false );
            }
        
        
        
        virtual void draw();
        

        
    protected:
        SpriteHandle mImage;

        float mOpacity;

        doublePair mPosition;
        
    };


#endif
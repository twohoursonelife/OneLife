#include "Background.h"


Background::Background( const char *inImageName, float inOpacity, doublePair inPosition )
        : PageComponent( 0, 0 ),
          mImage( loadSprite( inImageName, false ) ),
          mOpacity( inOpacity ),
          mPosition( inPosition ) {
    }

   
void Background::draw() {
    setDrawColor( 1, 1, 1, mOpacity );
    
    if ( mImage != NULL ) {
        drawSprite( mImage, mPosition );
        }
    }
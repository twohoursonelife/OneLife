#include "PageComponent.h"

#include "minorGems/game/game.h"

#include "minorGems/game/drawUtils.h"

extern Font *tinyHandwritingFontFixedSize;
extern double viewWidth;

PageComponent::PageComponent( double inX, double inY )
        : mX( inX ), mY( inY ), mParent( NULL ), mVisible( true ),
          mIgnoreEvents( false ),
          mMouseEventHog( NULL ),
          mCursorTip( NULL ) {
    
    }
        


doublePair PageComponent::getCenter() {
    doublePair c = { mX, mY };

    return c;
    }



void PageComponent::setParent( PageComponent *inParent ) {
    mParent = inParent;
    }



void PageComponent::setToolTip( const char *inTip ) {
    if( mParent != NULL && mCursorTip == NULL ) {
        mParent->setToolTip( inTip );
        }
    }



void PageComponent::clearToolTip( const char *inTipToClear ) {
    if( mParent != NULL ) {
        mParent->clearToolTip( inTipToClear );
        }
    }



void PageComponent::base_step(){
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
        
        if( c->isVisible() && c->isActive() ) {
            c->base_step();
            }
        }
    
    step();
    }


doublePair pointerPos = { 0, 0 };

void PageComponent::base_draw( doublePair inViewCenter, 
                               double inViewSize ){

    doublePair oldViewCenter = getViewCenterPosition();

    setViewCenterPosition( oldViewCenter.x - mX, 
                           oldViewCenter.y - mY );
    
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
    
        if( c->isVisible() ) {
            c->base_draw( inViewCenter, inViewSize );
            }
        }

    draw();
    
    if( mCursorTip != NULL && isMouseOver() ) {
        
        float textWidth = tinyHandwritingFontFixedSize->measureString( mCursorTip );
        
        doublePair pos = pointerPos;
        pos.x += 8 * 2;
        pos.y -= 8 * 2;
        
        pos.x -= mX;
        pos.y -= mY;
        
        // Alternatively, fixed tips position bottom left of element
        // doublePair pos = {mWide / 2, - mHigh / 2};
        // pos.x += - 8 * 2;
        // pos.y -= - 8 * 2;
        
        float padding = 5;
        // Tip never goes off screen, not working well
        // double rightBorderX = double(viewWidth / 4 * 0.85 );
        // if( pos.x + textWidth + padding > rightBorderX ) pos.x = rightBorderX - (textWidth + padding);
        
        setDrawColor( 0, 0, 0, 1.0 );
        drawRect( pos.x - padding, pos.y - 8 / 2 - padding, 
                  pos.x + textWidth + padding, pos.y + 8 / 2 + padding );
        
        setDrawColor( 1, 1, 1, 1.0 );
        tinyHandwritingFontFixedSize->drawString( mCursorTip, pos, alignLeft );
        }

    setViewCenterPosition( oldViewCenter.x, oldViewCenter.y );
    }



void PageComponent::setPosition( double inX, double inY ) {
    mX = inX;
    mY = inY;
    }



doublePair PageComponent::getPosition() {
    doublePair p;
    p.x = mX;
    p.y = mY;
    return p;
    }



void PageComponent::base_clearState(){
    
    mMouseEventHog = NULL;

    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );

        c->base_clearState();
        }


    clearState();
    }



void PageComponent::setIgnoreEvents( char inIgnoreEvents ) {
    mIgnoreEvents = inIgnoreEvents;
    }
    
    
void PageComponent::setCursorTip( const char *inTip ) {
    mCursorTip = inTip;
    }



void PageComponent::base_pointerMove( float inX, float inY ){
    
    pointerPos = {inX, inY};
    
    // Displaying cursor tips even when mIgnoreEvents is true
    if( mIgnoreEvents && mCursorTip == NULL ) {
        return;
        }
    
    inX -= mX;
    inY -= mY;

    if( mMouseEventHog != NULL ) {
        // Need to display cursor tips even when inactive
        if( mMouseEventHog->isVisible() ) {//&& mMouseEventHog->isActive() ) {
            mMouseEventHog->base_pointerMove( inX, inY );
            }
        }
    else {    
        for( int i=0; i<mComponents.size(); i++ ) {
            PageComponent *c = *( mComponents.getElement( i ) );
            
            // Need to display cursor tips even when inactive
            if( c->isVisible() ) {//&& c->isActive() ) {
                c->base_pointerMove( inX, inY );
                }
            }
        }
    
    pointerMove( inX, inY );
    }



void PageComponent::base_pointerDown( float inX, float inY ){
    if( mIgnoreEvents ) {
        return;
        }
    
    inX -= mX;
    inY -= mY;
    
    if( mMouseEventHog != NULL ) {
        if( mMouseEventHog->isVisible() && mMouseEventHog->isActive() ) {
            mMouseEventHog->base_pointerDown( inX, inY );
            }
        }
    else { 
        for( int i=0; i<mComponents.size(); i++ ) {
            PageComponent *c = *( mComponents.getElement( i ) );
            
            if( c->isVisible() && c->isActive() ) {
                c->base_pointerDown( inX, inY );
                }
            }
        }
    
    pointerDown( inX, inY );
    }



void PageComponent::base_pointerDrag( float inX, float inY ){
    if( mIgnoreEvents ) {
        return;
        }
    
    inX -= mX;
    inY -= mY;
    
    if( mMouseEventHog != NULL ) {
        if( mMouseEventHog->isVisible() && mMouseEventHog->isActive() ) {
            mMouseEventHog->base_pointerDrag( inX, inY );
            }
        }
    else {
        for( int i=0; i<mComponents.size(); i++ ) {
            PageComponent *c = *( mComponents.getElement( i ) );
            
            if( c->isVisible() && c->isActive() ) {
                c->base_pointerDrag( inX, inY );
                }
            }
        }

    pointerDrag( inX, inY );
    }



void PageComponent::base_pointerUp( float inX, float inY ){
    inX -= mX;
    inY -= mY;

    if( mMouseEventHog != NULL ) {
        if( mMouseEventHog->isVisible() && mMouseEventHog->isActive() ) {
            mMouseEventHog->base_pointerUp( inX, inY );
            }
        }
    else {
        for( int i=0; i<mComponents.size(); i++ ) {
            PageComponent *c = *( mComponents.getElement( i ) );
            
            if( c->isVisible() && c->isActive() ) {
                c->base_pointerUp( inX, inY );
                }
            }
        }

    pointerUp( inX, inY );
    }



void PageComponent::base_keyDown( unsigned char inASCII ){
    if( mIgnoreEvents ) {
        return;
        }
    
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
    
        if( c->isVisible() && c->isActive() ) {
            c->base_keyDown( inASCII );
            }
        }

    keyDown( inASCII );
    }


        
void PageComponent::base_keyUp( unsigned char inASCII ){
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
    
        if( c->isVisible() && c->isActive() ) {
            c->base_keyUp( inASCII );
            }
        }

    keyUp( inASCII );
    }



void PageComponent::base_specialKeyDown( int inKeyCode ){
    if( mIgnoreEvents ) {
        return;
        }
    
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
    
        if( c->isVisible() && c->isActive() ) {
            c->base_specialKeyDown( inKeyCode );
            }
        }

    specialKeyDown( inKeyCode );
    }



void PageComponent::base_specialKeyUp( int inKeyCode ){
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
    
        if( c->isVisible() && c->isActive() ) {
            c->base_specialKeyUp( inKeyCode );
            }
        }

    specialKeyUp( inKeyCode );
    }



void PageComponent::addComponent( PageComponent *inComponent ){

    mComponents.push_back( inComponent );

    inComponent->setParent( this );
    }


void PageComponent::removeComponent( PageComponent *inComponent ){

    mComponents.deleteElementEqualTo( inComponent );

    inComponent->setParent( NULL );
    }



void PageComponent::setWaiting( char inWaiting,
                                char inWarningOnly ) {
    // pass up chain (stops at GamePage)
    if( mParent != NULL ) {
        mParent->setWaiting( inWaiting, inWarningOnly );
        }
    }



void PageComponent::setHogMouseEvents( char inHogMouseEvents ) {
    PageComponent *newHog = this;
    
    
    if( ! inHogMouseEvents ) {
        newHog = NULL;
        }
    
    if( mParent != NULL ) {
        mParent->setMouseEventHog( newHog );
        }
    }



void PageComponent::setMouseEventHog( PageComponent *inHog ) {
    mMouseEventHog = inHog;
    }

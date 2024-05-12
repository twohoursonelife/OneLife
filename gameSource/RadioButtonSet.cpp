
#include "RadioButtonSet.h"

#include "minorGems/util/stringUtils.h"


static int baseCheckboxSep = 6;


RadioButtonSet::RadioButtonSet( Font *inDisplayFont, double inX, double inY,
                                int inNumItems, const char **inItemNames,
                                char inRightLabels,
                                double inDrawScale,
                                char inDrawNamesWithShadow )
        : PageComponent( inX, inY ),
          mHover( false ), 
          mDisplayFont( inDisplayFont ),
          mNumItems( inNumItems ),
          mItemNames( new char *[ inNumItems ] ),
          mCheckboxes( new CheckboxButton *[ inNumItems ] ),
          mRightLabels( inRightLabels ),
          mCheckboxSep( inDrawScale * baseCheckboxSep ),
          mSelectedItem( 0 ),
          mDrawNamesWithShadow( inDrawNamesWithShadow ) {

    
    for( int i=0; i<mNumItems; i++ ) {
        mItemNames[i] = stringDuplicate( inItemNames[i] );
        
        mCheckboxes[i] = new CheckboxButton( 0, 0 - inDrawScale * 10 * i,
                                             inDrawScale );
        
        addComponent( mCheckboxes[i] );
        mCheckboxes[i]->addActionListener( this );
        }

    mCheckboxes[mSelectedItem]->setToggled( true );
    }

          

RadioButtonSet::~RadioButtonSet() {
    for( int i=0; i<mNumItems; i++ ) {
        delete [] mItemNames[i];
        delete mCheckboxes[i];
        }
    delete [] mItemNames;
    delete [] mCheckboxes;
    }



void RadioButtonSet::setActive( char inActive ) {
    for( int i=0; i<mNumItems; i++ ) {
        mCheckboxes[i]->setActive( inActive );
        }
    }



char RadioButtonSet::isActive() {
    // return active status of first checkbox that exists
    for( int i=0; i<mNumItems; i++ ) {
        return mCheckboxes[i]->isActive();
        }
    return false;
    }



int RadioButtonSet::getSelectedItem() {
    return mSelectedItem;
    }


void RadioButtonSet::setSelectedItem( int inIndex ) {
    mSelectedItem = inIndex;
    
    for( int i=0; i<mNumItems; i++ ) {
        if( i == mSelectedItem ) {
            mCheckboxes[i]->setToggled( true );
            }
        else {
            // untoggle others
            mCheckboxes[i]->setToggled( false );
            }
        }
    }




void RadioButtonSet::actionPerformed( GUIComponent *inTarget ) {
    for( int i=0; i<mNumItems; i++ ) {
        if( inTarget == mCheckboxes[i] ) {
            if( mSelectedItem != i ) {
                mSelectedItem = i;
                fireActionPerformed( this );
                }
            else {
                // make sure it stays toggled
                mCheckboxes[i]->setToggled( true );
                }
            }
        else {
            // untoggle others
            mCheckboxes[i]->setToggled( false );
            }
        }
    }


void RadioButtonSet::draw() {
    setDrawColor( 1, 1, 1, 1 );

    double sep = -mCheckboxSep;
    
    TextAlignment a = alignRight;
    
    
    if( mRightLabels ) {
        sep = +mCheckboxSep;
        a = alignLeft;
        }
    

    for( int i=0; i<mNumItems; i++ ) {
        doublePair pos = mCheckboxes[i]->getPosition();
    
        pos.x += sep;
        pos.y -= 2;

        if( mDrawNamesWithShadow ) {
            setDrawColor( 0, 0, 0, 1 );
            doublePair shadowOffset = {-2, 2};
            mDisplayFont->drawString( mItemNames[i], add(pos, shadowOffset), a );
            setDrawColor( 1, 1, 1, 1 );
            }
        
        mDisplayFont->drawString( mItemNames[i], pos, a );
        }
    }


void RadioButtonSet::pointerMove( float inX, float inY ) {
    mHover = false;
    for( int i=0; i<mNumItems; i++ ) {
        if( mCheckboxes[i]->isMouseOver() ) mHover = true;
        }
    }


char RadioButtonSet::isMouseOver() {
    return mHover;
    }

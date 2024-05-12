#ifndef RADIO_BUTTON_SET_INCLUDED
#define RADIO_BUTTON_SET_INCLUDED


#include "CheckboxButton.h"
#include "minorGems/game/Font.h"


// action fired when sound or volume changes internally
class RadioButtonSet : public PageComponent, public ActionListenerList,
                       public ActionListener {
        
    public:
        
        RadioButtonSet( Font *inDisplayFont, double inX, double inY,
                        int inNumItems, const char **inItemNames,
                        // true to put labels on right side
                        char inRightLabels = false,
                        double inDrawScale = 1.0,
                        char inDrawNamesWithShadow = false );
        

        ~RadioButtonSet();
        

        int getSelectedItem();
        
        void setSelectedItem( int inIndex );


        virtual void setActive( char inActive );
        
        
        virtual char isMouseOver();

        virtual char isActive();
        

    protected:

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw();

        virtual void pointerMove( float inX, float inY );
        
        char mHover;
        

        Font *mDisplayFont;
        
        int mNumItems;
        
        char **mItemNames;
       
        CheckboxButton **mCheckboxes;

        char mRightLabels;
        double mCheckboxSep;
        

        int mSelectedItem;

        char mDrawNamesWithShadow;
    };

    


#endif

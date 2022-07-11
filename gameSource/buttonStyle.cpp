#include "buttonStyle.h"





// OHOL default style - Buttons are grey if setButtonStyle is not called
void setOHOLGreyButtonStyle( Button *inButton ) {

    // Text Color
    inButton->setNoHoverColor( 1, 1, 1, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setHoverColor( 0.886, 0.764, 0.475, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setDragOverColor( 0.828, 0.647, 0.212, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setInactiveColor( 0.5, 0.5, 0.5, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out

    // Fill Color
    inButton->setFillColor( 0.25, 0.25, 0.25, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setHoverFillColor( 0.25, 0.25, 0.25, 1 ); // Originally coded to be equal to FillColor in Button.cpp
    inButton->setDragOverFillColor( 0.1, 0.1, 0.1, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setInactiveFillColor( 0.125, 0.125, 0.125, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    // Border Color
    inButton->setBorderColor( 0.5, 0.5, 0.5, 1 ); // Originally hard-coded in Button.cpp
    inButton->setHoverBorderColor( 0.75, 0.75, 0.75, 1 ); // This is the hard-coded default originally in Button.cpp
    inButton->setDragOverBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally hard-coded in Button.cpp
    inButton->setInactiveBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    inButton->setBracketCoverLength( 16 );
    doublePair shift = { 0, -2 };
    inButton->setContentsShift( shift );
    }
    
    
// OHOL dark style - This is the original setButtonStyle function in this file
void setOHOLBlackButtonStyle( Button *inButton ) {

    // Text Color
    inButton->setNoHoverColor( 1, 1, 1, 1 );
    inButton->setHoverColor( 1, 1, 1, 1 );
    inButton->setDragOverColor( 1, 1, 1, 1 );
    inButton->setInactiveColor( 0.5, 0.5, 0.5, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out

    // Fill Color
    inButton->setFillColor( 0, 0, 0, 1 );
    inButton->setHoverFillColor( 0, 0, 0, 1 ); // Originally coded to be equal to FillColor in Button.cpp
    inButton->setDragOverFillColor( 0, 0, 0, 1 );
    inButton->setInactiveFillColor( 0, 0, 0, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    // Border Color
    inButton->setBorderColor( 0.5, 0.5, 0.5, 1 ); // Originally hard-coded in Button.cpp
    inButton->setHoverBorderColor( 1, 1, 1, 1 );
    inButton->setDragOverBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally hard-coded in Button.cpp
    inButton->setInactiveBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    inButton->setBracketCoverLength( 16 );
    doublePair shift = { 0, -2 };
    inButton->setContentsShift( shift );
    }


// 2HOL style by Defalt
void set2HOLOldButtonStyle( Button *inButton ) {
    
    // Text Color
    inButton->setNoHoverColor( 0, 0, 0, 1 );
    inButton->setHoverColor( 1, 1, 1, 0.5 );
    inButton->setDragOverColor( 1, 1, 0, 1 );
    inButton->setInactiveColor( 0, 0, 0, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out

    // Fill Color
    inButton->setFillColor( 1, 0.75, 0.2, 1 );
    inButton->setHoverFillColor( 1, 0.75, 0.2, 1 ); // Originally coded to be equal to FillColor in Button.cpp
    inButton->setDragOverFillColor( 0, 0, 0, 1 );
    inButton->setInactiveFillColor( 0.5, 0.375, 0.1, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    // Border Color
    inButton->setBorderColor( 0.5, 0.5, 0.5, 1 ); // Originally hard-coded in Button.cpp
    inButton->setHoverBorderColor( 1, 1, 1, 1 );
    inButton->setDragOverBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally hard-coded in Button.cpp
    inButton->setInactiveBorderColor( 0.25, 0.25, 0.25, 1 ); // Originally a black overlay (0, 0, 0, 0.5) is drawn on the button to gray it out
    
    inButton->setBracketCoverLength( 16 );
    doublePair shift = { 0, -2 };
    inButton->setContentsShift( shift );
    }


// Current 2HOL style
void setButtonStyle( Button *inButton ) {
    
    // Text Color
    inButton->setNoHoverColor( 0, 0, 0, 1 );
    inButton->setHoverColor( 0, 0, 0, 1 );
    inButton->setDragOverColor( 0, 0, 0, 1 );
    inButton->setInactiveColor( 0, 0, 0, 1 );

    // Fill Color
    inButton->setFillColor( 1, 0.75, 0.2, 1 );
    inButton->setHoverFillColor( 1, 0.75, 0.2, 1 );
    inButton->setDragOverFillColor( 1, 0.75, 0.2, 1 );
    inButton->setInactiveFillColor( 1, 0.75, 0.2, 1 );
    
    // Border Color
    inButton->setBorderColor( 0.5, 0.5, 0.5, 1 );
    inButton->setHoverBorderColor( 1, 1, 1, 1 );
    inButton->setDragOverBorderColor( 1, 1, 1, 1 );
    inButton->setInactiveBorderColor( 1, 1, 1, 1 );
    
    inButton->setBracketCoverLength( 16 );
    doublePair shift = { 0, -2 };
    inButton->setContentsShift( shift );
    }


// Dark mode style to be used with black background
void setDarkButtonStyle( Button *inButton ) {
    
    // Text Color
    inButton->setNoHoverColor( 0.5, 0.5, 0.5, 1 );
    inButton->setHoverColor( 1, 1, 1, 1 );
    inButton->setDragOverColor( 1, 1, 1, 1 );
    inButton->setInactiveColor( 0.5, 0.5, 0.5, 1 );

    // Fill Color
    inButton->setFillColor( 0, 0, 0, 1 );
    inButton->setHoverFillColor( 0, 0, 0, 1 );
    inButton->setDragOverFillColor( 0, 0, 0, 1 );
    inButton->setInactiveFillColor( 0, 0, 0, 1 );
    
    // Border Color
    inButton->setBorderColor( 0.5, 0.5, 0.5, 1 );
    inButton->setHoverBorderColor( 1, 1, 1, 1 );
    inButton->setDragOverBorderColor( 1, 1, 1, 1 );
    inButton->setInactiveBorderColor( 0, 0, 0, 1 );
    
    inButton->setBracketCoverLength( 16 );
    doublePair shift = { 0, -2 };
    inButton->setContentsShift( shift );
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
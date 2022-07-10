#include "Button.h"
#include "TextButton.h"


// OHOL default style - Buttons are grey if setButtonStyle is not called
void setOHOLGreyButtonStyle( Button *inButton );

    
// OHOL dark style - This is the original setButtonStyle function in this file
void setOHOLDarkButtonStyle( Button *inButton );

    
// 2HOL style by Defalt
void set2HOLOldButtonStyle( Button *inButton );

    
// Current 2HOL style
void setButtonStyle( Button *inButton );

    
// Dark mode style to be used with black background
void setDarkButtonStyle( Button *inButton );
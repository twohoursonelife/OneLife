#ifndef EDITOR_SCENE_PAGE_INCLUDED
#define EDITOR_SCENE_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"

#include "minorGems/io/file/File.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"
#include "ValueSlider.h"

#include "objectBank.h"
#include "animationBank.h"

#include "keyLegend.h"

#include "RadioButtonSet.h"

#include <vector>


typedef struct SceneCell {
        int biome;
        
        int oID;

        int heldID;
        
        SimpleVector<int> contained;
        SimpleVector< SimpleVector<int> > subContained;

        ClothingSet clothing;

        char flipH;
        double age;
        
        double heldAge;
        ClothingSet heldClothing;
        Emotion *heldEmotion;
        SimpleVector<Emotion*> heldExtraEmotion;
        
        double returnAge;
        double returnHeldAge;

        AnimType anim;
        
        // negative to unfreeze time for cell, or positive to freeze
        // time at a given point
        double frozenAnimTime;
        
        // for vanishing/appearing sprites based on use
        int numUsesRemaining;

        // for vanishing sprites based on variable object number
        int varNumber;
        

        int xOffset;
        int yOffset;
        
        // for moving cell, where they should end up
        int destCellXOffset;
        int destCellYOffset;

        // for live display only (not saved)
        double moveFractionDone;

        // current move offset in pixels
        doublePair moveOffset;
        
        double moveDelayTime;
        
        double moveStartTime;
        
        int frameCount;
        

        int graveID;
        
        Emotion *currentEmot;
        SimpleVector<Emotion*> extraEmot;
        
    } SceneCell;


    

class EditorScenePage : public GamePage, public ActionListener {
        
    public:
        EditorScenePage();
        ~EditorScenePage();
    
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        
        virtual void makeActive( char inFresh );
        
        virtual void step();

		virtual void pointerUp( float inX, float inY );
		virtual void pointerDown( float inX, float inY );
		virtual void pointerMove( float inX, float inY );
		virtual void pointerDrag( float inX, float inY );

        virtual void keyDown( unsigned char inASCII );
        virtual void keyUp( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
        
    protected:
        char mPlayingTime;
        char mRecordingFrames;
        
		TextButton mUndoButton;
		TextButton mRedoButton;
		
        TextButton mAnimEditorButton;
        
        TextButton mSaveNewButton;
        TextButton mReplaceButton;
		TextButton mConfirmReplaceButton;
        TextButton mDeleteButton;
		TextButton mConfirmDeleteButton;
        
        TextButton mSaveTestMapButton;

        TextButton mNextSceneButton;
        TextButton mPrevSceneButton;


        TextButton mClearSceneButton;

        Picker mGroundPicker;
        Picker mObjectPicker;
        
        ValueSlider mPersonAgeSlider;
        
        RadioButtonSet mCellAnimRadioButtons;
        RadioButtonSet mPersonAnimRadioButtons;
        
        ValueSlider mCellAnimFreezeSlider;
        ValueSlider mPersonAnimFreezeSlider;
        
        ValueSlider mCellSpriteVanishSlider;
        ValueSlider mCellSpriteVarSlider;

        ValueSlider mCellXOffsetSlider;
        ValueSlider mCellYOffsetSlider;

        ValueSlider mPersonXOffsetSlider;
        ValueSlider mPersonYOffsetSlider;
        
        TextField mCellMoveDelayField;
        TextField mPersonMoveDelayField;
        
        TextField mPersonEmotField;

        
        SpriteHandle mCellDestSprite;
        SpriteHandle mPersonDestSprite;

        SpriteHandle mGroundOverlaySprite[4];
        
        SpriteHandle mFloorSplitSprite;
        

        char mShowUI;
        char mShowWhite;
        
        float mCursorFade;
        
        int mSceneW, mSceneH;
        
        int mSceneID;

        SceneCell **mCells;
        SceneCell **mPersonCells;
        SceneCell **mFloorCells;
        
        SceneCell mEmptyCell;
        SceneCell mCopyBuffer;
        SceneCell mCopyFloorBuffer;
        
        
        // when we get near edge of screen with cell selection, we
        // shift everything into the center
        int mShiftX, mShiftY;
        
        int mCurX, mCurY;
		int cursorGridX, cursorGridY;
		int cursorX, cursorY;
        
        // location of origing in scene
        int mZeroX, mZeroY;

        double mFrameCount;
        
        char mLittleDheld;
        char mBigDheld;

        File mScenesFolder;
        File *mNextFile;
        int mNextSceneNumber;
        

        KeyLegend mKeyLegend, mKeyLegendG, mKeyLegendC, mKeyLegendP, 
            mKeyLegendF;
		
		bool mObjectPickerClicked;
		bool mGroundPickerClicked;
		int pickedOID;
		int pickedGID;


        void floodFill( int inX, int inY, int inOldBiome, int inNewBiome );
        

        void drawGroundOverlaySprites();
        
        // check which GUI components should be visible
        void checkVisible();
		
		bool hoverAnyUI( float inX, float inY );

        SceneCell *getCurrentCell();
        SceneCell *getCurrentPersonCell();
        SceneCell *getCurrentFloorCell();
		SceneCell *getCell(int absX, int absY);
		SceneCell *getFloorCell(int absX, int absY);
		
		void destroySceneCellArray( SceneCell ***array );
		SceneCell **copySceneCellArray( SceneCell **array );
		void queuesPopBack();
		void queuesPopFront();
		void applySceneCellArrays( SceneCell **cells, SceneCell **floorCells );
		
        void mark( int x, int y, int beforeOrAfter );
		void backup();
		void undo();
		void redo();
        void flashTile( int x, int y );
        
        // clear everything but biome
        void clearCell( SceneCell *inCell );
        
        void clearScene();
        

        void restartAllMoves();
        
        std::vector<File*> getSceneFiles();
        
        int getSceneFileID( char *fileName );

        File *getSceneFile( int inSceneID );
        
        char tryLoadScene( int inSceneID );
        
        void writeSceneToFile( int inIDToUse );
        
        void checkNextPrevVisible();
        
        void resizeGrid( int inNewH, int inNewW );
    };

        


#endif

#include "TownplannerScenePage.h"
#include <math.h>
#include <vector>

#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/io/file/File.h"

#include "minorGems/system/Time.h"

#include "minorGems/graphics/openGL/ScreenGL.h"

double scale = 1.0;
int xLimit = 7/scale/scale;
int yLimit = 4/scale/scale;
int cursorDim = 32*scale;
int rectDim = 64*scale;

#include "GridPos.h"
int pickedOID;
int pickedGID;
std::vector<GridPos> fadingTiles;
std::vector<float> fadingTilesFade;

int mapSizeX = 400;
int mapSizeY = 400;
int initCenterX = 200;
int initCenterY = 200;

int queueCapacity = 8;

int spriteCount;
bool mapChanged = false;

extern double viewHeightFraction;
extern double viewWidth;
double viewHeight = 720;

extern Font *mainFont;
extern Font *smallFont;


#include "GroundPickable.h"

static GroundPickable groundPickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;


static double multAmount = 0.15;
static double addAmount = 0.25;

extern double frameRateFactor;

#define NUM_CELL_ANIM 2
static const char *cellAnimNames[ NUM_CELL_ANIM ] = { "Ground", "Moving" };
static const AnimType cellAnimTypes[ NUM_CELL_ANIM ] = { ground, moving };

#define NUM_PERSON_ANIM 4
static const char *personAnimNames[ NUM_PERSON_ANIM ] = 
    { "Ground", "Moving", "Eating", "Doing" };
static const AnimType personAnimTypes[ NUM_PERSON_ANIM ] = 
    { ground, moving, eating, doing };


#define CELL_D 128

#define MAX_OFFSET 2 * CELL_D

static doublePair cornerPos = { 0, 0 }; //{ - 704, 360 };

void setScale(double inScale) {
    scale = inScale;
    xLimit = 7/scale*2;
    yLimit = 4/scale*2;
    cursorDim = 32*scale;
    rectDim = 64*scale;
    }

#define copyAreaSize 6

static char copyAreaSet = false;
static SceneCell copyArea[ copyAreaSize ][ copyAreaSize ];
static SceneCell copyFloorArea[ copyAreaSize ][ copyAreaSize ];
static SceneCell copyPeopleArea[ copyAreaSize ][ copyAreaSize ];



EditorScenePage::EditorScenePage()
        : mPlayingTime( false ),
          mRecordingFrames( false ),
          
          // mUndoButton( smallFont, -877, 340, "Undo" ),
          // mRedoButton( smallFont, -817, 340, "Redo" ),
          mUndoButton( smallFont, -(int)( viewWidth/2 * 0.75 ) - 55, (int)( viewWidth*viewHeightFraction * 0.32 ), "Undo" ),
          mRedoButton( smallFont, mUndoButton.getPosition().x + 60, mUndoButton.getPosition().y, "Redo" ),
          
          
          
          mAnimEditorButton( mainFont, 210, 260, "Anim" ),
          
          // mGroundPicker( &groundPickable, -410, 90 ),
          // mObjectPicker( &objectPickable, 410, 90 ),
          // mGroundPicker( &groundPickable, 820, 90 ),
          // mObjectPicker( &objectPickable, -820, 90 ),
          mGroundPicker( &groundPickable, (int)( viewWidth/2 * 0.75 ), (int)( viewWidth*viewHeightFraction * 0.15 ) ),
          mObjectPicker( &objectPickable, (int)( -viewWidth/2 * 0.75 ), (int)( viewWidth*viewHeightFraction * 0.15 ) ),
          
          
          
          // mDeleteButton( smallFont, 820, 220, "Delete file" ),
          mDeleteButton( smallFont, (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 70, "Delete file" ),
          mConfirmDeleteButton( smallFont, mDeleteButton.getPosition().x, mDeleteButton.getPosition().y, "Confirm ?" ),
          
          
          // mSaveTestMapButton( smallFont, -300, 200, "Export Test Map" ),
          // mSaveTestMapButton( smallFont, 820, 260, "Export Test Map" ),
          mSaveTestMapButton( smallFont, (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 35, "Export Test Map" ),
          
          
          // mSaveNewButton( smallFont, -300, 260, "Save New" ),
          // mSaveNewButton( smallFont, 820 + 30, 300, "Save New" ),
          mSaveNewButton( smallFont, (int)( viewWidth/2 * 0.75 ) + 30, mUndoButton.getPosition().y, "Save New" ),
          
          
          // mClearSceneButton( smallFont, 820 - 50, 300, "New" ),
          mClearSceneButton( smallFont, (int)( viewWidth/2 * 0.75 ) - 50, mUndoButton.getPosition().y, "New" ),
          

          // mReplaceButton( smallFont, -500, 260, "Replace" ),
          // mReplaceButton( smallFont, 820, 340, "Save" ),
          mReplaceButton( smallFont, (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y + 35, "Save" ),
          mConfirmReplaceButton( smallFont, mReplaceButton.getPosition().x, mReplaceButton.getPosition().y, "Confirm ?" ),
          // mNextSceneButton( smallFont, -420, 260, ">" ),
          // mPrevSceneButton( smallFont, -580, 260, "<" ),
          mNextSceneButton( smallFont, mReplaceButton.getPosition().x + 60, mReplaceButton.getPosition().y, ">" ),
          mPrevSceneButton( smallFont, mReplaceButton.getPosition().x - 60, mReplaceButton.getPosition().y, "<" ),
          
          mPersonAgeSlider( smallFont, -55, -220, 2,
                            100, 20,
                            0, 100, "Age" ),
          mCellAnimRadioButtons( smallFont, -200, -250, NUM_CELL_ANIM,
                                 cellAnimNames,
                                 true, 2 ),                                 
          mPersonAnimRadioButtons( smallFont, -55, -250, NUM_PERSON_ANIM,
                                   personAnimNames,
                                   true, 2 ),                            
          mCellAnimFreezeSlider( smallFont, -450, -340, 2,
                                 300, 20,
                                 -2, 2, "Cell Time" ),
          mPersonAnimFreezeSlider( smallFont, 50, -340, 2,
                                   300, 20,
                                   -2, 2, "Person Time" ),
          mCellSpriteVanishSlider( smallFont, -450, -310, 2,
                                   100, 20,
                                   0, 1, "Use" ),
          mCellSpriteVarSlider( smallFont, -250, -310, 2,
                                100, 20,
                                0, 1, "Var" ),
          mCellXOffsetSlider( smallFont, -450, -230, 2,
                              175, 20,
                              -MAX_OFFSET, MAX_OFFSET, "X Offset" ),
          mCellYOffsetSlider( smallFont, -450, -260, 2,
                              175, 20,
                              -MAX_OFFSET, MAX_OFFSET, "Y Offset" ),
          mPersonXOffsetSlider( smallFont, 200, -230, 2,
                              175, 20,
                              -MAX_OFFSET, MAX_OFFSET, "X Offset" ),
          mPersonYOffsetSlider( smallFont, 200, -260, 2,
                              175, 20,
                              -MAX_OFFSET, MAX_OFFSET, "Y Offset" ),
          mCellMoveDelayField( smallFont, -450, -290, 4,
                               false, "Move Delay Sec",
                               "0123456789." ),
          mPersonMoveDelayField( smallFont, 200, -290, 4,
                               false, "Move Delay Sec",
                               "0123456789." ),
          mPersonEmotField( smallFont, 360, -290, 7,
                               true, "Emot",
                               "*/ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789," ),
          mCellDestSprite( loadSprite( "centerMark.tga" ) ),
          mPersonDestSprite( loadSprite( "internalPaperMark.tga" ) ),
          mFloorSplitSprite( loadSprite( "floorSplit.tga", false ) ),
          mShowUI( true ),
          mShowWhite( false ),
          mCursorFade( 1.0 ),
          mSceneW( mapSizeX ),
          mSceneH( mapSizeY ),
          mShiftX( -initCenterX ), 
          mShiftY( initCenterY ),
          mCurX( initCenterX ),
          mCurY( initCenterY ),
          mZeroX( initCenterX ),
          mZeroY( initCenterY ),
          mFrameCount( 0 ),
          mLittleDheld( false ),
          mBigDheld( false ),
          mScenesFolder( NULL, "scenes" ),
          mNextFile( NULL ) {

    addComponent( &mUndoButton );
    mUndoButton.addActionListener( this );
    mUndoButton.setVisible( false );
    
    addComponent( &mRedoButton );
    mRedoButton.addActionListener( this );
    mRedoButton.setVisible( false );
    
    addComponent( &mAnimEditorButton );
    mAnimEditorButton.addActionListener( this );
    mAnimEditorButton.setVisible( false );

    addComponent( &mGroundPicker );
    mGroundPicker.addActionListener( this );

    addComponent( &mObjectPicker );
    mObjectPicker.addActionListener( this );


    addComponent( &mSaveNewButton );
    mSaveNewButton.addActionListener( this );

    addComponent( &mReplaceButton );
    mReplaceButton.addActionListener( this );
    
    addComponent( &mConfirmReplaceButton );
    mConfirmReplaceButton.addActionListener( this );

    addComponent( &mDeleteButton );
    mDeleteButton.addActionListener( this );

    addComponent( &mConfirmDeleteButton );
    mConfirmDeleteButton.addActionListener( this ); 

    addComponent( &mSaveTestMapButton );
    mSaveTestMapButton.addActionListener( this );


    addComponent( &mNextSceneButton );
    mNextSceneButton.addActionListener( this );

    addComponent( &mPrevSceneButton );
    mPrevSceneButton.addActionListener( this );

    addComponent( &mClearSceneButton );
    mClearSceneButton.addActionListener( this );

    mReplaceButton.setVisible( false );
    mConfirmReplaceButton.setVisible( false );
    mDeleteButton.setVisible( false );
    mConfirmDeleteButton.setVisible( false );
    

    addComponent( &mPersonAgeSlider );
    mPersonAgeSlider.setValue( 20 );
    mPersonAgeSlider.setVisible( false );
    mPersonAgeSlider.addActionListener( this );
    

    addComponent( &mCellAnimRadioButtons );
    addComponent( &mPersonAnimRadioButtons );

    mCellAnimRadioButtons.setVisible( false );
    mPersonAnimRadioButtons.setVisible( false );

    mCellAnimRadioButtons.addActionListener( this );
    mPersonAnimRadioButtons.addActionListener( this );


    addComponent( &mCellAnimFreezeSlider );
    addComponent( &mPersonAnimFreezeSlider );
    
    mCellAnimFreezeSlider.setVisible( false );
    mCellAnimFreezeSlider.addActionListener( this );
    
    mPersonAnimFreezeSlider.setVisible( false );
    mPersonAnimFreezeSlider.addActionListener( this );
    

    addComponent( &mCellSpriteVanishSlider );
    mCellSpriteVanishSlider.setVisible( false );
    mCellSpriteVanishSlider.addActionListener( this );


    addComponent( &mCellSpriteVarSlider );
    mCellSpriteVarSlider.setVisible( false );
    mCellSpriteVarSlider.addActionListener( this );
    
    
    addComponent( &mCellXOffsetSlider );
    addComponent( &mCellYOffsetSlider );
    
    mCellXOffsetSlider.setVisible( false );
    mCellYOffsetSlider.setVisible( false );
    
    mCellXOffsetSlider.addActionListener( this );
    mCellYOffsetSlider.addActionListener( this );


    addComponent( &mPersonXOffsetSlider );
    addComponent( &mPersonYOffsetSlider );
    
    mPersonXOffsetSlider.setVisible( false );
    mPersonYOffsetSlider.setVisible( false );
    
    mPersonXOffsetSlider.addActionListener( this );
    mPersonYOffsetSlider.addActionListener( this );


    addComponent( &mCellMoveDelayField );
    addComponent( &mPersonMoveDelayField );
    
    addComponent( &mPersonEmotField );
    mPersonEmotField.addActionListener( this );
    mPersonEmotField.setVisible( false );
    

    mCellMoveDelayField.setText( "0.0" );
    mPersonMoveDelayField.setText( "0.0" );
    
    mCellMoveDelayField.setVisible( false );
    mPersonMoveDelayField.setVisible( false );
    
    mCellMoveDelayField.addActionListener( this );
    mPersonMoveDelayField.addActionListener( this );


    if( viewHeight * 0.32 - viewHeight * 0.15 < 180  ) {
        mUndoButton.setPosition( -(int)( viewWidth/2 * 0.75 ) - 55, viewHeight/2 - 90 - 30 );
        mRedoButton.setPosition( mUndoButton.getPosition().x + 60, mUndoButton.getPosition().y  );
        mDeleteButton.setPosition( (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 70  );
        mSaveTestMapButton.setPosition( (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 35  );
        mSaveNewButton.setPosition( (int)( viewWidth/2 * 0.75 ) + 30, mUndoButton.getPosition().y  );
        mClearSceneButton.setPosition( (int)( viewWidth/2 * 0.75 ) - 50, mUndoButton.getPosition().y  );
        mReplaceButton.setPosition( (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y + 35  );
        
        mConfirmReplaceButton.setPosition( mReplaceButton.getPosition().x, mReplaceButton.getPosition().y );
        mNextSceneButton.setPosition( mReplaceButton.getPosition().x + 60, mReplaceButton.getPosition().y );
        mPrevSceneButton.setPosition( mReplaceButton.getPosition().x - 60, mReplaceButton.getPosition().y );
        
        mGroundPicker.setPosition( (int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 180 );
        mObjectPicker.setPosition( -(int)( viewWidth/2 * 0.75 ), mUndoButton.getPosition().y - 180 );
        }


    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }

    
    mEmptyCell.biome = -1;
    clearCell( &mEmptyCell );
    
    mCopyBuffer = mEmptyCell;
    mCopyFloorBuffer = mEmptyCell;

    
    mCells = NULL;
    
    resizeGrid( mSceneH, mSceneW );
    
    spriteCount = round( std::max(mSceneH, mSceneW) / 200 * 25 );

    mSceneID = -1;
    
    
    if( ! mScenesFolder.exists() ) {
        mScenesFolder.makeDirectory();
        }
    
    mNextSceneNumber = 0;
    if( mScenesFolder.isDirectory() ) {
        mNextFile = mScenesFolder.getChildFile( "next.txt" );
        if ( ! mNextFile->exists() ) { // It might change the scene order, and will change the count, when it's created.
            mNextFile->writeToFile( mNextSceneNumber );
            }
        
//        mNextSceneNumber = mNextFile->readFileIntContents( 0 );
        File **sceneFiles = mScenesFolder.getChildFiles( &mNextSceneNumber ); // Get the number of scenes we have.
    for (int i = 0; i<mNextSceneNumber; i++) {
            delete sceneFiles[i];
        }
        delete [] sceneFiles;
        }
    

    checkNextPrevVisible();


    addKeyClassDescription( &mKeyLegend, "Ctrl + L - Click / Shft + WASD", "Move view" );
    addKeyClassDescription( &mKeyLegend, "L - Click", "Add selected" );
    addKeyClassDescription( &mKeyLegend, "None/Ctrl/Shft + R - Click", "Clear object/floor/both" );
    
    addKeyDescription( &mKeyLegend, 'E', "Hide/show UI" );
    addKeyClassDescription( &mKeyLegend, "Z/X", "Undo/redo" );
    addKeyClassDescription( &mKeyLegend, "None/Shft/Ctrl + F", "Pick object/floor/biome" );
    addKeyClassDescription( &mKeyLegend, "None/Shft + C", "Copy cell/area" );
    addKeyClassDescription( &mKeyLegend, "None/Shft + V", "Paste cell/area" );
    addKeyClassDescription( &mKeyLegend, "None/Shft + Q", "Clear cell/area" );
    
    addKeyClassDescription( &mKeyLegend, "Tab", "Focus/Unfocus search" );
    addKeyClassDescription( &mKeyLegend, "Ctrl + W/S", "Change selection in search page" );
    addKeyClassDescription( &mKeyLegend, "Ctrl + A/D", "Next/Prev search page" );
    addKeyClassDescription( &mKeyLegend, "Ctrl + Tab", "Previous selected objects" );

    addKeyClassDescription( &mKeyLegendG, "Shft + L - Click", "Flood fill" );
    addKeyClassDescription( &mKeyLegendC, "Shft + L - Click", "Add to Container" );
    // addKeyClassDescription( &mKeyLegendP, "R-Click", "Add Clothing/Held" );
    // addKeyClassDescription( &mKeyLegendF, "R-Click", "Add Floor" );
    
    }



EditorScenePage::~EditorScenePage() {
    for( int i=0; i<4; i++ ) {
        freeSprite( mGroundOverlaySprite[i] );
        }

    freeSprite( mFloorSplitSprite );
    freeSprite( mCellDestSprite );
    freeSprite( mPersonDestSprite );

    for( int y=0; y<mSceneH; y++ ) {
        delete [] mCells[y];
        delete [] mPersonCells[y];
        delete [] mFloorCells[y];
        }
    delete [] mCells;
    delete [] mPersonCells;
    delete [] mFloorCells;

    if( mNextFile != NULL ) {
        delete mNextFile;
        }
    }



void EditorScenePage::floodFill( int inX, int inY, 
                                 int inOldBiome, int inNewBiome ) {
    
    if( inX < 0 || inX >= mSceneW ||
        inY < 0 || inY >= mSceneH ) {
        return;
        }
    
    // also limit based on cur pos, so we don't fill entire huge canvas
    // (given sparse file format that skips blank cells)
    
    if( inX < cursorGridX - 10 || inX > cursorGridX + 10 ||
        inY < cursorGridY - 10 || inY > cursorGridY + 10 ) {
        return;
        }
    

    if( mCells[inY][inX].biome == inOldBiome &&
        mCells[inY][inX].biome != inNewBiome ) {
        
        mark( inX, inY, 0 );
        mCells[inY][inX].biome = inNewBiome;
        mark( inX, inY, 1 );
        
        floodFill( inX - 1, inY, inOldBiome, inNewBiome );
        floodFill( inX + 1, inY, inOldBiome, inNewBiome );
        
        floodFill( inX, inY - 1, inOldBiome, inNewBiome );
        floodFill( inX, inY + 1, inOldBiome, inNewBiome );
        }
    }

void EditorScenePage::actionPerformed( GUIComponent *inTarget ) {
    
    if (!mShowUI) return;
    
    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();
    // SceneCell *f = getCurrentFloorCell();
    
    if( inTarget == &mAnimEditorButton ) {
        setSignal( "animEditor" );
        }
    else if( inTarget == &mUndoButton ) {
        undo();
        }
    else if( inTarget == &mRedoButton ) {
        redo();
        }
    else if( inTarget == &mGroundPicker ) {
        
        // char gWasRightClick = false;
        // pickedGID = mGroundPicker.getSelectedObject( &gWasRightClick );
        mObjectPicker.unselectObject();
        // pickedOID = 0;
        mGroundPickerClicked = true;

        }
    else if( inTarget == &mObjectPicker ) {
        
        // char oWasRightClick = false;
        // pickedOID = mObjectPicker.getSelectedObject( &oWasRightClick );
        mGroundPicker.unselectObject();
        // pickedGID = -1;
        mObjectPickerClicked = true;
        
        }
    else if( inTarget == &mSaveNewButton ) {

        writeSceneToFile( mNextSceneNumber );
        
        mDeleteButton.setVisible( true );
        mReplaceButton.setVisible( true );
        mNextSceneButton.setVisible( false );
        mapChanged = false;
        checkNextPrevVisible();
        }
    else if( inTarget == &mSaveTestMapButton ) {
        FILE *f = fopen( "testMap.txt", "w" );
        
        if( f != NULL ) {
            
            for( int y=0; y<mSceneH; y++ ) {
                for( int x=0; x<mSceneW; x++ ) {
                    SceneCell *c = &( mCells[y][x] );

                    int oID = c->oID;
                    int floorID = mFloorCells[y][x].oID;
                    
                    if( oID == -1 && c->biome == -1 && floorID == -1 ) {
                        continue;
                        }
                    
                    if( oID == -1 ) {
                        oID = 0;
                        }

                    if( floorID == -1 ) {
                        floorID = 0;
                        }
                    
                    fprintf( f, "%d %d %d %d %d", x - mZeroX, -( y - mZeroY ), 
                             c->biome, floorID, oID );

                    for( int i=0; i< c->contained.size(); i++ ) {
                        fprintf( f, ",%d",
                                 c->contained.getElementDirect( i ) );
                        SimpleVector<int> *subVec =
                            c->subContained.getElement(i);
                        
                        for( int s=0; s< subVec->size(); s++ ) {
                            
                            fprintf( f, ":%d", subVec->getElementDirect( s ) );
                            }
                        }
                    fprintf( f, "\n" );
                    }
                }
            fclose( f );
            }
        }
    else if( inTarget == &mReplaceButton ) {
        mReplaceButton.setVisible( false );
        mConfirmReplaceButton.setVisible( true );
        }
    else if( inTarget == &mConfirmReplaceButton ) {
        writeSceneToFile( mSceneID );
        mReplaceButton.setVisible( true );
        mConfirmReplaceButton.setVisible( false );
        mapChanged = false;
        checkNextPrevVisible();
        }
    else if( inTarget == &mDeleteButton ) {
        mDeleteButton.setVisible( false );
        mConfirmDeleteButton.setVisible( true );
        }
    else if( inTarget == &mConfirmDeleteButton ) {  
        File *f = getSceneFile( mSceneID );
        
        f->remove();
        
        delete f;
        
        mSceneID = -1;
        
        mDeleteButton.setVisible( true );
        mConfirmDeleteButton.setVisible( false );
        mReplaceButton.setVisible( false );
        checkNextPrevVisible();
        }
    else if( inTarget == &mNextSceneButton ) {
        int jump = 1;
        if( isCommandKeyDown() ) {
            jump *= 5;
            }
        if( isShiftKeyDown() ) {
            jump *= 5;
            }

        mSceneID += jump;
        
        // Try to load the scene while we're still in bounds.
        while( ! tryLoadScene( mSceneID ) && mSceneID < mNextSceneNumber ) { 
            mSceneID++;
            }
            
        if( mSceneID >= mNextSceneNumber ) mSceneID = mNextSceneNumber - 1;
            
        mReplaceButton.setVisible( true );
        mDeleteButton.setVisible( true );
        checkNextPrevVisible();
        checkVisible();
        restartAllMoves();
        }
    else if( inTarget == &mPrevSceneButton ) {
        int jump = 1;
        if( isCommandKeyDown() ) {
            jump *= 5;
            }
        if( isShiftKeyDown() ) {
            jump *= 5;
            }
        
        if( mSceneID < 0 ) {
            mSceneID = mNextSceneNumber;
            }
        mSceneID -= jump;
        
        // Try to load the scene while we're still in bounds.
        while( ! tryLoadScene( mSceneID ) && mSceneID > 0 ) {
            mSceneID--;
            }
            
        if( mSceneID < 0 ) mSceneID = 0;
            
        mReplaceButton.setVisible( true );
        mDeleteButton.setVisible( true );
        checkNextPrevVisible();
        checkVisible();
        restartAllMoves();
        }
    else if( inTarget == &mClearSceneButton ) {
        clearScene();
        mSceneID = -1;
        checkVisible();
        }
    else if( inTarget == &mPersonAgeSlider ) {
        p->age = mPersonAgeSlider.getValue();
        p->returnAge = p->age;
        }
    else if( inTarget == &mCellAnimRadioButtons ) {
        c->anim = 
            cellAnimTypes[ mCellAnimRadioButtons.getSelectedItem() ];
        }
    else if( inTarget == &mPersonAnimRadioButtons ) {
        p->anim = 
            personAnimTypes[ mPersonAnimRadioButtons.getSelectedItem() ];
        }
    else if( inTarget == &mCellAnimFreezeSlider ) {
        c->frozenAnimTime = mCellAnimFreezeSlider.getValue();
        }
    else if( inTarget == &mPersonAnimFreezeSlider ) {
        p->frozenAnimTime = mPersonAnimFreezeSlider.getValue();
        }
    else if( inTarget == &mCellSpriteVanishSlider ) {
        c->numUsesRemaining = lrint( mCellSpriteVanishSlider.getValue() );
        mCellSpriteVanishSlider.setValue( c->numUsesRemaining );
        }
    else if( inTarget == &mCellSpriteVarSlider ) {
        c->varNumber = lrint( mCellSpriteVarSlider.getValue() );
        mCellSpriteVarSlider.setValue( c->varNumber );
        }
    else if( inTarget == &mCellXOffsetSlider ) {
        c->xOffset = lrint( mCellXOffsetSlider.getValue() );
        mCellXOffsetSlider.setValue( c->xOffset );
        }
    else if( inTarget == &mCellYOffsetSlider ) {
        c->yOffset = lrint( mCellYOffsetSlider.getValue() );
        mCellYOffsetSlider.setValue( c->yOffset );
        }
    else if( inTarget == &mPersonXOffsetSlider ) {
        p->xOffset = lrint( mPersonXOffsetSlider.getValue() );
        mPersonXOffsetSlider.setValue( p->xOffset );
        }
    else if( inTarget == &mPersonYOffsetSlider ) {
        p->yOffset = lrint( mPersonYOffsetSlider.getValue() );
        mPersonYOffsetSlider.setValue( p->yOffset );
        }
    else if( inTarget == &mCellMoveDelayField ) {
        c->moveDelayTime = mCellMoveDelayField.getFloat();
        restartAllMoves();
        }
    else if( inTarget == &mPersonMoveDelayField ) {
        p->moveDelayTime = mPersonMoveDelayField.getFloat();
        restartAllMoves();
        }
    else if( inTarget == &mPersonEmotField ) {
        char *text = mPersonEmotField.getText();

        if( strstr( text, "/" ) == text ||
            strstr( text, "*" ) == text ) {
            // starts with / or *
            
            int numParts = 0;
            char **parts = split( text, ",", &numParts );
            
            if( numParts > 0 ) {
                p->currentEmot = getEmotion( getEmotionIndex( parts[0] ) );
                delete [] parts[0];
                }
            p->extraEmot.deleteAll();
            for( int i=1; i<numParts; i++ ) {
                Emotion *e = getEmotion( getEmotionIndex( parts[i] ) );
                if( e != NULL ) {
                    p->extraEmot.push_back( e );
                    }
                delete [] parts[i];
                }
            delete [] parts;
            }
        else {
            // check for straight number
            int readNum = -1;
            sscanf( text, "%d", &readNum );
            
            if( readNum >= 0 ) {
                p->currentEmot = getEmotion( readNum );
                }
            }
        delete [] text;
        }
    }


void EditorScenePage::drawGroundOverlaySprites() {
    doublePair overlayCornerPos = cornerPos;
    
    while( overlayCornerPos.x < mCurX * CELL_D*scale - 2048*scale ) {
        overlayCornerPos.x += 2048*scale;
        }
    
    
    while( overlayCornerPos.y > 2048*scale - mCurY * CELL_D*scale ) {
        overlayCornerPos.y -= 2048*scale;
        }
    
    // overlayCornerPos.x += 512*scale - 1024*scale;
    // overlayCornerPos.y -= 512*scale - 1024*scale;

    overlayCornerPos.x += 1024*scale/2 - spriteCount*1024*scale/2;
    overlayCornerPos.y -= 1024*scale/2 - spriteCount*1024*scale/2;

    for( int y=0; y<spriteCount; y++ ) {
        for( int x=0; x<spriteCount; x++ ) {
            doublePair pos = overlayCornerPos;
            pos.x += x * 1024*scale;
            pos.y -= y * 1024*scale;
            
            int tileY = y % 2;
            int tileX = x % 2;
            
            int tileI = tileY * 2 + tileX;
            
            pos.x += mShiftX * CELL_D*scale;
            pos.y += mShiftY * CELL_D*scale;
            
            drawSprite( mGroundOverlaySprite[tileI], pos, scale );
            }
        }
    }



SceneCell *EditorScenePage::getCurrentCell() {
    return &( mCells[ mCurY ][ mCurX ] );
    }


SceneCell *EditorScenePage::getCurrentPersonCell() {
    return &( mPersonCells[ mCurY ][ mCurX ] );
    }


SceneCell *EditorScenePage::getCurrentFloorCell() {
    return &( mFloorCells[ mCurY ][ mCurX ] );
    }

SceneCell *EditorScenePage::getCell(int absX, int absY) {
    return &( mCells[ absY ][ absX ] );
    }
    
SceneCell *EditorScenePage::getFloorCell(int absX, int absY) {
    return &( mFloorCells[ absY ][ absX ] );
    }
    




std::vector<std::vector<GridPos>> touchedQueue;
std::vector<std::vector<SceneCell>> cellsQueue;
std::vector<std::vector<SceneCell>> floorCellsQueue;
std::vector<std::vector<SceneCell>> cellsAfterQueue;
std::vector<std::vector<SceneCell>> floorCellsAfterQueue;
int queueIndex = 0;
int queueSize = 0;

std::vector<GridPos> currentTouchedTiles;
std::vector<GridPos> currentAfterTouchedTiles;
std::vector<SceneCell> currentCells;
std::vector<SceneCell> currentFloorCells;
std::vector<SceneCell> currentAfterCells;
std::vector<SceneCell> currentAfterFloorCells;

void EditorScenePage::mark( int x, int y, int beforeOrAfter = 0 ) {
    
    if( beforeOrAfter == 0 ) {
        for( int i=0; i<currentTouchedTiles.size(); i++ ) {
            GridPos currentTile = currentTouchedTiles[i];
            if( currentTile.x == x && currentTile.y == y ) return;
            }
        } 
    else {
        for( int i=0; i<currentAfterTouchedTiles.size(); i++ ) {
            GridPos currentTile = currentAfterTouchedTiles[i];
            if( currentTile.x == x && currentTile.y == y ) return;
            }
        }
    
    GridPos tile = { x, y };
    
    if( beforeOrAfter == 0 ) {
        currentTouchedTiles.push_back( tile );
        currentCells.push_back( mCells[tile.y][tile.x] );
        currentFloorCells.push_back( mFloorCells[tile.y][tile.x] );
        }
    else {
        currentAfterTouchedTiles.push_back( tile );
        currentAfterCells.push_back( mCells[tile.y][tile.x] );
        currentAfterFloorCells.push_back( mFloorCells[tile.y][tile.x] );
        }
    }

void EditorScenePage::backup() {
    
    mapChanged = true;
    
    while (queueIndex < queueSize) {
        queuesPopBack();
        }
    
    touchedQueue.push_back( currentTouchedTiles );
    cellsQueue.push_back( currentCells );
    floorCellsQueue.push_back( currentFloorCells );
    cellsAfterQueue.push_back( currentAfterCells );
    floorCellsAfterQueue.push_back( currentAfterFloorCells );
    
    currentTouchedTiles.clear();
    currentAfterTouchedTiles.clear();
    currentCells.clear();
    currentFloorCells.clear();
    currentAfterCells.clear();
    currentAfterFloorCells.clear();

    queueSize = touchedQueue.size();
    
    if (queueSize > queueCapacity) {
        queuesPopFront();
        }
    
    queueIndex = queueSize;
    checkVisible();
    
    }

void EditorScenePage::undo() {
    
    if (queueSize == 0 || queueIndex == 0) return;
    
    queueIndex--;
    
    std::vector<GridPos> touchedTiles = touchedQueue[queueIndex];
    std::vector<SceneCell> cells = cellsQueue[queueIndex];
    std::vector<SceneCell> floors = floorCellsQueue[queueIndex];
    
    for( int i=0; i<touchedTiles.size(); i++ ) {
        GridPos tile = touchedTiles[i];
        mCells[tile.y][tile.x] = cells[i];
        mFloorCells[tile.y][tile.x] = floors[i];
        }
    
    checkVisible();
    }

void EditorScenePage::redo() {
    
    if (queueIndex >= queueSize) return;

    std::vector<GridPos> touchedTiles = touchedQueue[queueIndex];
    std::vector<SceneCell> cells = cellsAfterQueue[queueIndex];
    std::vector<SceneCell> floors = floorCellsAfterQueue[queueIndex];
    
    for( int i=0; i<touchedTiles.size(); i++ ) {
        GridPos tile = touchedTiles[i];
        mCells[tile.y][tile.x] = cells[i];
        mFloorCells[tile.y][tile.x] = floors[i];
        }
    
    queueIndex++;
    
    checkVisible();
    }

void EditorScenePage::queuesPopBack() {
    touchedQueue.pop_back();
    cellsQueue.pop_back();
    floorCellsQueue.pop_back();
    cellsAfterQueue.pop_back();
    floorCellsAfterQueue.pop_back();
    
    queueSize = touchedQueue.size();
    }

void EditorScenePage::queuesPopFront() {
    touchedQueue.erase(touchedQueue.begin());
    cellsQueue.erase(cellsQueue.begin());
    floorCellsQueue.erase(floorCellsQueue.begin());
    cellsAfterQueue.erase(cellsAfterQueue.begin());
    floorCellsAfterQueue.erase(floorCellsAfterQueue.begin());
    
    queueSize = touchedQueue.size();
    }

void EditorScenePage::flashTile( int x, int y ) {
    GridPos tile = { x, y };
    fadingTiles.push_back( tile );
    fadingTilesFade.push_back( 1.0 );
    }




void EditorScenePage::checkVisible() {
    mCursorFade = 1.0f;
    
    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();

    int curFocusX = mCurX;
    int curFocusY = mCurY;

    if( mLittleDheld ) {
        curFocusX += c->destCellXOffset;
        curFocusY += c->destCellYOffset;
        }
    else if( mBigDheld ) {
        curFocusX += p->destCellXOffset;
        curFocusY += p->destCellYOffset;
        }
    
    mShiftX = 0-curFocusX;
    mShiftY = curFocusY-0;
    
    // if( curFocusX >= 4 && curFocusX <= 7 ) {
        // mShiftX = 0;
        // }
    // else {
        // if( curFocusX < 4 ) {
            // mShiftX = 4 - curFocusX;
            // }
        // else {
            // mShiftX = 7 - curFocusX;
            // }
        // }
    
    // if( curFocusY >= 2 && curFocusY <= 4 ) {
        // mShiftY = 0;
        // }
    // else {
        // if( curFocusY < 2 ) {
            // mShiftY = curFocusY - 2;
            // }
        // else {
            // mShiftY = curFocusY - 4;
            // }
        // }

    // make all visible, then turn some off selectively
    // below
    
    mUndoButton.setVisible( true );
    mRedoButton.setVisible( true );
        
    mAnimEditorButton.setVisible( false );
    mSaveNewButton.setVisible( true );
    mClearSceneButton.setVisible( true );
        
    mSaveTestMapButton.setVisible( true );


    mReplaceButton.setVisible( mSceneID != -1 );
    mConfirmReplaceButton.setVisible( false );
    mDeleteButton.setVisible( mSceneID != -1 );
    mConfirmDeleteButton.setVisible( false );
        
    checkNextPrevVisible();
        

    mGroundPicker.setVisible( true );
    mObjectPicker.setVisible( true );
        
    mPersonAgeSlider.setVisible( true );
        
    mCellAnimRadioButtons.setVisible( true );
    mPersonAnimRadioButtons.setVisible( true );
        
    mCellAnimFreezeSlider.setVisible( true );
    mPersonAnimFreezeSlider.setVisible( true );
        

    mCellXOffsetSlider.setVisible( true );
    mCellYOffsetSlider.setVisible( true );

    mPersonXOffsetSlider.setVisible( true );
    mPersonYOffsetSlider.setVisible( true );
        
    mCellMoveDelayField.setVisible( true );
    mPersonMoveDelayField.setVisible( true );
    
    mPersonEmotField.setVisible( true );



    mCellMoveDelayField.setVisible( c->destCellXOffset != 0 ||
                                    c->destCellYOffset != 0 );
    mPersonMoveDelayField.setVisible( p->destCellXOffset != 0 ||
                                      p->destCellYOffset != 0 );
    
    if (queueSize == 0 || queueIndex == 0) mUndoButton.setVisible( false );
    if (queueIndex >= queueSize - 1) mRedoButton.setVisible( false );

    if( c->oID > 0 ) {
        mCellAnimRadioButtons.setVisible( false );
        
        for( int a=0; a<NUM_CELL_ANIM; a++ ) {
            if( cellAnimTypes[a] == c->anim ) {
                mCellAnimRadioButtons.setSelectedItem( a );
                break;
                }
            }
        mCellAnimFreezeSlider.setVisible( false );
        mCellAnimFreezeSlider.setValue( c->frozenAnimTime );

        ObjectRecord *cellO = getObject( c->oID );
        
        if( cellO->numUses > 1 ) {
            mCellSpriteVanishSlider.setVisible( false );
            
            mCellSpriteVanishSlider.setHighValue( cellO->numUses );
            mCellSpriteVanishSlider.setValue( c->numUsesRemaining );
            }
        else {
            mCellSpriteVanishSlider.setVisible( false );
            }

        char *varPos = strstr( cellO->description, "$" );
        if( varPos != NULL ) {
            int maxVar = 0;
            sscanf( &( varPos[1] ), "%d", &maxVar );
            
            if( maxVar > 0 ) {
                mCellSpriteVarSlider.setVisible( true );
                mCellSpriteVarSlider.setHighValue( maxVar );
                mCellSpriteVanishSlider.setValue( c->varNumber );
                }
            }
        else {
            mCellSpriteVanishSlider.setVisible( false );
            }


        mCellXOffsetSlider.setVisible( false );
        mCellYOffsetSlider.setVisible( false );
        
        mCellXOffsetSlider.setValue( c->xOffset );
        mCellYOffsetSlider.setValue( c->yOffset );
        }
    else {
        mCellAnimRadioButtons.setVisible( false );
        mCellAnimFreezeSlider.setVisible( false );
        
        mCellXOffsetSlider.setVisible( false );
        mCellYOffsetSlider.setVisible( false );
        }
    


    if( p->oID > 0 ) {
        mPersonAgeSlider.setVisible( true );
        mPersonAgeSlider.setValue( p->age );
        mPersonAnimRadioButtons.setVisible( true );
        
        for( int a=0; a<NUM_PERSON_ANIM; a++ ) {
            if( personAnimTypes[a] == p->anim ) {
                mPersonAnimRadioButtons.setSelectedItem( a );
                break;
                }
            }

        mPersonAnimFreezeSlider.setVisible( true );
        mPersonAnimFreezeSlider.setValue( p->frozenAnimTime );
        
        mPersonXOffsetSlider.setVisible( true );
        mPersonYOffsetSlider.setVisible( true );
        
        mPersonXOffsetSlider.setValue( p->xOffset );
        mPersonYOffsetSlider.setValue( p->yOffset );
        
        
        mPersonEmotField.setVisible( true );

        if( p->currentEmot != NULL ) {
            mPersonEmotField.setText( p->currentEmot->triggerWord );
            for( int i=0; i < p->extraEmot.size(); i++ ) {
                char *s = autoSprintf( 
                    ",%s", 
                    p->extraEmot.getElementDirect( i )->triggerWord );
                mPersonEmotField.insertString( s );
                delete [] s;
                }
            }
        else {
            mPersonEmotField.setText( "" );
            }
        }
    else {
        mPersonAgeSlider.setVisible( false );
        mPersonAnimRadioButtons.setVisible( false );
        mPersonAnimFreezeSlider.setVisible( false );
        
        mPersonXOffsetSlider.setVisible( false );
        mPersonYOffsetSlider.setVisible( false );

        
        mPersonEmotField.setVisible( false );
        }
    }




static void stepMovingCell( SceneCell *inC ) {
    SceneCell *c = inC;
    
    c->frameCount++;

    if( c->oID <= 0 ) {
        return;
        }

    if( c->destCellXOffset == 0 && c->destCellYOffset == 0 ) {
        return;
        }

    if( c->moveDelayTime > 0 ) {
        if( c->moveDelayTime * 60 / frameRateFactor > c->frameCount ) {
            return;
            }
        }
    

    ObjectRecord *cellO = getObject( c->oID );

    // 4 cells per sec
    // base speed
    double speed = 4;

    if( cellO->person ) {
        // what they are holding applies speed mod to them
        if( c->heldID > 0  ) {
            speed *= getObject( c->heldID )->speedMult;
            }
        }
    else {
        // object itself has speed mod
        speed *= cellO->speedMult;
        }
    
    double totalCellDist = sqrt( c->destCellXOffset * c->destCellXOffset +
                                 c->destCellYOffset * c->destCellYOffset );
    
    double totalSec = totalCellDist / speed;
    
    double totalFrames = totalSec * 60 / frameRateFactor;
    
    double fractionPerFrame = 1.0 / totalFrames;
    
    c->moveFractionDone += fractionPerFrame;

    char wrap = false;
    while( c->moveFractionDone > 1.0 ) {
        c->moveFractionDone -= 1.0;
        wrap = true;
        }
    
    if( wrap ) {
        c->moveStartTime = Time::getCurrentTime();
        c->frameCount = 0;
        }

    c->moveOffset.x = c->destCellXOffset * c->moveFractionDone * CELL_D*scale;
    c->moveOffset.y = -c->destCellYOffset * c->moveFractionDone * CELL_D*scale;
    }



static void restartCell( SceneCell *inC ) {
    SceneCell *c = inC;
    
    if( c->oID > 0 && 
        ( c->destCellXOffset != 0 ||
          c->destCellYOffset != 0 ) ) {
    
        c->moveFractionDone = 0;
        c->moveOffset.x = 0;
        c->moveOffset.y = 0;
        c->moveStartTime = Time::getCurrentTime();
        c->frameCount = 0;
        }
    }



void EditorScenePage::restartAllMoves() {
    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            SceneCell *c = &( mCells[y][x] );
            SceneCell *p = &( mPersonCells[y][x] );
            
            restartCell( c );
            restartCell( p );
            }
        }
    }



static void drawOutlineString( const char *inString, 
                               doublePair inPos, TextAlignment inAlign ) {
    
    setDrawColor( 0, 0, 0, 1 );
    
    doublePair offsets[8] = 
        { {-2, 0 }, {2, 0 },
          { 0, -2 }, {0, 2},
          {-2, -2 }, {-2, 2 },
          {2, -2 }, {2, 2 } };
    
    for( int i=0; i<8; i++ ){
        doublePair pos = add( inPos, offsets[i] );
        
        smallFont->drawString( inString, pos, inAlign );
        }

    setDrawColor( 1, 1, 1, 1 );
    
    smallFont->drawString( inString, inPos, inAlign );
    }



void EditorScenePage::drawUnderComponents( doublePair inViewCenter, 
                                           double inViewSize ) {
    
    mFrameCount ++;

    // step any moving cells
    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            SceneCell *c = &( mCells[y][x] );
            SceneCell *p = &( mPersonCells[y][x] );
            
            stepMovingCell( c );
            stepMovingCell( p );
            }
        }
    
    

    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            doublePair pos = cornerPos;
                
            pos.x += x * CELL_D*scale;
            pos.y -= y * CELL_D*scale;

            if( y > mCurY + yLimit || 
                y < mCurY -yLimit ||
                x > mCurX + xLimit || 
                x < mCurX -xLimit ) {
                
                continue;
                }
            

            pos.x += mShiftX * CELL_D*scale;
            pos.y += mShiftY * CELL_D*scale;

            pos.x += 32*scale;
            pos.y -= 32*scale;
            
            SceneCell *c = &( mCells[y][x] );
            
            if( c->biome != -1 ) {
                
                
                GroundSpriteSet *s = groundSprites[ c->biome ];
                
                if( s  != NULL ) {

                    int tileX = x % s->numTilesWide;
                    int tileY = y % s->numTilesWide;
                    
                    setDrawColor( 1, 1, 1, 1 );
                    drawSprite( s->tiles[tileY][tileX], pos, scale );
                    }
                }

            }
        }

    
    double frameTime = frameRateFactor * mFrameCount / 60.0;


    double hugR = CELL_D*scale * 0.6;

    // floors on top of ground
    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            doublePair pos = cornerPos;
                
            pos.x += x * CELL_D*scale;
            pos.y -= y * CELL_D*scale;

            if( y > mCurY + yLimit || 
                y < mCurY -yLimit ||
                x > mCurX + xLimit || 
                x < mCurX -xLimit ) {
                
                continue;
                }
            

            pos.x += mShiftX * CELL_D*scale;
            pos.y += mShiftY * CELL_D*scale;


            SceneCell *f = &( mFloorCells[y][x] );


            // for main floor, and left and right hugging floor
            // 0 to skip a pass
            int passIDs[3] = { 0, 0, 0 };
            
            if( f->oID > 0 ) {
                passIDs[0] = f->oID;
                }
            


            if( f->oID <= 0 ) {
                
                char drawHuggingFloor = false;
                
                int cellOID = mCells[y][x].oID;
                
                if( cellOID > 0 && getObject( cellOID )->floorHugging ) {
                    
                    if( x > 0 && mFloorCells[y][ x - 1 ].oID > 0 && !getObject( mFloorCells[y][ x - 1 ].oID )->noCover ) {
                        // floor to our left
                        passIDs[1] = mFloorCells[y][ x - 1 ].oID;
                        drawHuggingFloor = true;
                        }
                    
                    if( x < mSceneW - 1 && mFloorCells[y][ x + 1 ].oID > 0 && !getObject( mFloorCells[y][ x + 1 ].oID )->noCover ) {
                        // floor to our right
                        passIDs[2] = mFloorCells[y][ x + 1 ].oID;
                        drawHuggingFloor = true;
                        }
                    }
                

                if( ! drawHuggingFloor ) {
                    continue;
                    }
                }



            
            for( int p=0; p<3; p++ ) {
                if( passIDs[p] == 0 ) {
                    continue;
                    }
                
                int oID = passIDs[p];
            
                if( p > 0 ) {
                    setDrawColor( 1, 1, 1, 1 );
                    startAddingToStencil( false, true );
                    }
                
                if( p == 1 ) {    
                    drawRect( pos.x - hugR, pos.y + hugR, 
                              pos.x, pos.y - hugR );
                    }
                else if( p == 2 ) {
                        
                    drawRect( pos.x, pos.y + hugR, 
                              pos.x + hugR, pos.y - hugR );
                    }

                if( p > 0 ) {
                    startDrawingThroughStencil();
                    }
                
                if( p > 0 ) {
                    // floor hugging pass

                    int numLayers = getObject( oID )->numSprites;
                    
                    if( numLayers > 1 ) {    
                        // draw all but top layer of floor
                        setAnimLayerCutoff( numLayers - 1 );
                        }
                    }

                char used;
                
                setDrawnObjectScale(scale);
                drawObjectAnim( oID, 2, ground, 
                                frameTime, 
                                0,
                                ground,
                                frameTime,
                                frameTime,
                                &used,
                                ground,
                                ground,
                                pos,
                                0,
                                false,
                                false,
                                -1,
                                false,
                                false,
                                false,
                                getEmptyClothingSet(),
                                NULL );
                if( p > 0 ) {
                    stopStencil();
                    }
                }
            
            if( passIDs[1] != passIDs[2] ) {
                setDrawColor( 1, 1, 1, 1 );
                pos.y += 10;
                drawSprite( mFloorSplitSprite, pos, scale );
                }
            }
        }



    toggleMultiplicativeBlend( true );
    
    // use this to lighten ground overlay
    toggleAdditiveTextureColoring( true );
    setDrawColor( multAmount, multAmount, multAmount, 1 );
    
    drawGroundOverlaySprites();

    toggleAdditiveTextureColoring( false );
    toggleMultiplicativeBlend( false );


    toggleAdditiveBlend( true );
    
    // use this to lighten ground overlay
    //toggleAdditiveTextureColoring( true );
    setDrawColor( 1, 1, 1, addAmount );

    drawGroundOverlaySprites();

    
    toggleAdditiveBlend( false );

    if( mShowWhite ) {
        setDrawColor( 1, 1, 1, 1 );
        
        doublePair squarePos = cornerPos;
        squarePos.x += 640;
        squarePos.y -= 360;
        
        drawRect( squarePos, 700, 400 );
        }
    
    

    for( int y=0; y<mSceneH; y++ ) {
        
        if( y > mCurY + yLimit || 
            y < mCurY -yLimit ) {
                
            continue;
            }


        // draw behind stuff first, b=0
        // then people, b=1, with permanent objects in front
        // then non-permanent objects, b=2
        // then non-container walls (floor hugging, no slots), b=3
        // then container walls (floor hugging, some slots), b=4
        for( int b=0; b<5; b++ ) {
            

            if( b == 1 ) {
                // draw people behind objects in this row

                for( int x=0; x<mSceneW; x++ ) {
                    
                    if( x > mCurX + xLimit || 
                        x < mCurX -xLimit ) {
                        
                        continue;
                        }


                    doublePair pos = cornerPos;
                
                    pos.x += x * CELL_D*scale;
                    pos.y -= y * CELL_D*scale;

                    pos.x += mShiftX * CELL_D*scale;
                    pos.y += mShiftY * CELL_D*scale;

                    SceneCell *p = &( mPersonCells[y][x] );

                    if( p->oID > 0 ) {
                        
                        doublePair personPos = pos;
                        
                        personPos.x += p->xOffset;
                        personPos.y += p->yOffset;
                        
                        personPos = add( personPos, p->moveOffset );
                        
                        int hideClosestArm = 0;
                        char hideAllLimbs = false;
                        
                        ObjectRecord *heldObject = NULL;
                        
                        AnimType frozenArmAnimType = endAnimType;
                        
                        if( p->heldID != -1 ) {
                            heldObject = getObject( p->heldID );
                            }
                

                        char splitHeld = false;
                        
                        if( heldObject != NULL &&
                            heldObject->rideable &&
                            heldObject->anySpritesBehindPlayer ) {
                            splitHeld = true;
                            }

                        getArmHoldingParameters( heldObject, 
                                                 &hideClosestArm, 
                                                 &hideAllLimbs );
                        

                        if( ( heldObject != NULL &&
                              heldObject->rideable ) ||
                            hideClosestArm == -2 ) {
                            frozenArmAnimType = moving;
                            }
                        
                        double thisFrameTime = p->frozenAnimTime;
                        
                        if( thisFrameTime < 0 ) {
                            thisFrameTime = frameTime + fabs( thisFrameTime );
                            }
                        
                        
                        if( heldObject != NULL ) {
                            thisFrameTime *= heldObject->speedMult;
                            }
                        
                        double frozenRotFrameTime = thisFrameTime;
                        
                        if( p->anim != moving ) {
                            frozenRotFrameTime = 0;
                            }

                        char used;
                    
                        HoldingPos holdingPos;
                        
                        ObjectRecord *obj = getObject( p->oID );
        
                        for( int i=0; i<obj->numSprites; i++ ) {
                            markSpriteLive( obj->sprites[i] );
                            }

                        if( p->graveID != -1 ) {
                            obj = getObject( p->graveID );
        
                            for( int i=0; i<obj->numSprites; i++ ) {
                                markSpriteLive( obj->sprites[i] );
                                }
                            }
                        


                        if( p->age >= 60 &&
                            p->graveID != -1 ) {
                            
                            setDrawnObjectScale(scale);
                            holdingPos = 
                            drawObjectAnim( p->graveID, 2, ground, 
                                            thisFrameTime, 
                                            0,
                                            ground,
                                            thisFrameTime,
                                            frozenRotFrameTime,
                                            &used,
                                            frozenArmAnimType,
                                            frozenArmAnimType,
                                            personPos,
                                            0,
                                            false,
                                            p->flipH,
                                            p->age,
                                            hideClosestArm,
                                            hideAllLimbs,
                                            false,
                                            p->clothing,
                                            NULL );
                            }
                        else {
                            
                            setAnimationEmotion( p->currentEmot );
                            addExtraAnimationEmotions( &( p->extraEmot ) );
                            
                            ClothingSet clothingToDraw = p->clothing;
                            
                            if( splitHeld ) {
                                // don't actually draw person now
                                // sandwitch them in between layers of 
                                // held later
                                prepareToSkipSprites( getObject( p->oID ),
                                                      false, true );
                                clothingToDraw = getEmptyClothingSet();
                                }

                            setDrawnObjectScale(scale);
                            holdingPos =
                            drawObjectAnim( p->oID, 2, p->anim, 
                                            thisFrameTime, 
                                            0,
                                            p->anim,
                                            thisFrameTime,
                                            frozenRotFrameTime,
                                            &used,
                                            frozenArmAnimType,
                                            frozenArmAnimType,
                                            personPos,
                                            0,
                                            false,
                                            p->flipH,
                                            p->age,
                                            hideClosestArm,
                                            hideAllLimbs,
                                            false,
                                            clothingToDraw,
                                            NULL );
                            if( splitHeld ) {
                                restoreSkipDrawing( getObject( p->oID ) );
                                }

                            setAnimationEmotion( NULL );
                            }
                    
                        if( heldObject != NULL ) {
                            
                            doublePair holdPos;
                            double holdRot;                            
                        
                            computeHeldDrawPos( holdingPos, personPos,
                                                heldObject,
                                                p->flipH,
                                                &holdPos, &holdRot );
                            
                            if( heldObject->person ) {
                                // baby doesn't rotate when held
                                holdRot = 0;
                                }

                            double heldAge = -1;
                            AnimType heldAnimType = p->anim;
                            AnimType heldFadeTargetType = p->anim;
                    
                            ClothingSet heldClothing = getEmptyClothingSet();
                            
                            double heldFrozenRotFrameTime = thisFrameTime;
                            
                            if( p->anim != moving ) {
                                heldAnimType = held;
                                heldFadeTargetType = held;
                                heldFrozenRotFrameTime = 0;
                                }

                            if( heldObject->person ) {
                                heldAge = p->heldAge;
                                heldClothing = p->heldClothing;
                                heldAnimType = held;
                                heldFadeTargetType = held;
                                heldFrozenRotFrameTime = 0;
                                }
                            
                            int *contained = p->contained.getElementArray();
                            SimpleVector<int> *subContained = 
                                p->subContained.getElementArray();

                            setAnimationEmotion( p->heldEmotion );
                            addExtraAnimationEmotions( 
                                &( p->heldExtraEmotion ) );
                            
                            if( splitHeld ) {
                                setDrawnObjectScale(scale);
                                // draw behind part
                                prepareToSkipSprites( getObject( p->heldID ), 
                                                      true );
                                drawObjectAnim( p->heldID,  
                                                heldAnimType, thisFrameTime,
                                                0, 
                                                heldFadeTargetType, 
                                                thisFrameTime, 
                                                heldFrozenRotFrameTime,
                                                &used,
                                                moving,
                                                moving,
                                                holdPos, holdRot, 
                                                false, p->flipH, 
                                                heldAge,
                                                false,
                                                false,
                                                false,
                                                heldClothing,
                                                NULL,
                                                p->contained.size(), contained,
                                                subContained );
                                restoreSkipDrawing( getObject( p->heldID ) );

                                // now draw player in between
                                drawObjectAnim( p->oID, 2, p->anim, 
                                                thisFrameTime, 
                                                0,
                                                p->anim,
                                                thisFrameTime,
                                                frozenRotFrameTime,
                                                &used,
                                                frozenArmAnimType,
                                                frozenArmAnimType,
                                                personPos,
                                                0,
                                                false,
                                                p->flipH,
                                                p->age,
                                                hideClosestArm,
                                                hideAllLimbs,
                                                false,
                                                p->clothing,
                                                NULL );

                                // draw front part of held
                                prepareToSkipSprites( getObject( p->heldID ), 
                                                      false );
                                }
                            

                            drawObjectAnim( p->heldID,  
                                            heldAnimType, thisFrameTime,
                                            0, 
                                            heldFadeTargetType, 
                                            thisFrameTime, 
                                            heldFrozenRotFrameTime,
                                            &used,
                                            moving,
                                            moving,
                                            holdPos, holdRot, 
                                            false, p->flipH, 
                                            heldAge,
                                            false,
                                            false,
                                            false,
                                            heldClothing,
                                            NULL,
                                            p->contained.size(), contained,
                                            subContained );
                            
                            if( splitHeld ) {
                                restoreSkipDrawing( getObject( p->heldID ) );
                                }

                            setAnimationEmotion( NULL );
                            delete [] contained;
                            delete [] subContained;
                            }
                        }
                    }
                }
            
            // now objects in row
            for( int x=0; x<mSceneW; x++ ) {
                if( x > mCurX + xLimit || 
                    x < mCurX -xLimit ) {
                    
                    continue;
                    }

                doublePair pos = cornerPos;
                
                pos.x += x * CELL_D*scale;
                pos.y -= y * CELL_D*scale;
                
                pos.x += mShiftX * CELL_D*scale;
                pos.y += mShiftY * CELL_D*scale;

                SceneCell *c = &( mCells[y][x] );
                
                if( c->oID > 0 ) {
                    
                    ObjectRecord *o = getObject( c->oID );
                    
                    if( ( b == 0 && ! ( o->drawBehindPlayer || 
                                        o->anySpritesBehindPlayer ) )
                        ||
                        ( b != 0 && o->drawBehindPlayer ) ) {
                        continue;
                        }
                    if( ( b == 3 && 
                          ! ( o->wallLayer && ! o->frontWall )  ) 
                        ||
                        ( b != 3 && o->wallLayer && ! o->frontWall 
                          && ! ( o->drawBehindPlayer || 
                                 o->anySpritesBehindPlayer ) ) ) {
                        continue;
                        }

                    
                    if( b == 4 &&
                        ! ( o->wallLayer && o->frontWall ) ) {
                        continue;
                        }


                    if( ( b == 1 && ! o->permanent ) ||
                        ( b == 2 && o->permanent ) ) {
                        continue;
                        }
                    

                    
                    doublePair cellPos = pos;
                    
                    cellPos.x += c->xOffset;
                    cellPos.y += c->yOffset;

                    cellPos = add( cellPos, c->moveOffset );

                    
                    double thisFrameTime = c->frozenAnimTime;
                        
                    if( thisFrameTime < 0 ) {
                        thisFrameTime = frameTime + fabs( thisFrameTime );
                        }

                
                    char used;
                    int *contained = c->contained.getElementArray();
                    SimpleVector<int> *subContained = 
                        c->subContained.getElementArray();


                    ObjectRecord *cellO = getObject( c->oID );
                    
                    // temporarily set up sprite vanish
                    if( cellO->numUses > 1 ) {
                        setupSpriteUseVis( cellO, c->numUsesRemaining,
                                           cellO->spriteSkipDrawing );
                        }
                    
                    // if( c->varNumber > 0 ) {
                    //     setupNumericSprites( 
                    //         cellO, c->varNumber,
                    //         mCellSpriteVarSlider.getHighValue(),
                    //         cellO->spriteSkipDrawing );
                    //     }

                    double frozenRotFrameTime = 0;
                    
                    if( c->anim == moving ) {
                        frozenRotFrameTime = thisFrameTime;
                        }
                    

                    char skippingSome = false;
                    if( b == 0 && cellO->anySpritesBehindPlayer ) {
                        prepareToSkipSprites( cellO, true );
                        skippingSome = true;
                        }
                    else if( b != 0 && cellO->anySpritesBehindPlayer ) {
                        prepareToSkipSprites( cellO, false );
                        skippingSome = true;
                        }

                    setDrawnObjectScale(scale);

                    // draw the object without animation
                    // animation creates too much lag
                    // also, contained position is incorrect with zoom in the drawObjectAnim
                    drawObject( getObject(c->oID), cellPos, 0, 
                                false, c->flipH, -1,
                                0,
                                false,
                                false,
                                c->clothing,
                                c->contained.size(), contained,
                                subContained
                                );
                    // drawObjectAnim( c->oID, c->anim, 
                    //                 thisFrameTime, 
                    //                 0,
                    //                 c->anim,
                    //                 thisFrameTime,
                    //                 frozenRotFrameTime,
                    //                 &used,
                    //                 ground,
                    //                 ground,
                    //                 cellPos,
                    //                 0,
                    //                 false,
                    //                 c->flipH,
                    //                 -1,
                    //                 0,
                    //                 false,
                    //                 false,
                    //                 c->clothing,
                    //                 NULL,
                    //                 c->contained.size(), contained,
                    //                 subContained );
                    delete [] contained;
                    delete [] subContained;

                    if( skippingSome ) {
                        restoreSkipDrawing( cellO );
                        }
                                        
                    // restore default sprite vanish
                    if( cellO->numUses > 1 ) {
                        setupSpriteUseVis( cellO, cellO->numUses,
                                           cellO->spriteSkipDrawing );
                        }

                    }
                }
            }
        }
    
    
    doublePair curPos = cornerPos;
                
    curPos.x += cursorGridX * CELL_D*scale;
    curPos.y -= cursorGridY * CELL_D*scale;

    curPos.x += mShiftX * CELL_D*scale;
    curPos.y += mShiftY * CELL_D*scale;
    

    if( mShowUI ) {
        mCursorFade += 0.05;
        if( mCursorFade > 1 ) {
            mCursorFade = 1;
            }
        }
    else {
        mCursorFade -= 0.05;
        if( mCursorFade < 0 ) {
            mCursorFade = 0;
            }
        }
    


    // draw a frame around the current cell
    startAddingToStencil( false, true );
    
    setDrawColor( 1, 1, 1, 1 );
    
    drawSquare( curPos, rectDim ); //62

    startDrawingThroughStencil( true );
    
    setDrawColor( 1, 1, 1, 0.75 * mCursorFade );
    drawSquare( curPos, rectDim+1 ); //64

    stopStencil();
        
        
    doublePair cursorInfoPos = {cursorX + 35.0, cursorY - 25.0};
    char *cursorInfo;

    int x = cursorGridX - mZeroX;
    int y = mZeroY - cursorGridY;
    
    cursorInfo = autoSprintf( "%d, %d", x, y );
    
    double w2 = smallFont->measureString( cursorInfo );
    double h2 = smallFont->getFontHeight();
    int padding = 4;
    w2 += 2 * padding;
    h2 += 2 * padding;
    
    setDrawColor( 0, 0, 0, 0.5 );
    doublePair rectPos = { cursorInfoPos.x + w2/2 - padding, cursorInfoPos.y };
    drawRect( rectPos, w2 / 2, h2 / 2 );
    setDrawColor( 1, 1, 1, 1 );
    smallFont->drawString( cursorInfo, cursorInfoPos, alignLeft );

    if( !mShowUI ) {
        return;
        }


    // draw + at origin
    doublePair zeroPos = cornerPos;
                
    zeroPos.x += mZeroX * CELL_D*scale;
    zeroPos.y -= mZeroY * CELL_D*scale;

    zeroPos.x += mShiftX * CELL_D*scale;
    zeroPos.y += mShiftY * CELL_D*scale;
    

    startAddingToStencil( false, true );
    
    setDrawColor( 1, 1, 1, 1 );
    
    
    
    // doublePair cornerPos = zeroPos;
    // cornerPos.x -= cursorDim+1;
    // cornerPos.y -= cursorDim+1;
    // drawSquare( cornerPos, cursorDim );

    // cornerPos = zeroPos;
    // cornerPos.x += cursorDim+1;
    // cornerPos.y -= cursorDim+1;
    // drawSquare( cornerPos, cursorDim );

    // cornerPos = zeroPos;
    // cornerPos.x -= cursorDim+1;
    // cornerPos.y += cursorDim+1;
    // drawSquare( cornerPos, cursorDim );

    // cornerPos = zeroPos;
    // cornerPos.x += cursorDim+1;
    // cornerPos.y += cursorDim+1;
    // drawSquare( cornerPos, cursorDim );

    startDrawingThroughStencil( true );
    
    for( int i=fadingTiles.size()-1; i>=0; i-- ) {
        GridPos tile = fadingTiles[i];
        float fade = fadingTilesFade[i];
        
        if( fade < 0 ) {
            fadingTiles.erase(fadingTiles.begin() + i);
            fadingTilesFade.erase(fadingTilesFade.begin() + i);
            continue;
            }
        else {
            fadingTilesFade[i] -= 0.05;
            fade = fadingTilesFade[i];
            }
        
        doublePair tilePos = { tile.x * CELL_D*scale, -tile.y * CELL_D*scale };
        tilePos.x += mShiftX * CELL_D*scale;
        tilePos.y += mShiftY * CELL_D*scale;
        
        setDrawColor( 1, 1, 1, 0.5 * fade );
        drawSquare( tilePos, rectDim );
        }
    
    // setDrawColor( 1, 0, 0, 0.75 );
    // drawSquare( zeroPos, cursorDim*2 );

    stopStencil();
    
    

    


    // SceneCell *c = getCurrentCell();
    SceneCell *c;
    if ( cursorGridX >= 0 && cursorGridY >= 0 && cursorGridX < mSceneW && cursorGridY < mSceneH ) {
        c = getCell(cursorGridX, cursorGridY);
        }
    else {
        c = &mEmptyCell;
        }
    SceneCell *p = getCurrentPersonCell();
    

    if( mLittleDheld || c->destCellXOffset != 0 || c->destCellYOffset != 0 ) {
        doublePair markPos = curPos;
        
        markPos.x += c->destCellXOffset * CELL_D*scale;
        markPos.y -= c->destCellYOffset * CELL_D*scale;
        
        drawSprite( mCellDestSprite, markPos );
        }

    if( mBigDheld || p->destCellXOffset != 0 || p->destCellYOffset != 0 ) {
        doublePair markPos = curPos;
        
        markPos.x += p->destCellXOffset * CELL_D*scale;
        markPos.y -= p->destCellYOffset * CELL_D*scale;
        
        drawSprite( mPersonDestSprite, markPos );
        }
    
    

    // doublePair legendPos = mAnimEditorButton.getPosition();
    // legendPos.x = -150;
    // legendPos.y += 20;
    
    doublePair legendPos = mObjectPicker.getPosition();
    legendPos.y -= 325;
    legendPos.x -= 80;
            
    drawKeyLegend( &mKeyLegend, legendPos );


    legendPos = mGroundPicker.getPosition();
    legendPos.y -= 255;
    drawKeyLegend( &mKeyLegendG, legendPos, alignCenter );


    legendPos = mObjectPicker.getPosition();
    legendPos.y -= 255;

    
    if( c->oID > 0 &&
        getObject( c->oID )->numSlots > c->contained.size() ) {
        
        drawKeyLegend( &mKeyLegendC, legendPos, alignCenter );
        }
    else if( p->oID > 0 ) {
        drawKeyLegend( &mKeyLegendP, legendPos, alignCenter );
        }
    else {
        drawKeyLegend( &mKeyLegendF, legendPos, alignCenter );
        }


    
    int relX = cursorGridX - mZeroX;
    int relY = mZeroY - cursorGridY;
    
    
    
    char *posStringX = autoSprintf( "%d,", relX );
    char *posStringY = autoSprintf( " %d", relY );
    
    double w = smallFont->measureString( posStringX ) + 
        smallFont->measureString( posStringY );

    double h = smallFont->getFontHeight();
    
    w += 8;
    h += 8;
    
    if( w < 84 ) {
        w = 84;
        }
    
    

    // doublePair posStringPos = {-820, 400}; //mSaveNewButton.getPosition();
    doublePair posStringPos = mUndoButton.getPosition();
    posStringPos.y += 90;
    posStringPos.x += 15;
    

    setDrawColor( 0, 0, 0, 0.5 );
    
    drawRect( posStringPos, w / 2, h / 2 );
    
    setDrawColor( 1, 1, 1, 1 );

    smallFont->drawString( posStringX, posStringPos, alignRight );
    smallFont->drawString( posStringY, posStringPos, alignLeft );

    delete [] posStringX;
    delete [] posStringY;

    if( pickedOID > 0 ) {
        // doublePair pos = { -400, -290 };
        doublePair pos = posStringPos;
        pos.y -= 25;
        pos.x -= 40;
        
        char *s = autoSprintf( "Picked: #%d  %s", pickedOID,
                               getObject( pickedOID )->description );
        

        drawOutlineString( s, pos, alignLeft );
        delete [] s;
        }
    if( c->oID > 0 ) {
        // doublePair pos = { -400, -290 };
        doublePair pos = posStringPos;
        pos.y -= 50;
        pos.x -= 40;
        
        char *s = autoSprintf( "Drawn: #%d  %s", c->oID,
                               getObject( c->oID )->description );
        

        drawOutlineString( s, pos, alignLeft );
        delete [] s;
        }

    if( p->oID > 0 ) {
        
        doublePair pos = { 450, -230 };
        
        char *s = autoSprintf( "oID=%d", p->oID );
        

        drawOutlineString( s, pos, alignLeft );

        delete [] s;
        
        if( p->heldID > 0 ) {
            pos.y -= 20;
            
            s = autoSprintf( "heldID=%d", p->heldID );

            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.hat != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "hat=%d", p->clothing.hat->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.tunic != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "tunic=%d", p->clothing.tunic->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.bottom != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "bottom=%d", p->clothing.bottom->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.frontShoe != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "frontShoe=%d", p->clothing.frontShoe->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.backShoe != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "backShoe=%d", p->clothing.backShoe->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        if( p->clothing.backpack != NULL ) {
            pos.y -= 20;
            
            s = autoSprintf( "backpack=%d", p->clothing.backpack->id );
            
            drawOutlineString( s, pos, alignLeft );
            delete [] s;
            }
        
        }
    
    // if( mReplaceButton.isVisible() ) {
        
        doublePair pos = mReplaceButton.getPosition();
        
        pos.y += 32;
        // pos.x -= 40;
        
        if (mSceneID == -1) {
            char *s = autoSprintf( "Unsaved Scene*" );
            drawOutlineString( s, pos, alignCenter );
            delete [] s;
            }
        else {
            if (mapChanged) { // Output the current scene filename to the screen.
                File *f = getSceneFile( mSceneID );
                char *n = f->getFileName();
                char *s = autoSprintf( "Scene %s*", n );
                drawOutlineString( s, pos, alignCenter );
                delete [] n;
                delete f;
                delete [] s;
                }
            else {
                File *f = getSceneFile( mSceneID );
                char *n = f->getFileName();
                char *s = autoSprintf( "Scene %s", n );
                drawOutlineString( s, pos, alignCenter );
                delete [] n;
                delete f;
                delete [] s;
                }
            }
        
        
        // }
    

    }



void EditorScenePage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }
    
    int grave = getRandomDeathMarker();

    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            SceneCell *p = &( mPersonCells[y][x] );
            p->graveID = grave;
            }
        }
    mEmptyCell.graveID = grave;
    
    TextField::unfocusAll();
    
    mGroundPicker.redoSearch( false );
    mObjectPicker.redoSearch( false );

    mLittleDheld = false;
    mBigDheld = false;
    }


void EditorScenePage::step() {

    if( mPlayingTime ) {
        
        for( int y=0; y<mSceneH; y++ ) {
            for( int x=0; x<mSceneW; x++ ) {
                SceneCell *c = &( mPersonCells[y][x] );
                
                if( c->age != -1 ) {
                    double rate = 0.05;

                    double old = c->age;
                    if( old > 20 && old < 40 ) {
                        rate *= 2;
                        }
                    
                    double newAge = old + rate * frameRateFactor;
        
                    if( newAge > 60 ) {
                        newAge = 60;
                        }
                    c->age = newAge;
                    }
                if( c->heldAge != -1 ) {
                    double rate = 0.05;

                    double old = c->heldAge;
                    if( old > 20 && old < 40 ) {
                        rate *= 2;
                        }
                    
                    double newAge = old + rate * frameRateFactor;
        
                    if( newAge > 60 ) {
                        newAge = 60;
                        }
                    c->heldAge = newAge;
                    }
                }
            }
        }
    }

// bool shiftDown = false;
// bool ctrlDown = false;
extern char upKey;
extern char leftKey;
extern char downKey;
extern char rightKey;

void EditorScenePage::keyDown( unsigned char inASCII ) {
    
    // if (isShiftKeyDown()) shiftDown = true;
    // if (isCommandKeyDown()) ctrlDown = true;
    
    char skipCheckVisible = false;
    
    if( inASCII == 13 ) {
        // enter
        // return to cursor control
        TextField::unfocusAll();
        }
    
    bool commandKey = isCommandKeyDown();
    if ( mShowUI ) {
        if( !commandKey && inASCII == 9 ) { // TAB
            if( TextField::isAnyFocused() ) {
                TextField::unfocusAll();
                }
            else {
                mObjectPicker.clearSearchField();
                mObjectPicker.focusSearchField();
                }
            return;
            }
        else if( commandKey && inASCII == 9 ) { // ctrl + TAB
            mObjectPicker.setSearchField( "." );
            TextField::unfocusAll();
            return;
            }
        else if( !TextField::isAnyFocused() && commandKey ) {
            if( inASCII + 64 == toupper(upKey) ) {
                mObjectPicker.selectUp();
                char oWasRightClick = false;
                pickedOID = mObjectPicker.getSelectedObject( &oWasRightClick );
                pickedGID = -1;
                }
            else if( inASCII + 64 == toupper(downKey) ) {
                mObjectPicker.selectDown();
                char oWasRightClick = false;
                pickedOID = mObjectPicker.getSelectedObject( &oWasRightClick );
                pickedGID = -1;
                }
            else if( inASCII + 64 == toupper(rightKey) ) {
                mObjectPicker.nextPage();
                }
            else if( inASCII + 64 == toupper(leftKey) ) {
                mObjectPicker.prevPage();
                }
            }
        }

    if( TextField::isAnyFocused() ) {
        mShowUI = true;
        return;
        }
    
    if ( tolower(inASCII) == 'e' ) {
        mShowUI = ! mShowUI;
        skipDrawingSubComponents( ! mShowUI );
        }
    if( tolower(inASCII) == 'z' ) {
        if (mShowUI) undo();
        }
        
    if( tolower(inASCII) == 'x' ) {
        if (mShowUI) redo();
        }
    
    if( inASCII == '=' ) {
        // screen shot
        // don't checkVisible, because it makes cur cell border appear
        return;
        }
        
    if( cursorGridX < 0 ||
        cursorGridY < 0 ||
        cursorGridX >= mSceneW ||
        cursorGridY >= mSceneH )
        return;

    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();
    SceneCell *f = getCurrentFloorCell();
    
    int offset = 1;
    
    if( isShiftKeyDown() ) {
        offset = 4;
        }
    if( isCommandKeyDown() && isShiftKeyDown() ) {
        offset = 8;
        }
    
    switch( tolower(inASCII) ) {
        case 'a':
            mCurX -= offset;
            if( mCurX < 0 ) {
                mCurX = 0;
                }
            break;
        case 'd':
            mCurX += offset;
            if( mCurX >= mSceneW ) {
                mCurX = mSceneW - 1;
                }
            break;
        case 's':
            mCurY += offset;
            if( mCurY >= mSceneH ) {
                mCurY = mSceneH - 1;
                }
            break;
        case 'w':
            mCurY -= offset;
            if( mCurY < 0 ) {
                mCurY = 0;
                }
            break;
        }
    
    if ( tolower(inASCII) == 'f' ) {
        if ( mShowUI ) {
            int tempOID = -1;
            if ( !isShiftKeyDown() ) {
                tempOID = getCell(cursorGridX, cursorGridY)->oID;
                
                }
            else {
                tempOID = getFloorCell(cursorGridX, cursorGridY)->oID;
                }
            if (tempOID != -1) {
                mObjectPicker.unselectObject();
                mGroundPicker.unselectObject();
                pickedOID = tempOID;
                pickedGID = -1;
                
                flashTile( cursorGridX, cursorGridY );
                }
            }
        }
    if ( inASCII + 64 == toupper('f') ) {
        if ( mShowUI ) {
            int tempBiome = -1;
            tempBiome = getCell(cursorGridX, cursorGridY)->biome;
            if (tempBiome != -1) {
                mObjectPicker.unselectObject();
                mGroundPicker.unselectObject();
                pickedOID = 0;
                pickedGID = tempBiome;
                
                flashTile( cursorGridX, cursorGridY );
                }
            }
        }
    if ( tolower(inASCII) == 'c' ) {
        if ( mShowUI ) {
            if ( !isShiftKeyDown() ) {
                // copy
                mCopyBuffer = mCells[ cursorGridY ][ cursorGridX ];
                mCopyFloorBuffer = mFloorCells[ cursorGridY ][ cursorGridX ];
                copyAreaSet = false;
                
                flashTile( cursorGridX, cursorGridY );
                }
            else {
                if( cursorGridY + copyAreaSize <= mSceneH &&
                    cursorGridX + copyAreaSize <= mSceneW &&
                    cursorGridY >= 0 &&
                    cursorGridX >= 0
                    ) {
                    for( int y=cursorGridY; y< cursorGridY + copyAreaSize; y++ ) {
                        for( int x=cursorGridX; x< cursorGridX + copyAreaSize; x++ ) {
                            
                            flashTile( x, y );
                            
                            copyArea[ y - cursorGridY ][ x - cursorGridX ] = mCells[ y ][ x ];
                            copyFloorArea[ y - cursorGridY ][ x - cursorGridX ] = mFloorCells[ y ][ x ];
                            }
                        }
                    copyAreaSet = true;
                    }
                }
            }
        }
    if( tolower(inASCII) == 'v' ) {
        if ( mShowUI ) {
            // paste
            if( copyAreaSet && isShiftKeyDown() ) {
                if( cursorGridY + copyAreaSize <= mSceneH &&
                    cursorGridX + copyAreaSize <= mSceneW &&
                    cursorGridY >= 0 &&
                    cursorGridX >= 0
                    ) {
                    for( int y=cursorGridY; y< cursorGridY + copyAreaSize; y++ ) {
                        for( int x=cursorGridX; x< cursorGridX + copyAreaSize; x++ ) {
                            
                            flashTile( x, y );
                            
                            mark( x, y, 0 );
                            
                            copyArea[ y - cursorGridY ][ x - cursorGridX ].biome = mCells[ y ][ x ].biome;
                        
                            mCells[ y ][ x ] = copyArea[ y - cursorGridY ][ x - cursorGridX ];
                            mFloorCells[ y ][ x ] = copyFloorArea[ y - cursorGridY ][ x - cursorGridX ];
                            
                            mark( x, y, 1 );
                            
                            }
                        }
                    backup();
                    }
                }
            if ( !isShiftKeyDown() ) {
                if( mCopyBuffer.oID > 0 &&
                    getObject( mCopyBuffer.oID )->person ) {
                    // *p = mCopyBuffer;
                    }
                else {
                    mark( cursorGridX, cursorGridY, 0 );
                    // mCopyBuffer.biome = c->biome;
                    int oldBiome = mCells[ cursorGridY ][ cursorGridX ].biome;
                    mCells[ cursorGridY ][ cursorGridX ] = mCopyBuffer;
                    mCells[ cursorGridY ][ cursorGridX ].biome = oldBiome;
                    mFloorCells[ cursorGridY ][ cursorGridX ] = mCopyFloorBuffer;
                    mark( cursorGridX, cursorGridY, 1 );
                    backup();
                    
                    flashTile( cursorGridX, cursorGridY );
                    }
                }
            restartAllMoves();
            }
        }
    if ( tolower(inASCII) == 'q' ) {
        if ( mShowUI ) {
            if ( isShiftKeyDown() ) {
                if( cursorGridY + copyAreaSize <= mSceneH &&
                    cursorGridX + copyAreaSize <= mSceneW &&
                    cursorGridY >= 0 &&
                    cursorGridX >= 0
                    ) {
                    for( int y=cursorGridY; y< cursorGridY + copyAreaSize; y++ ) {
                        for( int x=cursorGridX; x< cursorGridX + copyAreaSize; x++ ) {
                            
                            flashTile( x, y );
                            
                            mark( x, y, 0 );
                            
                            clearCell( &(mCells[ y ][ x ]) );
                            clearCell( &(mFloorCells[ y ][ x ]) );
                            mCells[ y ][ x ].biome = -1;
                            
                            mark( x, y, 1 );
                            
                            }
                        }
                    backup();
                    }
                }
            else {
                mark( cursorGridX, cursorGridY, 0 );
                
                clearCell( &(mCells[ cursorGridY ][ cursorGridX ]) );
                clearCell( &(mFloorCells[ cursorGridY ][ cursorGridX ]) );
                mCells[ cursorGridY ][ cursorGridX ].biome = -1;
                
                mark( cursorGridX, cursorGridY, 1 );
                backup();
                }
            }
        }
    if( tolower(inASCII) == 'o' ) {
        if ( mShowUI ) {
            mZeroX = cursorGridX;
            mZeroY = cursorGridY;
            }
        }
        
    checkVisible();
        
    return;
    
    
    // if( inASCII == 'h' ) {
        // mShowUI = ! mShowUI;
        // skipDrawingSubComponents( ! mShowUI );
        // }
    if( inASCII == 'w' ) {
        mShowWhite = ! mShowWhite;
        }
    else if( inASCII == 't' ) {
        mFrameCount = 0;
        restartAllMoves();
        skipCheckVisible = true;
        }
    else if( inASCII == 'o' ) {
        mZeroX = mCurX;
        mZeroY = mCurY;
        }
    else if( inASCII == 'p' ) {
        skipCheckVisible = true;
        if( ! mPlayingTime ) {
            
            mPlayingTime = true;
            }
        else {
            mPlayingTime = false;
            for( int y=0; y<mSceneH; y++ ) {
                for( int x=0; x<mSceneW; x++ ) {
                    SceneCell *c = &( mPersonCells[y][x] );
                    c->age = c->returnAge;
                    c->heldAge = c->returnHeldAge;
                    }
                }
            }
        }
    else if( inASCII == 'r' ) {
        skipCheckVisible = true;
        if( ! mRecordingFrames ) {
            startOutputAllFrames();
            mRecordingFrames = true;
            }
        else {
            stopOutputAllFrames();
            mRecordingFrames = false;
            }
        }
    else if( inASCII == 'f' ) {
        c->flipH = ! c->flipH;
        }
    else if( inASCII == 'F' ) {
        p->flipH = ! p->flipH;
        }
    else if( inASCII == 'A' ) {
        for( int y=mCurY; y< mCurY + copyAreaSize; y++ ) {
            for( int x=mCurX; x< mCurX + copyAreaSize; x++ ) {
                
                copyArea[ y - mCurY ][ x - mCurX ] 
                    = mCells[ y ][ x ];
                copyFloorArea[ y - mCurY ][ x - mCurX ] 
                    = mFloorCells[ y ][ x ];
                copyPeopleArea[ y - mCurY ][ x - mCurX ] 
                    = mPersonCells[ y ][ x ];
                }
            }
        copyAreaSet = true;
        }
    else if( inASCII == 'c' ) {
        // copy
        mCopyBuffer = *c;
        copyAreaSet = false;
        }
    else if( inASCII == 'C' ) {
        // copy
        mCopyBuffer = *p;
        copyAreaSet = false;
        }
    else if( inASCII == 'x' ) {
        // cut
        // delete all but biome
        int oldBiome = c->biome;
        mCopyBuffer = *c;
        *c = mEmptyCell;
        c->biome = oldBiome;
        copyAreaSet = false;
        }
    else if( inASCII == 'X' ) {
        // cut person
        mCopyBuffer = *p;
        *p = mEmptyCell;
        copyAreaSet = false;
        }
    else if( inASCII == 'v' ) {
        // paste
        if( copyAreaSet ) {
            for( int y=mCurY; y< mCurY + copyAreaSize; y++ ) {
                for( int x=mCurX; x< mCurX + copyAreaSize; x++ ) {
                
                    copyArea[ y - mCurY ][ x - mCurX ].biome = mCells[ y ][ x ].biome;
                
                    mCells[ y ][ x ] = 
                        copyArea[ y - mCurY ][ x - mCurX ];
                    mFloorCells[ y ][ x ] = 
                        copyFloorArea[ y - mCurY ][ x - mCurX ];
                    mPersonCells[ y ][ x ] = 
                        copyPeopleArea[ y - mCurY ][ x - mCurX ];
                    }
                }
            }
        else if( mCopyBuffer.oID > 0 &&
            getObject( mCopyBuffer.oID )->person ) {
            *p = mCopyBuffer;
            }
        else {
            *c = mCopyBuffer;
            }
        restartAllMoves();
        }
    else if( inASCII == 'V' ) {
        if( mCopyBuffer.oID > 0 &&
            getObject( mCopyBuffer.oID )->person ) {
            // do nothing, don't paste people
            }
        else {
            for( int dy=-4; dy<4; dy++ ) {
                int y = mCurY + dy;
                if( y >= 0 && y < mSceneH ) {
                    for( int dx=-6; dx<6; dx++ ) {
                        int x = mCurX + dx;
                        if( x >= 0 && x < mSceneW ) {
                            mCells[ y ][ x ] = mCopyBuffer;
                            }
                        }
                    }
                }
            }
        }
    else if( inASCII == 'i' ) {
        // insert into container
        
        if( mCopyBuffer.oID > 0 &&
            c->oID > 0 &&
            getObject( c->oID )->numSlots > c->contained.size() ) {
            // room
            
            c->contained.push_back( mCopyBuffer.oID );
            SimpleVector<int> sub;            
            
            if( mCopyBuffer.contained.size() > 0 ) {
                
                int *pasteContained = mCopyBuffer.contained.getElementArray();
                
                sub.appendArray( pasteContained, mCopyBuffer.contained.size() );
                delete [] pasteContained;
                }
            
            c->subContained.push_back( sub );
            }
        }
    else if( inASCII == 'I' ) {
        // insert as held
        
        if( mCopyBuffer.oID > 0 && p->oID > 0 ) {
            
            p->heldID = mCopyBuffer.oID;
            
            p->contained = mCopyBuffer.contained;
            p->subContained = mCopyBuffer.subContained;
            
            if( getObject( p->heldID )->person ) {
                // add their clothing too
                p->heldClothing = mCopyBuffer.clothing;
                p->heldAge = mCopyBuffer.age;
                p->returnHeldAge = p->heldAge;
                p->heldEmotion = mCopyBuffer.currentEmot;
                p->heldExtraEmotion = mCopyBuffer.extraEmot;
                }
            }
        }
    else if( inASCII == 8 ) {
        // backspace
        clearCell( c );
        clearCell( p );
        clearCell( f );
        }
    else if( inASCII == 'd' ) {
        mLittleDheld = true;
        }
    else if( inASCII == 'D' ) {
        mBigDheld = true;
        }
    
    if( !skipCheckVisible ) {
        checkVisible();
        }
    }




void EditorScenePage::keyUp( unsigned char inASCII ) {
    
    // shiftDown = false;
    // ctrlDown = false;
    
    if( TextField::isAnyFocused() ) {
        return;
        }
    
    if( inASCII == '=' ) {
        // screen shot
        // don't checkVisible, because it makes cur cell border appear
        return;
        }


    if( inASCII == 'd' || inASCII == 'D' ) {
        // mLittleDheld = false;
        // mBigDheld = false;
            
        // checkVisible();
        }
    }




void EditorScenePage::clearCell( SceneCell *inCell ) {
    inCell->oID = -1;
    inCell->flipH = false;
    inCell->age = -1;
    inCell->heldID = -1;
    
    inCell->heldAge = -1;

    inCell->returnAge = -1;
    inCell->returnHeldAge = -1;

    inCell->clothing = getEmptyClothingSet();
    inCell->heldClothing = getEmptyClothingSet();
    
    inCell->heldEmotion = NULL;
    inCell->heldExtraEmotion.deleteAll();
    
    inCell->contained.deleteAll();
    inCell->subContained.deleteAll();    
    
    inCell->anim = ground;
    inCell->frozenAnimTime = -2;
    inCell->numUsesRemaining = 1;

    inCell->xOffset = 0;
    inCell->yOffset = 0;

    inCell->destCellXOffset = 0;
    inCell->destCellYOffset = 0;

    inCell->moveFractionDone = 0;

    inCell->moveOffset.x = 0;
    inCell->moveOffset.y = 0;

    inCell->moveDelayTime = 0;
    
    inCell->currentEmot = NULL;
    inCell->extraEmot.deleteAll();
    }



void EditorScenePage::clearScene() {    
    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            mCells[y][x] = mEmptyCell;
            mPersonCells[y][x] = mEmptyCell;
            mFloorCells[y][x] = mEmptyCell;
            }
        }
    mapChanged = false;
    }
    
bool EditorScenePage::hoverAnyUI( float inX, float inY ) {
    if (mUndoButton.isMouseOver()) return true;
    if (mRedoButton.isMouseOver()) return true;
    if (mSaveNewButton.isMouseOver()) return true;
    if (mReplaceButton.isMouseOver()) return true;
    if (mConfirmReplaceButton.isMouseOver()) return true;
    if (mDeleteButton.isMouseOver()) return true;
    if (mConfirmDeleteButton.isMouseOver()) return true;
    if (mSaveTestMapButton.isMouseOver()) return true;
    if (mNextSceneButton.isMouseOver()) return true;
    if (mPrevSceneButton.isMouseOver()) return true;
    
    doublePair gPos = mGroundPicker.getPosition();
    if ( abs(inX - gPos.x) < 100 && abs(inY - gPos.y) < 350 ) return true;
    doublePair oPos = mObjectPicker.getPosition();
    if ( abs(inX - oPos.x) < 100 && abs(inY - oPos.y) < 350 ) return true;
    
    return false;
    }

void EditorScenePage::pointerUp( float inX, float inY ) {
    
    if( currentTouchedTiles.size() > 0 ) backup();
    
    if ( mObjectPickerClicked ) {
        char oWasRightClick = false;
        pickedOID = mObjectPicker.getSelectedObject( &oWasRightClick );
        pickedGID = -1;
        mObjectPickerClicked = false;
        }
    if ( mGroundPickerClicked ) {
        char gWasRightClick = false;
        pickedGID = mGroundPicker.getSelectedObject( &gWasRightClick );
        pickedOID = 0;
        mGroundPickerClicked = false;
        
        }
    
    }

void EditorScenePage::pointerDown( float inX, float inY ) {

    int mouseButton = getLastMouseButton();
    if ( mouseButton == MouseButton::WHEELUP ) {
        float newScale = scale * 1.15f;
        if( newScale > 1.5f ) newScale = 1.5f;
        setScale( newScale );
        return;
        }
    else if ( mouseButton == MouseButton::WHEELDOWN ) {
        float newScale = scale / 1.15f;
        if( newScale < 0.01f ) newScale = 0.01f;
        setScale( newScale );
        return;
        }
    
    if ( mShowUI && hoverAnyUI(inX, inY) ) return;
    
    int x = round( inX / (CELL_D*scale) ) + mCurX;
    int y = round( - inY / (CELL_D*scale) ) + mCurY;
    
    if( ( !mShowUI && !isLastMouseButtonRight() ) ||
        ( mShowUI && !isLastMouseButtonRight() && isCommandKeyDown() )
    ) {
        mCurX = x;
        mCurY = y;
        
        if( mCurX < 0 ) {
            mCurX = 0;
            }
        if( mCurX >= mSceneW ) {
            mCurX = mSceneW - 1;
            }
        if( mCurY >= mSceneH ) {
            mCurY = mSceneH - 1;
            }
        if( mCurY < 0 ) {
            mCurY = 0;
            }
        
        checkVisible();
        
        return;
        }
    
    if ( mShowUI ) {
        
        if( x < 0 || x >= mSceneW || y >= mSceneH || y < 0 ) {
            checkVisible();
            return;
            }
        
        SceneCell *f = getFloorCell(x, y);
        SceneCell *c = getCell(x, y);
        
        if ( !isLastMouseButtonRight() ) {
            
            char oWasRightClick = false;
            char gWasRightClick = false;
            
            int oId = pickedOID;
            int gId = pickedGID;
            
            // int oId = mObjectPicker.getSelectedObject( &oWasRightClick );
            // int gId = mGroundPicker.getSelectedObject( &gWasRightClick );
            
            if( oId > 0 ) {
                char placed = false;
                ObjectRecord *o = getObject( oId );
                
                if( isShiftKeyDown() && c->oID > 0 && !o->floor ) {
                    if( getObject( c->oID )->numSlots > c->contained.size() ) {
                        mark( x, y, 0 );
                        c->contained.push_back( oId );
                        SimpleVector<int> sub;
                        c->subContained.push_back( sub );
                        placed = true;
                        mark( x, y, 1 );
                        }
                    }
                if( !placed && o->floor ) {
                    mark( x, y, 0 );
                    // place floor
                    f->oID = oId;
                    placed = true;
                    mark( x, y, 1 );
                    }
                
                if( !placed ) {
                    if( !getObject( oId )->person ) {
                        mark( x, y, 0 );
                        c->oID = oId;
                        c->contained.deleteAll();
                        c->subContained.deleteAll();
                        c->numUsesRemaining = o->numUses;
                        mark( x, y, 1 );
                        }
                    }
                
                mObjectPicker.usePickable( oId );
                
                }
            else if( gId >= 0 ) {
                
                if( gId >= 0 ) {
                    
                    if( isShiftKeyDown() ) {
                        
                        floodFill( x, y,
                                   c->biome,
                                   gId );
                                   
                        }
                    else {
                        // single cell
                        mark( x, y, 0 );
                        mCells[ y ][ x ].biome = gId;
                        mark( x, y, 1 );
                        }
                        
                    }               
                }
            }
        else {
            mark( x, y, 0 );
            
            bool clearObj = false;
            bool clearFloor = false;
            bool clearBiome = false;

            if( isCommandKeyDown() && isShiftKeyDown() ) {
                clearObj = true;
                clearFloor = true;
                clearBiome = true;
                }
            else if( isCommandKeyDown() ) {
                clearFloor = true;
                }
            else if( isShiftKeyDown() ) {
                clearObj = true;
                clearFloor = true;
                }
            else {
                clearObj = true;                
                }
            
            int oldBiome = c->biome;
            if( clearObj ) clearCell(c);
            if( clearFloor ) clearCell(f);
            if( !clearBiome ) c->biome = oldBiome;
            
            mark( x, y, 1 );
            }
            
        }
    
    checkVisible();
    
    }

float cursorX;
float cursorY;
int cursorGridX;
int cursorGridY;

void EditorScenePage::pointerMove( float inX, float inY ) {    

    int x = round( inX / (CELL_D*scale) ) + mCurX;
    int y = round( - inY / (CELL_D*scale) ) + mCurY;
    
    cursorX = inX;
    cursorY = inY;
    cursorGridX = x;
    cursorGridY = y;

    }



void EditorScenePage::pointerDrag( float inX, float inY ) {
    
    int x = round( inX / (CELL_D*scale) ) + mCurX;
    int y = round( - inY / (CELL_D*scale) ) + mCurY;
    
    cursorX = inX;
    cursorY = inY;
    cursorGridX = x;
    cursorGridY = y;
    
    if (!mShowUI) return;
    if ( hoverAnyUI(inX, inY) ) return;
    
    if( x < 0 || x >= mSceneW || y >= mSceneH || y < 0 ) return;
    
    SceneCell *f = getFloorCell(x, y);
    SceneCell *c = getCell(x, y);
   
    if ( !isLastMouseButtonRight() ) {
        
        if ( !isCommandKeyDown() ) {
        
            char oWasRightClick = false;
            char gWasRightClick = false;
            
            int oId = pickedOID;
            int gId = pickedGID;
            
            // int oId = mObjectPicker.getSelectedObject( &oWasRightClick );
            // int gId = mGroundPicker.getSelectedObject( &gWasRightClick );
            
            if( oId > 0 ) {
                char placed = false;
                ObjectRecord *o = getObject( oId );
                
                if( oWasRightClick && c->oID > 0 && !o->floor ) {
                    if( getObject( c->oID )->numSlots > c->contained.size() ) {
                        mark( x, y, 0 );
                        c->contained.push_back( oId );
                        SimpleVector<int> sub;
                        c->subContained.push_back( sub );
                        placed = true;
                        mark( x, y, 1 );
                        }
                    }
                if( !placed && o->floor ) {
                    mark( x, y, 0 );
                    // place floor
                    f->oID = oId;
                    placed = true;
                    mark( x, y, 1 );
                    }
                
                if( !placed ) {
                    if( !getObject( oId )->person ) {
                        mark( x, y, 0 );
                        c->oID = oId;
                        c->contained.deleteAll();
                        c->subContained.deleteAll();
                        c->numUsesRemaining = o->numUses;                    
                        mark( x, y, 1 );
                        }
                    }
                }
            else if( gId >= 0 ) {
                
                if( gId >= 0 ) {
                    
                    if( isShiftKeyDown() ) {
                        
                        floodFill( x, y,
                                   c->biome,
                                   gId );
                        }
                    else {
                        mark( x, y, 0 );
                        // single cell
                        mCells[ y ][ x ].biome = gId;
                        mark( x, y, 1 );
                        }
                    }
                }
            }
        
        }
    else {
        mark( x, y, 0 );
        
        bool clearObj = false;
        bool clearFloor = false;
        bool clearBiome = false;

        if( isCommandKeyDown() && isShiftKeyDown() ) {
            clearObj = true;
            clearFloor = true;
            clearBiome = true;
            }
        else if( isCommandKeyDown() ) {
            clearFloor = true;
            }
        else if( isShiftKeyDown() ) {
            clearObj = true;
            clearFloor = true;
            }
        else {
            clearObj = true;                
            }
        
        int oldBiome = c->biome;
        if( clearObj ) clearCell(c);
        if( clearFloor ) clearCell(f);
        if( !clearBiome ) c->biome = oldBiome;
        
        mark( x, y, 1 );
        }
    // checkVisible();
    
    }
    



void EditorScenePage::specialKeyDown( int inKeyCode ) {
    
    if( TextField::isAnyFocused() ) {
        return;
        }


    int offset = 1;
    
    if( isCommandKeyDown() ) {
        offset = 2;
        }
    if( ! mLittleDheld && ! mBigDheld ) {    
        if( isShiftKeyDown() ) {
            offset = 4;
            }
        if( isCommandKeyDown() && isShiftKeyDown() ) {
            offset = 8;
            }
        }
    
    
    if( ! mLittleDheld && ! mBigDheld ) {    
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurX -= offset;
                if( mCurX < 0 ) {
                    mCurX = 0;
                    }
                break;
            case MG_KEY_RIGHT:
                mCurX += offset;
                if( mCurX >= mSceneW ) {
                    mCurX = mSceneW - 1;
                    }
                break;
            case MG_KEY_DOWN:
                mCurY += offset;
                if( mCurY >= mSceneH ) {
                    mCurY = mSceneH - 1;
                    }
                break;
            case MG_KEY_UP:
                mCurY -= offset;
                if( mCurY < 0 ) {
                    mCurY = 0;
                    }
                break;
            }
        }
    else {
        SceneCell *c;
        
        if( mLittleDheld ) {
            c = getCurrentCell();
            }
        else {
            c = getCurrentPersonCell();
            }
        
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                c->destCellXOffset -= offset;
                restartAllMoves();
                break;
            case MG_KEY_RIGHT:
                c->destCellXOffset += offset;
                restartAllMoves();
                break;
            case MG_KEY_DOWN:
                c->destCellYOffset += offset;
                restartAllMoves();
                break;
            case MG_KEY_UP:
                c->destCellYOffset -= offset;
                restartAllMoves();
                break;
            }
        
        }
    

    checkVisible();
    }


std::vector<File*> EditorScenePage::getSceneFiles() {
    
    std::vector<File*> results;
    
    int numFiles = 0;
    File **SceneDirectoryList = mScenesFolder.getChildFiles( &numFiles );  

    for( int i=0; i<numFiles; i++ ) {
        if( strcmp( SceneDirectoryList[i]->getFileName(), "next.txt" ) != 0 ) {
            results.push_back( SceneDirectoryList[i] );
            }
        else {
            delete SceneDirectoryList[i];
            }
        }
    delete [] SceneDirectoryList;
    
    return results;
    
    }

int EditorScenePage::getSceneFileID( char *fileName ) {
    
    // We want a number that getSceneFile will return this file for.
    std::vector<File*>SceneDirectoryList = getSceneFiles();
    int numFiles = SceneDirectoryList.size();
    int ret = -1;

    for( int i = 0; i < numFiles && ret == -1; i++) {
        char *thisFileName = SceneDirectoryList[i]->getFileName();
        if ( strcmp(thisFileName, fileName) == 0 ) {
            ret = i; // Found it!
            }
        delete [] thisFileName;
        }

    for( int i = 0; i < numFiles; i++ ) {
        delete SceneDirectoryList[i];
        }

    return ret;
    
    }

File *EditorScenePage::getSceneFile( int inSceneID ) {
    
    // char *name = autoSprintf( "%d.txt", inSceneID );
    
    std::vector<File*>SceneDirectoryList = getSceneFiles();
    int numFiles = SceneDirectoryList.size();
    File *f = NULL; // get a sorted list of all files
    
    // inSceneID bounds should be checked elsewhere
    if( inSceneID < 0 ) {
        return NULL;
        }
    
    if ( inSceneID >= numFiles ) {
        
        do { // If we're not in the bounds of that list, we want a NEW file.
            char *name = autoSprintf( "Editor_%02d.txt", inSceneID );
            if( f != NULL ) {
                delete f;
                f = NULL;
                }

            f = mScenesFolder.getChildFile( name );
            inSceneID++;
            delete [] name;
            } while ( f->exists() ); // For real though, a new file.
        
        }
    else {
    
        char *name = SceneDirectoryList[inSceneID]->getFileName();

        f = mScenesFolder.getChildFile( name );
        delete [] name;
        
        }

    for( int i=0; i<numFiles; i++ ) {
        delete SceneDirectoryList[i];
        }

    return f;
    }




void addClothingSetLines( SimpleVector<char*> *inLines,
                          ClothingSet inSet ) {
    inLines->push_back( 
        autoSprintf( "hat=%d",
                     inSet.hat == NULL 
                     ? 0 
                     : inSet.hat->id ) );
    inLines->push_back( 
        autoSprintf( "tunic=%d",
                     inSet.tunic == NULL 
                     ? 0 
                     : inSet.tunic->id ) );
    
    inLines->push_back( 
        autoSprintf( "frontShoe=%d",
                     inSet.frontShoe == NULL 
                     ? 0 
                     : inSet.frontShoe->id ) );
        
    inLines->push_back( 
        autoSprintf( "backShoe=%d",
                     inSet.backShoe == NULL 
                     ? 0 
                     : inSet.backShoe->id ) );
    
    inLines->push_back( 
        autoSprintf( "bottom=%d",
                     inSet.bottom == NULL 
                     ? 0 
                     : inSet.bottom->id ) );
    
    inLines->push_back( 
        autoSprintf( "backpack=%d",
                     inSet.backpack == NULL 
                     ? 0 
                     : inSet.backpack->id ) );
    }



void addCellLines( SimpleVector<char*> *inLines, 
                   SceneCell *inCell, char isPersonCell ) {
    
    
    if( !isPersonCell ) {    
        inLines->push_back( autoSprintf( "biome=%d", inCell->biome ) );
        }
    
    if( inCell->oID == -1 ) {
        inLines->push_back( stringDuplicate( "empty" ) );
        return;
        }
    

    inLines->push_back( autoSprintf( "oID=%d", inCell->oID ) );
    inLines->push_back( autoSprintf( "heldID=%d", inCell->heldID ) );

    int numCont = inCell->contained.size();
    
    inLines->push_back( autoSprintf( "numCont=%d", numCont ) );
    
    for( int i=0; i<numCont; i++ ) {
        inLines->push_back( 
            autoSprintf( "cont=%d", 
                         inCell->contained.getElementDirect( i ) ) );
        
        int numSub = inCell->subContained.getElementDirect(i).size();
        
        inLines->push_back( autoSprintf( "numSubCont=%d", numSub ) );
        
        for( int s=0; s<numSub; s++ ) {
            inLines->push_back( 
                autoSprintf( "subCont=%d", 
                             inCell->subContained.getElementDirect( i ).
                             getElementDirect( s ) ) );
            }
        }
    
    
    addClothingSetLines( inLines, inCell->clothing );
    
    
    inLines->push_back( autoSprintf( "flipH=%d", (int)( inCell->flipH ) ) );
    inLines->push_back( autoSprintf( "age=%f", inCell->age ) );

    inLines->push_back( autoSprintf( "heldAge=%f", inCell->heldAge ) );

    addClothingSetLines( inLines, inCell->heldClothing );


    inLines->push_back( autoSprintf( "anim=%d", (int)( inCell->anim ) ) );
    
    inLines->push_back( autoSprintf( "frozenAnimTime=%f", 
                                     inCell->frozenAnimTime ) );
    
    inLines->push_back( autoSprintf( "numUsesRemaining=%d", 
                                     inCell->numUsesRemaining ) );
    
    inLines->push_back( autoSprintf( "xOffset=%d", inCell->xOffset ) );
    inLines->push_back( autoSprintf( "yOffset=%d", inCell->yOffset ) );

    inLines->push_back( autoSprintf( "destCellXOffset=%d", 
                                     inCell->destCellXOffset ) );
    
    inLines->push_back( autoSprintf( "destCellYOffset=%d", 
                                     inCell->destCellYOffset ) );
    
    inLines->push_back( autoSprintf( "moveDelayTime=%f", 
                                     inCell->moveDelayTime ) );
    }



void EditorScenePage::writeSceneToFile( int inIDToUse ) {
    File *f = getSceneFile( inIDToUse );

    SimpleVector<char*> lines;
        
    lines.push_back( autoSprintf( "w=%d", mSceneW ) );
    lines.push_back( autoSprintf( "h=%d", mSceneH ) );
    lines.push_back( autoSprintf( "origin=%d,%d", mZeroX, mZeroY ) );
    lines.push_back( stringDuplicate( "floorPresent" ) );
    
    for( int y=0; y<mSceneH; y++ ) {
        for( int x=0; x<mSceneW; x++ ) {
            SceneCell *c = &( mCells[y][x] );
            SceneCell *p = &( mPersonCells[y][x] );
            SceneCell *f = &( mFloorCells[y][x] );
            

            if( c->biome == -1 &&
                c->oID == -1 &&
                p->oID == -1 &&
                f->oID == -1 ) {
                // don't represent blank cells at all
                }
            else {
                lines.push_back( autoSprintf( "x=%d,y=%d", x, y ) );
                addCellLines( &lines, c, false );
                addCellLines( &lines, p, true );
                addCellLines( &lines, f, true );
                }
            }
        }
        
    lines.push_back( stringDuplicate("") );
    
        
    
    char **linesArray = lines.getElementArray();
    
    
    char *contents = join( linesArray, lines.size(), "\n" );
    
    delete [] linesArray;
    lines.deallocateStringElements();

    f->writeToFile( contents );
    if( inIDToUse == mNextSceneNumber ) {
        mNextSceneNumber++; // Just in case we've deleted it. It might change the ID when it's created.
        mNextFile->writeToFile( mNextSceneNumber );

        char *fileName = f->getFileName(); // Make sure we have the same file we just created loaded.
        mSceneID = getSceneFileID( fileName );
        delete [] fileName;
        }
    delete [] contents;

    delete f;
    }



void scanClothingLine( char *inLine, ObjectRecord **inSpot, 
                       const char *inFormat ) {
    int id = -1;
    sscanf( inLine, inFormat, &id );
    
    if( id <= 0 ) {
        *inSpot = NULL;
        }
    else {
        *inSpot = getObject( id );
        }
    }


int scanClothingSet( char **inLines, int inNextLine, ClothingSet *inSet ) {
    char **lines = inLines;
    int next = inNextLine;
    
    scanClothingLine( lines[next], &( inSet->hat ), "hat=%d" );
    next++;
    
    scanClothingLine( lines[next], &( inSet->tunic ), "tunic=%d" );
    next++;
    
    scanClothingLine( lines[next], &( inSet->frontShoe ), "frontShoe=%d" );
    next++;
    
    scanClothingLine( lines[next], &( inSet->backShoe ), "backShoe=%d" );
    next++;
    
    scanClothingLine( lines[next], &( inSet->bottom ), "bottom=%d" );
    next++;
    
    scanClothingLine( lines[next], &( inSet->backpack ), "backpack=%d" );
    next++;
    
    return next;
    }



// returns next line index after scan
int scanCell( char **inLines, int inNextLine, SceneCell *inCell ) {
    char **lines = inLines;
    int next = inNextLine;


    int numRead = sscanf( lines[next], "biome=%d", &( inCell->biome ) );
    
    if( numRead == 1 ) {
        // person cells don't have biome listed
        next++;
        }
    
    if( strcmp( lines[next], "empty" ) == 0 ) {
        next++;
        return next;
        }
    

    sscanf( lines[next], "oID=%d", &( inCell->oID ) );
    next++;

    sscanf( lines[next], "heldID=%d", &( inCell->heldID ) );
    next++;

    int numCont = 0;
    
    sscanf( lines[next], "numCont=%d", &numCont );
    next++;
    
    for( int i=0; i<numCont; i++ ) {
        int cont;
        
        sscanf( lines[next], "cont=%d", &cont );
        next++;
            
        inCell->contained.push_back( cont );
        
        int numSub;
        
        SimpleVector<int> subVec;
        
        sscanf( lines[next], "numSubCont=%d", &numSub );
        next++;
    
        for( int s=0; s<numSub; s++ ) {
            int subCont;
            sscanf( lines[next], "subCont=%d", &subCont );
            next++;
            
            subVec.push_back( subCont );
            }
        inCell->subContained.push_back( subVec );
        }
    
    
    next = scanClothingSet( inLines, next, &( inCell->clothing ) );
                            
                            
                         
    int flip;
    sscanf( inLines[next], "flipH=%d", &flip );
    next++;

    inCell->flipH = (char)flip;

    sscanf( inLines[next], "age=%lf", &( inCell->age ) );
    inCell->returnAge = inCell->age;
    next++;

    sscanf( inLines[next], "heldAge=%lf", &( inCell->heldAge ) );
    inCell->returnHeldAge = inCell->heldAge;
    next++;
    
    
    next = scanClothingSet( inLines, next, &( inCell->heldClothing ) );


    int anim;
    sscanf( inLines[next], "anim=%d", &anim );
    inCell->anim = (AnimType)anim;
    next++;

    
    sscanf( inLines[next], "frozenAnimTime=%lf", &( inCell->frozenAnimTime ) );
    next++;

    
    sscanf( inLines[next], "numUsesRemaining=%d",
            &( inCell->numUsesRemaining ) );
    next++;

    
    sscanf( inLines[next], "xOffset=%d", &( inCell->xOffset ) );
    next++;

    sscanf( inLines[next], "yOffset=%d", &( inCell->yOffset ) );
    next++;


    sscanf( inLines[next], "destCellXOffset=%d", &( inCell->destCellXOffset ) );
    next++;

    
    sscanf( inLines[next], "destCellYOffset=%d", &( inCell->destCellYOffset ) );
    next++;

    sscanf( inLines[next], "moveDelayTime=%lf", &( inCell->moveDelayTime ) );
    next++;


    return next;
    }




char EditorScenePage::tryLoadScene( int inSceneID ) {
    mPlayingTime = false;
    
    File *f = getSceneFile( inSceneID );
    
    char r = false;
    if( f == NULL ) {
        clearScene();
        return r;
        }
    
    if( f->exists() && ! f->isDirectory() ) {
        
        printf( "Trying to load scene %d\n", inSceneID );
        
        char *fileText = f->readFileContents();
        
        if( fileText != NULL ) {
            
            while ( queueSize > 0 ) {
                queuesPopBack();
                }
            queueIndex = 0;
            
            int numLines = 0;
            
            char **lines = split( fileText, "\n", &numLines );
            delete [] fileText;
            clearScene(); // If we fail to load a scene, we're still on a new ID, so clear it.

            if( numLines > 1 ) { // One line files obviously aren't real.

                int next = 0;


                int w = mSceneW;
                int h = mSceneH;

                if( next < numLines ) { // if we ever look at lines[next] after running out of lines
                    sscanf( lines[next], "w=%d", &w );    // we will have a segfault.
                    next++;
                    }

                if( next < numLines ) {
                    sscanf( lines[next], "h=%d", &h );
                    next++;
                    }

                if( w != mSceneW || h != mSceneH ) {
                    resizeGrid( h, w );
                    }

                if( next < numLines ) {
                    if( strstr( lines[next], "origin" ) != NULL ) {
                        sscanf( lines[next], "origin=%d,%d", &mZeroX, &mZeroY );
                        next++;
                        }
                    }
                
                char floorPresent = false;
                if( next < numLines ) {
                    if( strstr( lines[next], "floorPresent" ) != NULL ) {
                        floorPresent = true;
                        next++;
                        }
                    }
                clearScene();

                int numRead = 0;
                int x, y;
                if (next < numLines) {
                    numRead = sscanf( lines[next], "x=%d,y=%d", &x, &y );
                    next++;
                    }

                while( numRead == 2 ) {
                    SceneCell *c = &( mCells[y][x] );
                    SceneCell *p = &( mPersonCells[y][x] );
                    SceneCell *f = &( mFloorCells[y][x] );

                    next = scanCell( lines, next, c ); // TODO: Keep scanCell from segfaulting when the file was interrupted midstream.
                    next = scanCell( lines, next, p );

                    if( floorPresent ) {
                        next = scanCell( lines, next, f );
                        }

                    numRead = 0;

                    if( next < numLines ) {
                        numRead = sscanf( lines[next], "x=%d,y=%d", &x, &y );
                        next++;
                        }
                    }
                    
                }
            
            for( int i=0; i<numLines; i++ ) {
                delete [] lines[i];
                }
            delete [] lines;

            r = true;
            mCurX = mZeroX;
            mCurY = mZeroY;
            mShiftX = 0;
            mShiftY = 0;
            
            spriteCount = round( std::max(mSceneH, mSceneW) / 200 * 25 );
                
            }
        }    
    
    delete f;
    
    return r;
    }

        


void EditorScenePage::checkNextPrevVisible() {
    
    std::vector<File*>cf = getSceneFiles();
    int num = cf.size();
    mNextSceneNumber = num; // Keep mNextSceneNumber out of bounds, so new files will be new.
        
    mNextSceneButton.setVisible( false );
    mPrevSceneButton.setVisible( false );

    if( mSceneID == -1 ) { // We're on an unsaved scene. We can always look for the latest from here.
        mNextSceneButton.setVisible( false );
        
        mPrevSceneButton.setVisible( num >= 1 );
        }
    else { // Bounds are from 0 to one less than the number of files in the directory.
        if( mSceneID < num - 1 ) {
            mNextSceneButton.setVisible( true );
            }
        if( mSceneID > 0 && num > 0 ) {
            mPrevSceneButton.setVisible( true );
            }
        }
        
    for( int i=0; i<num; i++ ) {
        delete cf[i];
        }
    
    }



void EditorScenePage::resizeGrid( int inNewH, int inNewW ) {
    if( mCells != NULL ) {
        
        for( int y=0; y<mSceneH; y++ ) {
            delete [] mCells[y];
            delete [] mPersonCells[y];
            delete [] mFloorCells[y];
            }
        delete [] mCells;
        delete [] mPersonCells;
        delete [] mFloorCells;
        }
    

    mSceneH = inNewH;
    mSceneW = inNewW;
    
    mCells = new SceneCell*[mSceneH];
    mPersonCells = new SceneCell*[mSceneH];
    mFloorCells = new SceneCell*[mSceneH];
    
    
    for( int y=0; y<mSceneH; y++ ) {
        mCells[y] = new SceneCell[ mSceneW ];
        mPersonCells[y] = new SceneCell[ mSceneW ];
        mFloorCells[y] = new SceneCell[ mSceneW ];
        
        for( int x=0; x<mSceneW; x++ ) {
            mCells[y][x] = mEmptyCell;
            mPersonCells[y][x] = mEmptyCell;
            mFloorCells[y][x] = mEmptyCell;
            }
        }
    }


        

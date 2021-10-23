//
// Created by olivier on 23/10/2021.
//
/*
 * Modification History
 *
 * 2010-September-3  Jason Rohrer
 * Fixed mouse to world translation function.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>


// let SDL override our main function with SDLMain
#include <SDL/SDL_main.h>


// must do this before SDL include to prevent WinMain linker errors on win32
int mainFunction( int inArgCount, char **inArgs );

int main( int inArgCount, char **inArgs ) {
	return mainFunction( inArgCount, inArgs );
}


#include <SDL/SDL.h>



#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/Color.h"

#include "minorGems/graphics/openGL/gui/GUIComponentGL.h"
#include "minorGems/network/web/WebRequest.h"

#include "minorGems/graphics/openGL/glInclude.h"

#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/ui/event/ActionListener.h"


#include "minorGems/system/Time.h"
#include "minorGems/system/Thread.h"

#include "minorGems/io/file/File.h"

#include "minorGems/network/HostAddress.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketClient.h"

#include "minorGems/network/upnp/portMapping.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/TranslationManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"


#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/log/FileLog.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/util/ByteBufferInputStream.h"


#include "minorGems/sound/formats/aiff.h"
#include "minorGems/sound/audioNoClip.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/game/diffBundle/client/diffBundleClient.h"



#include "minorGems/util/random/CustomRandomSource.h"

// static seed
static CustomRandomSource randSource( 34957197 );


#ifdef RASPBIAN

#include "minorGems/graphics/openGL/RaspbianGLSurface.cpp"

// demo code panel uses non-GLES code
void showDemoCodePanel( ScreenGL *inScreen, const char *inFontTGAFileName,
                        int inWidth, int inHeight ) {
    }
char isDemoCodePanelShowing() {
    return false;
    }
void freeDemoCodePanel() {
    }
void showWriteFailedPanel( ScreenGL *inScreen, const char *inFontTGAFileName,
                           int inWidth, int inHeight ) {
    }
void freeWriteFailedPanel() {
    }


#else

#include "minorGems/game/platforms/SDL/demoCodePanel.h"

#endif


static int nextAsyncFileHandle = 0;

typedef struct AsyncFileRecord {
	int handle;
	char *filePath;

	int dataLength;
	unsigned char *data;

	char doneReading;

} AsyncFileRecord;



#include "minorGems/system/StopSignalThread.h"
#include "minorGems/system/MutexLock.h"
#include "minorGems/system/BinarySemaphore.h"

static MutexLock asyncLock;
static BinarySemaphore newFileToReadSem;

static BinarySemaphore newFileDoneReadingSem;

static SimpleVector<AsyncFileRecord> asyncFiles;


class AsyncFileThread : public StopSignalThread {

public:

	AsyncFileThread() {
		start();
	}

	~AsyncFileThread() {
		stop();
		newFileToReadSem.signal();
		join();

		for( int i=0; i<asyncFiles.size(); i++ ) {

			AsyncFileRecord *r = asyncFiles.getElement( i );

			if( r->filePath != NULL ) {
				delete [] r->filePath;
			}

			if( r->data != NULL ) {
				delete [] r->data;
			}
		}
		asyncFiles.deleteAll();
	}


	virtual void run() {
		while( ! isStopped() ) {

			int handleToRead = -1;
			char *pathToRead = NULL;

			asyncLock.lock();

			for( int i=0; i<asyncFiles.size(); i++ ) {
				AsyncFileRecord *r = asyncFiles.getElement( i );

				if( ! r->doneReading ) {
					// can't trust pointer to record in vector
					// outside of lock, because vector storage may
					// change
					handleToRead = r->handle;
					pathToRead = r->filePath;
					break;
				}
			}

			asyncLock.unlock();

			if( handleToRead != -1 ) {
				// read file data

				File f( NULL, pathToRead );

				int dataLength;
				unsigned char *data = f.readFileContents( &dataLength );

				// re-lock vector, search for handle, and add it
				// cannot count on vector order or pointers to records
				// when we don't have it locked

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( r->handle == handleToRead ) {
						r->dataLength = dataLength;
						r->data = data;
						r->doneReading = true;
						break;
					}
				}

				asyncLock.unlock();

				// let anyone waiting for a new file to finish
				// reading (only matters in the case of playback, where
				// the file-done must happen on a specific frame)
				newFileDoneReadingSem.signal();
			}
			else {
				// wait on binary semaphore until something else added
				// for us to read
				newFileToReadSem.wait();
			}
		}
	};
};


static AsyncFileThread fileReadThread;




// some settings

static int cursorMode = 0;
static double emulatedCursorScale = 1.0;


// size of game image
int gameWidth = 320;
int gameHeight = 240;

// size of screen for fullscreen mode
int screenWidth = 640;
int screenHeight = 480;


int idealTargetFrameRate = 60;
int targetFrameRate = idealTargetFrameRate;

SimpleVector<int> possibleFrameRates;


char countingOnVsync = false;


int soundSampleRate = 22050;
//int soundSampleRate = 44100;


char soundRunning = false;
char soundOpen = false;


char hardToQuitMode = false;
int pauseOnMinimize = 1;


char demoMode = false;

char writeFailed = false;
char loadingFailedFlag = false;
char *loadingFailedMessage = NULL;


char loadingDone = false;


char loadingMessageShown = false;
char frameDrawerInited = false;


// ^ and & keys to slow down and speed up for testing
// read from settings folder
char enableSpeedControlKeys = false;

// should each and every frame be saved to disk?
// useful for making videos
char outputAllFrames = false;

// should output only every other frame, and blend in dropped frames?
char blendOutputFramePairs = false;

float blendOutputFrameFraction = 0;


char *webProxy = NULL;



static unsigned char *lastFrame_rgbaBytes = NULL;


static unsigned int frameNumber = 0;


static char recordAudio = false;

static FILE *aiffOutFile = NULL;

static int samplesLeftToRecord = 0;

static char bufferSizeHinted = false;





static char measureFrameRate = true;
static char startMeasureTimeRecorded = false;


static double startMeasureTime = 0;

static int numFramesMeasured = 0;

// show measure screen longer if there's a vsync warning
static double noWarningSecondsToMeasure = 1;
static double warningSecondsToMeasure = 3;

static double secondsToMeasure = noWarningSecondsToMeasure;

static int numFramesSkippedBeforeMeasure = 0;
static int numFramesToSkipBeforeMeasure = 30;

static char measureRecorded = false;







char mouseWorldCoordinates = true;


#ifdef USE_JPEG
#include "minorGems/graphics/converters/JPEGImageConverter.h"
    static JPEGImageConverter screenShotConverter( 90 );
    static const char *screenShotExtension = "jpg";
#elif defined(USE_PNG)
#include "minorGems/graphics/converters/PNGImageConverter.h"
    static PNGImageConverter screenShotConverter;
    static const char *screenShotExtension = "png";
#else
static TGAImageConverter screenShotConverter;
static const char *screenShotExtension = "tga";
#endif


// should screenshot be taken at end of next redraw?
static char shouldTakeScreenshot = false;
static char manualScreenShot = false;

static char *screenShotPrefix = NULL;

static void takeScreenShot();







class GameSceneHandler :
		public SceneHandlerGL, public MouseHandlerGL, public KeyboardHandlerGL,
		public RedrawListenerGL, public ActionListener  {

public:

	/**
	 * Constructs a sceen handler.
	 *
	 * @param inScreen the screen to interact with.
	 *   Must be destroyed by caller after this class is destroyed.
	 */
	GameSceneHandler( ScreenGL *inScreen );

	virtual ~GameSceneHandler();



	/**
	 * Executes necessary init code that reads from files.
	 *
	 * Must be called before using a newly-constructed GameSceneHandler.
	 *
	 * This call assumes that the needed files are in the current working
	 * directory.
	 */
	void initFromFiles();



	ScreenGL *mScreen;








	// implements the SceneHandlerGL interface
	virtual void drawScene();

	// implements the MouseHandlerGL interface
	virtual void mouseMoved( int inX, int inY );
	virtual void mouseDragged( int inX, int inY );
	virtual void mousePressed( int inX, int inY );
	virtual void mouseReleased( int inX, int inY );

	// implements the KeyboardHandlerGL interface
	virtual char isFocused() {
		// always focused
		return true;
	}
	virtual void keyPressed( unsigned char inKey, int inX, int inY );
	virtual void specialKeyPressed( int inKey, int inX, int inY );
	virtual void keyReleased( unsigned char inKey, int inX, int inY );
	virtual void specialKeyReleased( int inKey, int inX, int inY );

	// implements the RedrawListener interface
	virtual void fireRedraw();


	// implements the ActionListener interface
	virtual void actionPerformed( GUIComponent *inTarget );


	char mPaused;
	char mPausedDuringFrameBatch;
	char mLoadingDuringFrameBatch;

	// reduce sleep time when user hits keys to restore responsiveness
	unsigned int mPausedSleepTime;


	char mBlockQuitting;

	double mLastFrameRate;


protected:

	timeSec_t mStartTimeSeconds;


	char mPrintFrameRate;
	unsigned long mNumFrames;
	unsigned long mFrameBatchSize;
	double mFrameBatchStartTimeSeconds;



	Color mBackgroundColor;


};



GameSceneHandler *sceneHandler;
ScreenGL *screen;


// how many pixels wide is each game pixel drawn as?
int pixelZoomFactor;




typedef struct WebRequestRecord {
	int handle;
	WebRequest *request;
} WebRequestRecord;

SimpleVector<WebRequestRecord> webRequestRecords;




typedef struct SocketConnectionRecord {
	int handle;
	Socket *sock;
} SocketConnectionRecord;

SimpleVector<SocketConnectionRecord> socketConnectionRecords;



void getScreenDimensions( int *outWidth, int *outHeight ) {
	*outWidth = screenWidth;
	*outHeight = screenHeight;
}



typedef struct SoundSprite {
	int handle;
	int numSamples;
	int samplesPlayed;

	// true for sound sprites that are marked to never use
	// pitch and volume variance
	char noVariance;

	// floating point position of next interpolated sameple to play
	// (for variable rate playback)
	double samplesPlayedF;


	Sint16 *samples;
} SoundSprite;



// don't worry about dynamic mallocs here
// because we don't need to lock audio thread before adding values
// (playing sound sprites is handled directly through SoundSprite* handles
//  without accessing this vector).
static SimpleVector<SoundSprite*> soundSprites;

// audio thread is locked every time we touch this vector
// So, we want to avoid mallocs here
// Can we imagine more than 100 sound sprites ever playing at the same time?
static SimpleVector<SoundSprite> playingSoundSprites( 100 );

static double *soundSpriteMixingBufferL = NULL;
static double *soundSpriteMixingBufferR = NULL;


// variable rate per sprite
// these are also touched with audio thread locked, so avoid mallocs
// we won't see more than 100 of these simultaneously either
static SimpleVector<double> playingSoundSpriteRates( 100 );
static SimpleVector<double> playingSoundSpriteVolumesR( 100 );
static SimpleVector<double> playingSoundSpriteVolumesL( 100 );


static SDL_Cursor *ourCursor = NULL;


// function that destroys object when exit is called.
// exit is the only way to stop the loop in  ScreenGL
void cleanUpAtExit() {

	if( ourCursor != NULL ) {
		SDL_FreeCursor( ourCursor );
	}

	AppLog::info( "exiting...\n" );

	if( soundOpen ) {
		AppLog::info( "exiting: calling SDL_CloseAudio\n" );
		SDL_CloseAudio();
	}
	soundOpen = false;


	AppLog::info( "exiting: Deleting sceneHandler\n" );
	delete sceneHandler;



	AppLog::info( "Freeing sound sprites\n" );
	for( int i=0; i<soundSprites.size(); i++ ) {
		SoundSprite *s = soundSprites.getElementDirect( i );
		delete [] s->samples;
		delete s;
	}
	soundSprites.deleteAll();
	playingSoundSprites.deleteAll();
	playingSoundSpriteRates.deleteAll();
	playingSoundSpriteVolumesL.deleteAll();
	playingSoundSpriteVolumesR.deleteAll();

	if( bufferSizeHinted ) {
		freeHintedBuffers();
		bufferSizeHinted = false;
	}


	if( frameDrawerInited ) {
		AppLog::info( "exiting: freeing frameDrawer\n" );
		freeFrameDrawer();
	}


	AppLog::info( "exiting: Deleting screen\n" );
	delete screen;


	AppLog::info( "exiting: freeing drawString\n" );
	freeDrawString();

#ifdef RASPBIAN
	raspbianReleaseSurface();
#endif

	AppLog::info( "exiting: calling SDL_Quit()\n" );
	SDL_Quit();

	if( screenShotPrefix != NULL ) {
		AppLog::info( "exiting: deleting screenShotPrefix\n" );

		delete [] screenShotPrefix;
		screenShotPrefix = NULL;
	}

	if( lastFrame_rgbaBytes != NULL ) {
		AppLog::info( "exiting: deleting lastFrame_rgbaBytes\n" );

		delete [] lastFrame_rgbaBytes;
		lastFrame_rgbaBytes = NULL;
	}


	if( recordAudio ) {
		AppLog::info( "exiting: closing audio output file\n" );

		recordAudio = false;
		fclose( aiffOutFile );
		aiffOutFile = NULL;
	}

	for( int i=0; i<webRequestRecords.size(); i++ ) {
		AppLog::infoF( "exiting: Deleting lingering web request %d\n", i );

		WebRequestRecord *r = webRequestRecords.getElement( i );

		delete r->request;
	}

	if( webProxy != NULL ) {
		delete [] webProxy;
		webProxy = NULL;
	}

	if( soundSpriteMixingBufferL != NULL ) {
		delete [] soundSpriteMixingBufferL;
	}

	if( soundSpriteMixingBufferR != NULL ) {
		delete [] soundSpriteMixingBufferR;
	}


	AppLog::info( "exiting: Done.\n" );
}


static int nextSoundSpriteHandle;





static double soundSpriteRateMax = 1.0;
static double soundSpriteRateMin = 1.0;


void setSoundSpriteRateRange( double inMin, double inMax ) {
	soundSpriteRateMin = inMin;
	soundSpriteRateMax = inMax;
}

static double soundSpriteVolumeMax = 1.0;
static double soundSpriteVolumeMin = 1.0;


void setSoundSpriteVolumeRange( double inMin, double inMax ) {
	soundSpriteVolumeMin = inMin;
	soundSpriteVolumeMax = inMax;
}


static float soundLoudness = 1.0f;

static float currentSoundLoudness = 0.0f;

static float soundLoudnessIncrementPerSample = 0.0f;

static float soundSpriteGlobalLoudness = 1.0f;


static char soundSpritesFading = false;
static float soundSpriteFadeIncrementPerSample = 0.0f;


void setSoundLoudness( float inLoudness ) {
	lockAudio();
	soundLoudness = inLoudness;
	currentSoundLoudness = inLoudness;
	unlockAudio();
}


void fadeSoundSprites( double inFadeSeconds ) {
	lockAudio();
	soundSpritesFading = true;

	soundSpriteFadeIncrementPerSample =
			1.0f / ( inFadeSeconds * soundSampleRate );

	unlockAudio();
}



void resumePlayingSoundSprites() {
	lockAudio();
	soundSpritesFading = false;
	soundSpriteGlobalLoudness = 1.0f;
	unlockAudio();
}





SoundSpriteHandle loadSoundSprite( const char *inAIFFFileName ) {
	return loadSoundSprite( "sounds", inAIFFFileName );
}


SoundSpriteHandle loadSoundSprite( const char *inFolderName,
								   const char *inAIFFFileName ) {

	File aiffFile( new Path( inFolderName ), inAIFFFileName );

	if( ! aiffFile.exists() ) {
		printf( "File does not exist in sounds folder: %s\n",
				inAIFFFileName );
		return NULL;
	}

	int numBytes;
	unsigned char *data = aiffFile.readFileContents( &numBytes );


	if( data == NULL ) {
		printf( "Failed to read sound file: %s\n", inAIFFFileName );
		return NULL;
	}


	int numSamples;
	int16_t *samples = readMono16AIFFData( data, numBytes, &numSamples );

	delete [] data;

	if( samples == NULL ) {
		printf( "Failed to parse AIFF sound file: %s\n", inAIFFFileName );
		return NULL;
	}

	SoundSpriteHandle s = setSoundSprite( samples, numSamples );

	delete [] samples;

	return s;
}



SoundSpriteHandle setSoundSprite( int16_t *inSamples, int inNumSamples ) {
	SoundSprite *s = new SoundSprite;

	s->handle = nextSoundSpriteHandle ++;
	s->numSamples = inNumSamples;

	s->noVariance = false;

	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;

	s->samples = new Sint16[ s->numSamples ];

	memcpy( s->samples, inSamples, inNumSamples * sizeof( int16_t ) );

	soundSprites.push_back( s );

	return (SoundSpriteHandle)s;
}



void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance ) {
	SoundSprite *s = (SoundSprite*)inHandle;
	s->noVariance = inNoVariance;
}




static double maxTotalSoundSpriteVolume = 1.0;

static double soundSpriteCompressionFraction = 0.0;

static double totalSoundSpriteNormalizeFactor = 1.0;

static NoClip soundSpriteNoClip;

static NoClip totalAudioMixNoClip;


void setMaxTotalSoundSpriteVolume( double inMaxTotal,
								   double inCompressionFraction ) {
	lockAudio();

	maxTotalSoundSpriteVolume = inMaxTotal;
	soundSpriteCompressionFraction = inCompressionFraction;

	totalSoundSpriteNormalizeFactor =
			1.0 / ( 1.0 - soundSpriteCompressionFraction );

	soundSpriteNoClip =
			resetAudioNoClip( ( 1.0 - soundSpriteCompressionFraction ) *
							  maxTotalSoundSpriteVolume * 32767,
					// half second hold and release
							  soundSampleRate / 2, soundSampleRate / 2 );

	unlockAudio();
}


static int maxSimultaneousSoundSprites = -1;


void setMaxSimultaneousSoundSprites( int inMaxCount ) {
	maxSimultaneousSoundSprites = inMaxCount;
}




static double pickRandomRate() {
	if( soundSpriteRateMax != 1.0 ||
		soundSpriteRateMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteRateMin,
												  soundSpriteRateMax );
	}
	else {
		return 1.0;
	}
}



static double pickRandomVolume() {
	if( soundSpriteVolumeMax != 1.0 ||
		soundSpriteVolumeMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteVolumeMin,
												  soundSpriteVolumeMax );
	}
	else {
		return 1.0;
	}
}




// no locking
static void playSoundSpriteInternal(
		SoundSpriteHandle inHandle, double inVolumeTweak,
		double inStereoPosition,
		double inForceVolume = -1,
		double inForceRate = -1 ) {


	if( soundSpritesFading && soundSpriteGlobalLoudness == 0.0f ) {
		// don't play any new sound sprites
		return;
	}

	if( maxSimultaneousSoundSprites != -1 &&
		playingSoundSpriteVolumesR.size() >= maxSimultaneousSoundSprites ) {
		// cap would be exceeded
		// don't play this sound sprite at all
		return;
	}


	double volume = inVolumeTweak;

	SoundSprite *s = (SoundSprite*)inHandle;

	if( ! s->noVariance ) {

		if( inForceVolume == -1 ) {
			volume *= pickRandomVolume();
		}
		else {
			volume *= inForceVolume;
		}
	}






	// constant power rule
	double p = M_PI * inStereoPosition * 0.5;

	double rightVolume = volume * sin( p );
	double leftVolume = volume * cos( p );


	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;




	playingSoundSprites.push_back( *s );

	if( s->noVariance ) {
		playingSoundSpriteRates.push_back( 1.0 );
	}
	else {

		if( inForceRate != -1 ) {
			playingSoundSpriteRates.push_back( inForceRate );
		}
		else {
			playingSoundSpriteRates.push_back(  pickRandomRate() );
		}
	}



	playingSoundSpriteVolumesR.push_back( rightVolume );
	playingSoundSpriteVolumesL.push_back( leftVolume );
}


// locking
void playSoundSprite( SoundSpriteHandle inHandle, double inVolumeTweak,
					  double inStereoPosition ) {

	lockAudio();
	playSoundSpriteInternal( inHandle, inVolumeTweak, inStereoPosition );
	unlockAudio();
}



// multiple with single lock
void playSoundSprite( int inNumSprites, SoundSpriteHandle *inHandles,
					  double *inVolumeTweaks,
					  double *inStereoPositions ) {
	lockAudio();

	// one random volume and rate for whole batch
	double volume = pickRandomVolume();
	double rate = pickRandomRate();

	for( int i=0; i<inNumSprites; i++ ) {
		playSoundSpriteInternal( inHandles[i], inVolumeTweaks[i],
								 inStereoPositions[i], volume, rate );
	}
	unlockAudio();
}





void freeSoundSprite( SoundSpriteHandle inHandle ) {
	// make sure this sprite isn't playing
	lockAudio();

	SoundSprite *s = (SoundSprite*)inHandle;

	// find it in vector to remove it
	for( int i=playingSoundSprites.size()-1; i>=0; i-- ) {
		SoundSprite *s2 = playingSoundSprites.getElement( i );
		if( s2->handle == s->handle ) {
			// stop it abruptly
			playingSoundSprites.deleteElement( i );
			playingSoundSpriteRates.deleteElement( i );
			playingSoundSpriteVolumesL.deleteElement( i );
			playingSoundSpriteVolumesR.deleteElement( i );
		}
	}

	unlockAudio();


	for( int i=0; i<soundSprites.size(); i++ ) {
		SoundSprite *s2 = soundSprites.getElementDirect( i );
		if( s2->handle == s->handle ) {
			delete [] s2->samples;
			soundSprites.deleteElement( i );
			delete s2;
		}
	}
}




void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill ) {
	getSoundSamples( inStream, inLengthToFill );

	int numSamples = inLengthToFill / 4;


	if( playingSoundSprites.size() > 0 ) {

		for( int i=0; i<numSamples; i++ ) {
			soundSpriteMixingBufferL[ i ] = 0.0;
			soundSpriteMixingBufferR[ i ] = 0.0;
		}

		for( int i=0; i<playingSoundSprites.size(); i++ ) {
			SoundSprite *s = playingSoundSprites.getElement( i );

			double rate = playingSoundSpriteRates.getElementDirect( i );
			double volumeL = playingSoundSpriteVolumesL.getElementDirect( i );
			double volumeR = playingSoundSpriteVolumesR.getElementDirect( i );

			int filled = 0;

			//int filledBytes = 0;

			if( rate == 1 ) {

				int samplesPlayed = s->samplesPlayed;
				int spriteNumSamples = s->numSamples;

				while( filled < numSamples &&
					   samplesPlayed < spriteNumSamples  ) {

					Sint16 sample = s->samples[ samplesPlayed ];

					soundSpriteMixingBufferL[ filled ]
							+= volumeL * sample;

					soundSpriteMixingBufferR[ filled ]
							+= volumeR * sample;

					filled ++;
					samplesPlayed ++;
				}
				s->samplesPlayed = samplesPlayed;
			}
			else {
				double samplesPlayedF = s->samplesPlayedF;
				int spriteNumSamples = s->numSamples;

				while( filled < numSamples &&
					   samplesPlayedF < spriteNumSamples - 1  ) {

					int aIndex = (int)floor( samplesPlayedF );
					int bIndex = (int)ceil( samplesPlayedF );

					Sint16 sampleA = s->samples[ aIndex ];
					Sint16 sampleB = s->samples[ bIndex ];

					double bWeight = samplesPlayedF - aIndex;
					double aWeight = 1 - bWeight;

					double sampleBlend = sampleA * aWeight + sampleB * bWeight;

					soundSpriteMixingBufferL[ filled ]
							+= volumeL * sampleBlend;

					soundSpriteMixingBufferR[ filled ]
							+= volumeR * sampleBlend;

					filled ++;
					samplesPlayedF += rate;
				}
				s->samplesPlayedF = samplesPlayedF;
			}
		}


		// respect their collective volume cap
		audioNoClip( &soundSpriteNoClip,
					 soundSpriteMixingBufferL, soundSpriteMixingBufferR,
					 numSamples );

		// and normalize to compensate for any compression below that cap
		if( totalSoundSpriteNormalizeFactor != 1.0 ) {
			for( int i=0; i<numSamples; i++ ) {
				soundSpriteMixingBufferL[i] *= totalSoundSpriteNormalizeFactor;
				soundSpriteMixingBufferR[i] *= totalSoundSpriteNormalizeFactor;
			}
		}


		// now mix them in
		int filledBytes = 0;


		// next, do final mix, then
		//  apply global no-clip, for mix of sound sprites and
		// music or other sounds created by getSoundSamples

		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample =
					(Sint16)( (inStream[filledBytes+1] << 8) |
							  inStream[filledBytes] );
			Sint16 rSample =
					(Sint16)( (inStream[filledBytes+3] << 8) |
							  inStream[filledBytes+2] );

			filledBytes += 4;

			// apply global loudness to sound sprites as part of this mx
			soundSpriteMixingBufferL[i] *= soundSpriteGlobalLoudness;
			soundSpriteMixingBufferR[i] *= soundSpriteGlobalLoudness;

			if( soundSpritesFading ) {
				soundSpriteGlobalLoudness -= soundSpriteFadeIncrementPerSample;

				if( soundSpriteGlobalLoudness < 0.0f ) {
					soundSpriteGlobalLoudness = 0.0f;
				}
			}


			soundSpriteMixingBufferL[i] += lSample;
			soundSpriteMixingBufferR[i] += rSample;
		}



		// we have our final mix, make sure it never clips
		audioNoClip( &totalAudioMixNoClip,
					 soundSpriteMixingBufferL, soundSpriteMixingBufferR,
					 numSamples );


		// now convert back to integers
		filledBytes = 0;
		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample = lrint( soundSpriteMixingBufferL[i] );
			Sint16 rSample = lrint( soundSpriteMixingBufferR[i] );


			inStream[filledBytes++] = (Uint8)( lSample & 0xFF );
			inStream[filledBytes++] =
					(Uint8)( ( lSample >> 8 ) & 0xFF );
			inStream[filledBytes++] = (Uint8)( rSample & 0xFF );
			inStream[filledBytes++] =
					(Uint8)( ( rSample >> 8 ) & 0xFF );
		}

		// walk backward, removing any that are done
		// OR remove all if sound sprites are completely faded out
		for( int i=playingSoundSprites.size()-1; i>=0; i-- ) {
			SoundSprite *s = playingSoundSprites.getElement( i );

			if( soundSpriteGlobalLoudness == 0 ||
				s->samplesPlayed >= s->numSamples ||
				s->samplesPlayedF >= s->numSamples - 1 ) {

				playingSoundSprites.deleteElement( i );
				playingSoundSpriteRates.deleteElement( i );
				playingSoundSpriteVolumesL.deleteElement( i );
				playingSoundSpriteVolumesR.deleteElement( i );
			}
		}
	}

	// now apply global loudness fade for pause
	if( ( currentSoundLoudness != soundLoudness && ! sceneHandler->mPaused )
		||
		( currentSoundLoudness != 0.0f && sceneHandler->mPaused )
		||
		currentSoundLoudness != 1.0f ) {


		int nextByte = 0;
		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample =
					inStream[nextByte] |
					( inStream[nextByte + 1] << 8 );

			Sint16 rSample =
					inStream[nextByte + 2] |
					( inStream[nextByte + 3] << 8 );

			lSample = (Sint16)( lSample * currentSoundLoudness );
			rSample = (Sint16)( rSample * currentSoundLoudness );


			inStream[nextByte++] = (Uint8)( lSample & 0xFF );
			inStream[nextByte++] = (Uint8)( ( lSample >> 8 ) & 0xFF );
			inStream[nextByte++] = (Uint8)( rSample & 0xFF );
			inStream[nextByte++] = (Uint8)( ( rSample >> 8 ) & 0xFF );

			if( currentSoundLoudness != soundLoudness &&
				! sceneHandler->mPaused ) {
				currentSoundLoudness += soundLoudnessIncrementPerSample;

				if( currentSoundLoudness > soundLoudness ) {
					currentSoundLoudness = soundLoudness;
				}
			}
			else if( currentSoundLoudness != 0 &&
					 sceneHandler->mPaused ) {

				currentSoundLoudness -= soundLoudnessIncrementPerSample;

				if( currentSoundLoudness < 0 ) {
					currentSoundLoudness = 0;
				}
			}

		}
	}


	if( recordAudio ) {


		if( numSamples > samplesLeftToRecord ) {
			numSamples = samplesLeftToRecord;
		}

		// reverse byte order
		int nextByte = 0;
		for( int i=0; i<numSamples; i++ ) {

			fwrite( &( inStream[ nextByte + 1 ] ), 1, 1, aiffOutFile );
			fwrite( &( inStream[ nextByte ] ), 1, 1, aiffOutFile );

			fwrite( &( inStream[ nextByte + 3 ] ), 1, 1, aiffOutFile );
			fwrite( &( inStream[ nextByte + 2 ] ), 1, 1, aiffOutFile );
			nextByte += 4;
		}


		samplesLeftToRecord -= numSamples;


		if( samplesLeftToRecord <= 0 ) {
			recordAudio = false;
			fclose( aiffOutFile );
			aiffOutFile = NULL;
		}
	}

}


int getSampleRate() {
	return soundSampleRate;
}


void setSoundPlaying( char inPlaying ) {
	SDL_PauseAudio( !inPlaying );
}



void lockAudio() {
	SDL_LockAudio();
}



void unlockAudio() {
	SDL_UnlockAudio();
}



char isSoundRunning() {
	return soundRunning;
}



#ifdef __mac__

#include <unistd.h>
#include <stdarg.h>


// returns path to folder
static char *pickFolder( const char *inPrompt ) {

    const char *commandFormat =
        "osascript -e 'tell application \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to return POSIX path of "
        "(choose folder with prompt \"%s\")'";

    char *command = autoSprintf( commandFormat, inPrompt );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for picking a folder." );
        }
    else {
        char buffer[200];

        int numRead = fscanf( osascriptPipe, "%199s", buffer );

        if( numRead == 1 ) {
            return stringDuplicate( buffer );
            }

        pclose( osascriptPipe );
        }
    return NULL;
    }



static void showMessage( const char *inAppName,
                         const char *inTitle, const char *inMessage,
                         char inError = false ) {
    const char *iconName = "note";
    if( inError ) {
        // stop icon is broken in OS 10.12
        // always use note
        // iconName = "stop";
        }

    const char *commandFormat =
        "osascript -e 'tell app \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to display dialog \"%s\" "
        "with title \"%s:  %s\" buttons \"Ok\" "
        "with icon %s default button \"Ok\"' ";

    char *command = autoSprintf( commandFormat, inMessage, inAppName, inTitle,
                                 iconName );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for displaying GUI messages." );
        }
    else {
        pclose( osascriptPipe );
        }
    }


static char *getPrefFilePath() {
    return autoSprintf( "%s/Library/Preferences/%s_prefs.txt",
                        getenv( "HOME" ),
                        getAppName() );
    }


static char isSettingsFolderFound() {

    File settingsFolder( NULL, "settings" );

    if( settingsFolder.exists() && settingsFolder.isDirectory() ) {
        return true;
        }

    return false;
    }


#endif



#ifdef WIN32

#include <windows.h>
#include <tchar.h>

#endif



int mainFunction( int inNumArgs, char **inArgs ) {

#ifdef WIN32


	HMODULE hShcore = LoadLibrary( _T( "shcore.dll" ) );

    if( hShcore != NULL ) {
        printf( "shcore.dll loaded successfully\n" );

        typedef enum _PROCESS_DPI_AWARENESS {
            PROCESS_DPI_UNAWARE            = 0,
            PROCESS_SYSTEM_DPI_AWARE       = 1,
            PROCESS_PER_MONITOR_DPI_AWARE  = 2
            } PROCESS_DPI_AWARENESS;

        typedef HRESULT (*SetProcessDpiAwarenessFunc)( PROCESS_DPI_AWARENESS );

        SetProcessDpiAwarenessFunc setAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(
                hShcore,
                "SetProcessDpiAwareness" );

        if( setAwareness ) {
            printf( "Found SetProcessDpiAwareness function in shcore.dll\n" );
            setAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
            }
        else {
            printf( "Could NOT find SetProcessDpiAwareness function in "
                    "Shcore.dll\n" );
            }
        FreeLibrary( hShcore );
        }
    else {
        printf( "shcore.dll NOT loaded successfully... pre Win 8.1 ?\n" );


        printf( "   Trying to load user32.dll instead\n" );


        // backwards compatible code
        // on vista and higher, this will tell Windows that we are DPI aware
        // and that we should not be artificially scaled.
        //
        // Found here:  http://www.rw-designer.com/DPI-aware
        HMODULE hUser32 = LoadLibrary( _T( "user32.dll" ) );

        if( hUser32 != NULL ) {

            typedef BOOL (*SetProcessDPIAwareFunc)();

            SetProcessDPIAwareFunc setDPIAware =
                (SetProcessDPIAwareFunc)GetProcAddress( hUser32,
                                                        "SetProcessDPIAware" );
            if( setDPIAware ) {
                printf( "Found SetProcessDPIAware function in user32.dll\n" );
                setDPIAware();
                }
            else {
                printf( "Could NOT find SetProcessDPIAware function in "
                        "user32.dll\n" );
                }
            FreeLibrary( hUser32 );
            }
        else {
            printf( "Failed to load user32.dll\n" );
            }
        }
#endif


	// check result below, after opening log, so we can log failure
	Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
	if( getUsesSound() ) {
		flags |= SDL_INIT_AUDIO;
	}

	int sdlResult = SDL_Init( flags );


	// do this mac check after initing SDL,
	// since it causes various Mac frameworks to be loaded (which can
	// change the current working directory out from under us)
#ifdef __mac__
	// make sure working directory is the same as the directory
        // that the app resides in
        // this is especially important on the mac platform, which
        // doesn't set a proper working directory for double-clicked
        // app bundles

        // arg 0 is the path to the app executable
        char *appDirectoryPath = stringDuplicate( inArgs[0] );

        char *bundleName = autoSprintf( "%s_%d.app",
                                        getAppName(), getAppVersion() );

        char *appNamePointer = strstr( appDirectoryPath, bundleName );

        if( appNamePointer != NULL ) {
            // terminate full app path to get parent directory
            appNamePointer[0] = '\0';

            chdir( appDirectoryPath );
            }


        if( ! isSettingsFolderFound() ) {
            // first, try setting dir based on preferences file
            char *prefFilePath = getPrefFilePath();

            FILE *prefFile = fopen( prefFilePath, "r" );

            if( prefFile != NULL ) {

                char path[200];

                int numRead = fscanf( prefFile, "%199s", path );

                if( numRead == 1 ) {
                    chdir( path );
                    }
                fclose( prefFile );
                }

            delete [] prefFilePath;
            }


        if( ! isSettingsFolderFound() ) {

            showMessage( getAppName(), "First Start Up Error",
                         "Cannot find game data.\n\n"
                         "Newer versions of MacOS have strict sandboxing, "
                         "so we have to work around this issue by asking "
                         "you a question.\n\n"
                         //"There will also be some info presented along the "
                         //"way for debugging purposes.\n\n"
                         "Hopefully, you will only have to do this once.",
                         true );


            //showMessage( getAppName(), "Debug Info, Executable path =",
            //             inArgs[0], false );


            //showMessage( getAppName(), "Debug Info, Home dir =",
            //             getenv( "HOME" ), false );


            showMessage( getAppName(), "First Start Up",
                         "Please locate the game folder "
                         "in the next dialog box.",
                         false );

            char *prompt = autoSprintf(
                "Find the game folder (where the %s App is located):",
                getAppName() );

            char *path = pickFolder( prompt );

            delete [] prompt;

            if( path != NULL ) {
                //showMessage( getAppName(), "Debug Info, Chosen path =",
                //             path, false );


                char *prefFilePath = getPrefFilePath();

                FILE *prefFile = fopen( prefFilePath, "w" );

                if( prefFilePath == NULL ) {
                    char *message =
                        autoSprintf( "Failed to open this preferences file "
                                     "for writing:\n\n%s\n\n"
                                     "You will have to find the game folder "
                                     "again at next startup.",
                                     prefFilePath );

                    showMessage( getAppName(), "First Start Up Error",
                                 message,
                                 true );
                    delete [] message;
                    }
                else {
                    fprintf( prefFile, path );
                    fclose( prefFile );
                    }

                delete [] prefFilePath;


                chdir( path );

                delete [] path;

                if( !isSettingsFolderFound() ) {
                    showMessage( getAppName(), "First Start Up Error",
                                 "Still cannot find game data, exiting.",
                                 true );
                    return 1;
                    }
                }
            else {
                showMessage( getAppName(), "First Start Up Error",
                             "Picking a folder failed, exiting.",
                             true );
                return 1;
                }

            }


        delete [] bundleName;
        delete [] appDirectoryPath;
#endif



	AppLog::setLog( new FileLog( "log.txt" ) );
	AppLog::setLoggingLevel( Log::DETAIL_LEVEL );

	AppLog::info( "New game starting up" );


	if( sdlResult < 0 ) {
		AppLog::getLog()->logPrintf(
				Log::CRITICAL_ERROR_LEVEL,
				"Couldn't initialize SDL: %s\n", SDL_GetError() );
		return 0;
	}


	if( doesOverrideGameImageSize() ) {
		getGameImageSize( &gameWidth, &gameHeight );
	}

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Target game image size:  %dx%d\n",
			gameWidth, gameHeight );


	// read screen size from settings
	char widthFound = false;
	int readWidth = SettingsManager::getIntSetting( "screenWidth",
													&widthFound );
	char heightFound = false;
	int readHeight = SettingsManager::getIntSetting( "screenHeight",
													 &heightFound );

	if( widthFound && heightFound ) {
		// override hard-coded defaults
		screenWidth = readWidth;
		screenHeight = readHeight;
	}

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Ideal window dimensions:  %dx%d\n",
			screenWidth, screenHeight );


	if( ! isNonIntegerScalingAllowed() &&
		screenWidth < gameWidth ) {

		AppLog::info(
				"Screen width smaller than target game width, fixing" );
		screenWidth = gameWidth;
	}
	if( ! isNonIntegerScalingAllowed() &&
		screenHeight < gameHeight ) {
		AppLog::info(
				"Screen height smaller than target game height, fixing" );
		screenHeight = gameHeight;
	}


	if( isNonIntegerScalingAllowed() ) {

		double screenRatio = (double)screenWidth / (double)screenHeight;
		double gameRatio = (double)gameWidth / (double)gameHeight;

		if( screenRatio > gameRatio ) {
			// screen too wide

			// tell game about this by making game image wider than requested}

			AppLog::info(
					"Screen has wider aspect ratio than desired game image, "
					"fixing by makign game image wider" );

			gameWidth = (int)( screenRatio * gameHeight );

			// if screen too narrow, this is already handled elsewhere
		}
	}


	char fullscreenFound = false;
	int readFullscreen = SettingsManager::getIntSetting( "fullscreen",
														 &fullscreenFound );

	char fullscreen = true;

	if( fullscreenFound && readFullscreen == 0 ) {
		fullscreen = false;
	}

	if( fullscreen ) {
		AppLog::info( "Trying to start in fullscreen mode." );
	}
	else {
		AppLog::info( "Trying to start in window mode." );
	}


	char useLargestWindowFound = false;
	int readUseLargestWindow =
			SettingsManager::getIntSetting( "useLargestWindow",
											&useLargestWindowFound );

	char useLargestWindow = true;

	if( useLargestWindowFound && readUseLargestWindow == 0 ) {
		useLargestWindow = false;
	}


	if( !fullscreen && useLargestWindow ) {
		AppLog::info( "Want to use largest window that fits on screen." );

		const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

		int currentW = currentScreenInfo->current_w;
		int currentH = currentScreenInfo->current_h;

		if( isNonIntegerScalingAllowed() ) {
			double aspectRatio = screenHeight / (double) screenWidth;

			int tryW = currentW;
			int tryH = lrint( aspectRatio * currentW );

			// never fill more than 85% of screen vertically, because
			// this large of a window is a pain to manage
			if( tryH >= 0.85 * currentH ) {
				tryH = lrint( 0.84 * currentH );
				tryW = lrint( tryH / aspectRatio );
			}
			if( tryW < screenWidth ) {
				// largest window is smaller than requested screen size,
				// that's okay.
				screenWidth = tryW;
				screenHeight = tryH;
			}
			else if( tryW > screenWidth ) {
				// we're attempting a blow-up
				// but this is not worth it if the blow-up is too small
				// we loose pixel accuracy without giving the user a much
				// larger image.
				// make sure it's at least 25% wider to be worth it,
				// otherwise, keep the requested window size
				if( tryW >= 1.25 * screenWidth ) {
					screenWidth = tryW;
					screenHeight = tryH;
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Largest-window mode would offer a <25% width "
							"increase, using requested window size instead." );
				}
			}
		}
		else {

			int blowUpFactor = 1;

			while( gameWidth * blowUpFactor < currentW &&
				   gameHeight * blowUpFactor < currentH ) {

				blowUpFactor++;
			}

			while( blowUpFactor > 1 &&
				   ( gameWidth * blowUpFactor >= currentW * 0.85 ||
					 gameHeight * blowUpFactor >= currentH * 0.85 ) ) {

				// scale back, because we don't want to totally
				// fill the screen (annoying to manage such a big window)

				// stop at a window that fills < 85% of screen in
				// either direction
				blowUpFactor --;
			}

			screenWidth = blowUpFactor * gameWidth;
			screenHeight = blowUpFactor * gameHeight;
		}


		AppLog::getLog()->logPrintf(
				Log::INFO_LEVEL,
				"Screen dimensions for largest-window mode:  %dx%d\n",
				screenWidth, screenHeight );
	}
	else if( !fullscreen && !useLargestWindow ) {
		// make sure window isn't too big for screen

		const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

		int currentW = currentScreenInfo->current_w;
		int currentH = currentScreenInfo->current_h;

		if( isNonIntegerScalingAllowed() &&
			( screenWidth > currentW || screenHeight > currentH ) ) {
			double aspectRatio = screenHeight / (double) screenWidth;

			// make window as wide as screen, preserving game aspect ratio
			screenWidth = currentW;

			int testScreenHeight = lrint( aspectRatio * screenWidth );

			if( testScreenHeight <= currentH ) {
				screenHeight = testScreenHeight;
			}
			else {
				screenHeight = currentH;

				screenWidth = lrint( screenHeight / aspectRatio );
			}
		}
		else {


			int blowDownFactor = 1;

			while( screenWidth / blowDownFactor > currentW
				   ||
				   screenHeight / blowDownFactor > currentH ) {
				blowDownFactor += 1;
			}

			if( blowDownFactor > 1 ) {
				screenWidth /= blowDownFactor;
				screenHeight /= blowDownFactor;
			}
		}
	}





	char frameRateFound = false;
	int readFrameRate = SettingsManager::getIntSetting( "halfFrameRate",
														&frameRateFound );

	if( frameRateFound && readFrameRate >= 1 ) {
		// cut frame rate in half N times
		targetFrameRate /= (int)pow( 2, readFrameRate );
	}

	// can't draw less than 1 frame per second
	if( targetFrameRate < 1 ) {
		targetFrameRate = 1;
	}

	SimpleVector<char*> *possibleFrameRatesSetting =
			SettingsManager::getSetting( "possibleFrameRates" );

	for( int i=0; i<possibleFrameRatesSetting->size(); i++ ) {
		char *f = possibleFrameRatesSetting->getElementDirect( i );

		int v;

		int numRead = sscanf( f, "%d", &v );

		if( numRead == 1 ) {
			possibleFrameRates.push_back( v );
		}

		delete [] f;
	}

	delete possibleFrameRatesSetting;


	if( possibleFrameRates.size() == 0 ) {
		possibleFrameRates.push_back( 60 );
		possibleFrameRates.push_back( 120 );
		possibleFrameRates.push_back( 144 );
	}

	AppLog::info( "The following frame rates are considered possible:" );

	for( int i=0; i<possibleFrameRates.size(); i++ ) {
		AppLog::infoF( "%d fps", possibleFrameRates.getElementDirect( i ) );
	}



	char recordFound = false;
	int readRecordFlag = SettingsManager::getIntSetting( "recordGame",
														 &recordFound );

	char recordGame = false;

	if( recordFound && readRecordFlag == 1 ) {
		recordGame = true;
	}


	int speedControlKeysFlag =
			SettingsManager::getIntSetting( "enableSpeedControlKeys", 0 );

	if( speedControlKeysFlag == 1 ) {
		enableSpeedControlKeys = true;
	}



	int outputAllFramesFlag =
			SettingsManager::getIntSetting( "outputAllFrames", 0 );

	if( outputAllFramesFlag == 1 ) {
		outputAllFrames = true;
		// start with very first frame
		shouldTakeScreenshot = true;

		screenShotPrefix = stringDuplicate( "frame" );
	}

	int blendOutputFramePairsFlag =
			SettingsManager::getIntSetting( "blendOutputFramePairs", 0 );

	if( blendOutputFramePairsFlag == 1 ) {
		blendOutputFramePairs = true;
	}

	blendOutputFrameFraction =
			SettingsManager::getFloatSetting( "blendOutputFrameFraction", 0.0f );

	webProxy = SettingsManager::getStringSetting( "webProxy" );

	if( webProxy != NULL &&
		strcmp( webProxy, "" ) == 0 ) {

		delete [] webProxy;
		webProxy = NULL;
	}


	// make sure dir is writeable
	FILE *testFile = fopen( "testWrite.txt", "w" );

	if( testFile == NULL ) {
		writeFailed = true;
	}
	else {
		fclose( testFile );

		remove( "testWrite.txt" );

		writeFailed = false;
	}


	// don't try to record games if we can't write to dir
	// can cause a crash.
	if( writeFailed ) {
		recordGame = false;
	}






	char *customData = getCustomRecordedGameData();

	char *hashSalt = getHashSalt();

	screen =
			new ScreenGL( screenWidth, screenHeight, fullscreen,
						  shouldNativeScreenResolutionBeUsed(),
						  targetFrameRate,
						  recordGame,
						  customData,
						  hashSalt,
						  getWindowTitle(), NULL, NULL, NULL );

	delete [] customData;
	delete [] hashSalt;


	// may change if specified resolution is not supported
	// or for event playback mode
	screenWidth = screen->getWidth();
	screenHeight = screen->getHeight();
	targetFrameRate = screen->getMaxFramerate();




	// watch out for huge resolutions that make default SDL cursor
	// too small

	int forceBigPointer = SettingsManager::getIntSetting( "forceBigPointer",
														  0 );
	if( forceBigPointer ||
		screenWidth > 1920 || screenHeight > 1080 ) {
		// big cursor

		AppLog::info( "Trying to load pointer from graphics/bigPointer.tga" );


		Image *cursorImage = readTGAFile( "bigPointer.tga" );

		if( cursorImage != NULL ) {

			if( cursorImage->getWidth() == 32 &&
				cursorImage->getHeight() == 32 &&
				cursorImage->getNumChannels() == 4 ) {

				double *r = cursorImage->getChannel( 0 );
				double *a = cursorImage->getChannel( 3 );


				Uint8 data[4*32];
				Uint8 mask[4*32];
				int i = -1;

				for( int y=0; y<32; y++ ) {
					for( int x=0; x<32; x++ ) {
						int p = y * 32 + x;

						if ( x % 8 ) {
							data[i] <<= 1;
							mask[i] <<= 1;
						}
						else {
							i++;
							data[i] = mask[i] = 0;
						}

						if( a[p] == 1 ) {
							if( r[p] == 0 ) {
								data[i] |= 0x01;
							}
							mask[i] |= 0x01;
						}
					}
				}

				// hot in upper left corner, (0,0)
				ourCursor =
						SDL_CreateCursor( data, mask, 32, 32, 0, 0 );

				SDL_SetCursor( ourCursor );
			}
			else {
				AppLog::error(
						"bigPointer.tga is not a 32x32 4-channel image." );

			}

			delete cursorImage;
		}
		else {

			AppLog::error( "Failed to read bigPointer.tga" );
		}
	}













	// adjust gameWidth to match available screen space
	// keep gameHeight constant


	/*
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY,
						 SDL_DEFAULT_REPEAT_INTERVAL );
	*/

	// never cut off top/bottom of image, and always try to use largest
	// whole multiples of screen pixels per game pixel to fill
	// the screen vertically as well as we can
	pixelZoomFactor = screenHeight / gameHeight;

	if( pixelZoomFactor < 1 ) {
		pixelZoomFactor = 1;
	}


	if( ! isNonIntegerScalingAllowed() ) {

		// make sure game width fills the screen at this pixel zoom,
		// even if game
		// height does not (letterbox on top/bottom, but never on left/rigtht)

		// closest number of whole pixels
		// may be *slight* black bars on left/right
		gameWidth = screenWidth / pixelZoomFactor;

		screen->setImageSize( pixelZoomFactor * gameWidth,
							  pixelZoomFactor * gameHeight );
	}
	else {

		pixelZoomFactor = 1;

		double targetAspectRatio = (double)gameWidth / (double)gameHeight;

		double screenAspectRatio = (double)screenWidth / (double)screenHeight;

		int imageW = screenWidth;
		int imageH = screenHeight;

		if( screenAspectRatio > targetAspectRatio ) {
			// screen too wide

			imageW = (int)( targetAspectRatio * imageH );
		}
		else if( screenAspectRatio < targetAspectRatio ) {
			// too tall

			imageH = (int)( imageW / targetAspectRatio );
		}

		screen->setImageSize( imageH,
							  imageW );
	}


	screen->allowSlowdownKeysDuringPlayback( enableSpeedControlKeys );

	//SDL_ShowCursor( SDL_DISABLE );


	sceneHandler = new GameSceneHandler( screen );



	// also do file-dependent part of init for GameSceneHandler here
	// actually, constructor is file dependent anyway.
	sceneHandler->initFromFiles();


	// hard to quit mode?
	char hardToQuitFound = false;
	int readHardToQuit = SettingsManager::getIntSetting( "hardToQuitMode",
														 &hardToQuitFound );

	if( readHardToQuit == 1 ) {
		hardToQuitMode = true;
	}

	pauseOnMinimize = SettingsManager::getIntSetting( "pauseOnMinimize", 1 );



	// translation language
	File *languageNameFile = new File( NULL, "language.txt" );

	if( languageNameFile->exists() ) {
		char *languageNameText = languageNameFile->readFileContents();

		SimpleVector<char *> *tokens = tokenizeString( languageNameText );

		int numTokens = tokens->size();

		// first token is name
		if( numTokens > 0 ) {
			char *languageName = *( tokens->getElement( 0 ) );

			TranslationManager::setLanguage( languageName, true );

			// augment translation by adding other languages listed
			// to fill in missing keys in top-line language

			for( int i=1; i<numTokens; i++ ) {
				languageName = *( tokens->getElement( i ) );

				TranslationManager::setLanguage( languageName, false );
			}
		}
		else {
			// default

			// TranslationManager already defaults to English, but
			// it looks for the language files at runtime before we have set
			// the current working directory.

			// Thus, we specify the default again here so that it looks
			// for its language files again.
			TranslationManager::setLanguage( "English", true );
		}

		delete [] languageNameText;

		for( int t=0; t<numTokens; t++ ) {
			delete [] *( tokens->getElement( t ) );
		}
		delete tokens;
	}

	delete languageNameFile;




	// register cleanup function, since screen->start() will never return
	atexit( cleanUpAtExit );




	screen->switchTo2DMode();



	if( getUsesSound() ) {

		soundSampleRate =
				SettingsManager::getIntSetting( "soundSampleRate", 22050 );

		int requestedBufferSize =
				SettingsManager::getIntSetting( "soundBufferSize", 512 );

		// 1 second fade in/out
		soundLoudnessIncrementPerSample = 1.0f / soundSampleRate;

		// force user-specified value to closest (round up) power of 2
		int bufferSize = 2;
		while( bufferSize < requestedBufferSize ) {
			bufferSize *= 2;
		}


		SDL_AudioSpec audioFormat;

		/* Set 16-bit stereo audio at 22Khz */
		audioFormat.freq = soundSampleRate;
		audioFormat.format = AUDIO_S16;
		audioFormat.channels = 2;
		//audioFormat.samples = 512;        /* A good value for games */
		audioFormat.samples = bufferSize;
		audioFormat.callback = audioCallback;
		audioFormat.userdata = NULL;

		SDL_AudioSpec actualFormat;


		int recordAudioFlag =
				SettingsManager::getIntSetting( "recordAudio", 0 );
		int recordAudioLengthInSeconds =
				SettingsManager::getIntSetting( "recordAudioLengthInSeconds", 0 );



		SDL_LockAudio();

		int openResult = 0;

		if( ! recordAudioFlag ) {
			openResult = SDL_OpenAudio( &audioFormat, &actualFormat );
		}



		/* Open the audio device and start playing sound! */
		if( openResult < 0 ) {
			AppLog::getLog()->logPrintf(
					Log::ERROR_LEVEL,
					"Unable to open audio: %s\n", SDL_GetError() );
			soundRunning = false;
			soundOpen = false;
		}
		else {

			if( !recordAudioFlag &&
				( actualFormat.format != AUDIO_S16 ||
				  actualFormat.channels != 2 ) ) {


				AppLog::getLog()->logPrintf(
						Log::ERROR_LEVEL,
						"Able to open audio, "
						"but stereo S16 samples not availabl\n");

				SDL_CloseAudio();
				soundRunning = false;
				soundOpen = false;
			}
			else {

				int desiredRate = soundSampleRate;

				if( !recordAudioFlag ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Successfully opened audio: %dHz (requested %dHz), "
							"sample buffer size=%d (requested %d)\n",
							actualFormat.freq, desiredRate, actualFormat.samples,
							bufferSize );

					// tell game what their buffer size will be
					// so they can allocate it outside the callback
					hintBufferSize( actualFormat.samples * 4 );
					bufferSizeHinted = true;
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Successfully faked opening of audio for "
							"recording to file: %dHz\n", desiredRate );
					// don't hint buffer size yet
					bufferSizeHinted = false;
				}




				soundSpriteNoClip =
						resetAudioNoClip(
								( 1.0 - soundSpriteCompressionFraction ) *
								maxTotalSoundSpriteVolume * 32767,
								// half second hold and release
								soundSampleRate / 2, soundSampleRate / 2 );

				totalAudioMixNoClip =
						resetAudioNoClip( 32767.0,
								// 10x faster hold and release
								// for master mix
								// pull music and sound effects down
								// to prevent clipping, but bring
								// it right back up again quickly
								// after the transient passes to
								// avoid audible pumping in music
										  soundSampleRate / 20,
										  soundSampleRate / 20 );



				if( !recordAudioFlag ) {
					soundSampleRate = actualFormat.freq;

					soundSpriteMixingBufferL =
							new double[ actualFormat.samples ];
					soundSpriteMixingBufferR =
							new double[ actualFormat.samples ];
				}


				soundRunning = true;
				soundOpen = true;

				if( recordAudioFlag ) {
					soundOpen = false;
				}

				if( recordAudioFlag == 1 && recordAudioLengthInSeconds > 0 ) {
					recordAudio = true;

					samplesLeftToRecord =
							recordAudioLengthInSeconds * soundSampleRate;

					aiffOutFile = fopen( "recordedAudio.aiff", "wb" );

					int headerLength;

					unsigned char *header =
							getAIFFHeader( 2, 16,
										   soundSampleRate,
										   samplesLeftToRecord,
										   &headerLength );

					fwrite( header, 1, headerLength, aiffOutFile );

					delete [] header;
				}



			}
		}

		SDL_UnlockAudio();
	}






	if( ! writeFailed ) {
		demoMode = isDemoMode();
	}



	initDrawString( pixelZoomFactor * gameWidth,
					pixelZoomFactor * gameHeight );



	//glLineWidth( pixelZoomFactor );


	if( demoMode ) {
		showDemoCodePanel( screen, getFontTGAFileName(), gameWidth,
						   gameHeight );

		// wait to start handling events
		// wait to start recording/playback
	}
	else if( writeFailed ) {
		// handle key events right away to listen for ESC
		screen->addKeyboardHandler( sceneHandler );
	}
	else {
		// handle events right away
		screen->addMouseHandler( sceneHandler );
		screen->addKeyboardHandler( sceneHandler );

		if( screen->isPlayingBack() ) {

			// start playback right away
			screen->startRecordingOrPlayback();

			AppLog::infoF( "Using frame rate from recording file:  %d fps\n",
						   screen->getMaxFramerate() );
		}
		// else wait to start recording until after we've measured
		// frame rate
	}


	int readTarget = SettingsManager::getIntSetting( "targetFrameRate", -1 );
	int readCounting = SettingsManager::getIntSetting( "countingOnVsync", -1 );

	if( readTarget != -1 && readCounting != -1 ) {
		targetFrameRate = readTarget;
		countingOnVsync = readCounting;

		screen->setFullFrameRate( targetFrameRate );
		screen->useFrameSleep( !countingOnVsync );
		screen->startRecordingOrPlayback();

		if( screen->isPlayingBack() ) {
			screen->useFrameSleep( true );
		}
		measureFrameRate = false;
	}


	// default texture mode
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );


	screen->start();


	return 0;
}




GameSceneHandler::GameSceneHandler( ScreenGL *inScreen )
		: mScreen( inScreen ),
		  mPaused( false ),
		  mPausedDuringFrameBatch( true ),
		  mLoadingDuringFrameBatch( true ),
		  mPausedSleepTime( 0 ),
		  mBlockQuitting( false ),
		  mLastFrameRate( targetFrameRate ),
		  mStartTimeSeconds( Time::timeSec() ),
		  mPrintFrameRate( true ),
		  mNumFrames( 0 ), mFrameBatchSize( 100 ),
		  mFrameBatchStartTimeSeconds( -1 ),
		  mBackgroundColor( 0, 0, 0, 1 ) {


	glClearColor( mBackgroundColor.r,
				  mBackgroundColor.g,
				  mBackgroundColor.b,
				  mBackgroundColor.a );


	// set external pointer so it can be used in calls below
	sceneHandler = this;


	mScreen->addSceneHandler( sceneHandler );
	mScreen->addRedrawListener( sceneHandler );
}



GameSceneHandler::~GameSceneHandler() {
	mScreen->removeMouseHandler( this );
	mScreen->removeSceneHandler( this );
	mScreen->removeRedrawListener( this );

	if( demoMode ) {
		// panel has not freed itself yet
		freeDemoCodePanel();

		demoMode = false;
	}


	if( loadingFailedMessage != NULL ) {
		delete [] loadingFailedMessage;
		loadingFailedMessage = NULL;
	}


}




void GameSceneHandler::initFromFiles() {

}




static float viewCenterX = 0;
static float viewCenterY = 0;
// default -1 to +1
static float viewSize = 2;

static float visibleWidth = -1;
static float visibleHeight = -1;


static void redoDrawMatrix() {
	// viewport square centered on screen (even if screen is rectangle)
	float hRadius = viewSize / 2;

	float wRadius = hRadius;

	if( visibleHeight > 0 ) {
		wRadius = visibleWidth / 2;
		hRadius = visibleHeight / 2;
	}


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( viewCenterX - wRadius, viewCenterX + wRadius,
			 viewCenterY - hRadius, viewCenterY + hRadius, -1.0f, 1.0f);

	if( visibleHeight > 0 ) {

		float portWide = screenWidth;
		float portHigh = ( visibleHeight / visibleWidth ) * portWide;

		float screenHeightFraction = (float) screenHeight / (float) screenWidth;

		if( screenHeightFraction < 9.0 / 16.0 ) {
			// wider than 16:9

			// fill vertically instead
			portHigh = screenHeight;

			portWide = ( visibleWidth / visibleHeight ) * portHigh;
		}

		float excessW = screenWidth - portWide;
		float excessH = screenHeight - portHigh;

		glViewport( excessW / 2,
					excessH / 2,
					portWide,
					portHigh );
	}

	glMatrixMode(GL_MODELVIEW);
}



unsigned int getRandSeed() {
	return screen->getRandSeed();
}



void pauseGame() {
	sceneHandler->mPaused = !( sceneHandler->mPaused );
}


char isPaused() {
	return sceneHandler->mPaused;
}



void blockQuitting( char inNoQuitting ) {
	sceneHandler->mBlockQuitting = inNoQuitting;
}



char isQuittingBlocked() {
	return sceneHandler->mBlockQuitting;
}



void wakeUpPauseFrameRate() {
	sceneHandler->mPausedSleepTime = 0;
}


// returns true if we're currently executing a recorded game
char isGamePlayingBack() {
	return screen->isPlayingBack();
}







void mapKey( unsigned char inFromKey, unsigned char inToKey ) {
	screen->setKeyMapping( inFromKey, inToKey );
}


void toggleKeyMapping( char inMappingOn ) {
	screen->toggleKeyMapping( inMappingOn );
}





void setViewCenterPosition( float inX, float inY ) {
	viewCenterX = inX;
	viewCenterY = inY;
	redoDrawMatrix();
}



doublePair getViewCenterPosition() {
	doublePair p = { viewCenterX, viewCenterY };

	return p;
}



void setViewSize( float inSize ) {
	viewSize = inSize;
	redoDrawMatrix();
}


void setLetterbox( float inVisibleWidth, float inVisibleHeight ) {
	visibleWidth = inVisibleWidth;
	visibleHeight = inVisibleHeight;
}


void setCursorVisible( char inIsVisible ) {
	if( inIsVisible ) {
		SDL_ShowCursor( SDL_ENABLE );
	}
	else {
		SDL_ShowCursor( SDL_DISABLE );
	}
}



void setCursorMode( int inMode ) {
	SettingsManager::setSetting( "cursorMode", inMode );
	cursorMode = inMode;

	switch( cursorMode ) {
		case 0:
		case 2:
			setCursorVisible( true );
			break;
		case 1:
			setCursorVisible( false );
			break;
		default:
			setCursorVisible( true );
			break;
	}
}


int getCursorMode() {
	return cursorMode;
}



void setEmulatedCursorScale( double inScale ) {
	SettingsManager::setDoubleSetting( "emulatedCursorScale", inScale );
	emulatedCursorScale = inScale;
}



double getEmulatedCursorScale() {
	return emulatedCursorScale;
}




void grabInput( char inGrabOn ) {
	if( inGrabOn ) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
	}
}



void setMouseReportingMode( char inWorldCoordinates ) {
	mouseWorldCoordinates = inWorldCoordinates;
}



static int lastMouseX = 0;
static int lastMouseY = 0;
static int lastMouseDownX = 0;
static int lastMouseDownY = 0;

static char mouseDown = false;
static char mouseRightDown = false;
// start with last click expired
static int mouseDownSteps = 1000;


static char ignoreNextMouseEvent = false;
static int xCoordToIgnore, yCoordToIgnore;



static void warpMouseToScreenPos( int inX, int inY ) {
	if( inX == lastMouseX && inY == lastMouseY ) {
		// mouse already there, no need to warp
		// (and warping when already there may or may not generate
		//  an event on some platforms, which causes trouble when we
		//  try to ignore the event)
		return;
	}

	if( SDL_GetAppState() & SDL_APPINPUTFOCUS ) {

		if( frameDrawerInited ) {
			// not ignoring mouse events currently due to demo code panel
			// or loading message... frame drawer not inited yet
			ignoreNextMouseEvent = true;
			xCoordToIgnore = inX;
			yCoordToIgnore = inY;
		}

		SDL_WarpMouse( inX, inY );
	}
}




void warpMouseToCenter( int *outNewMouseX, int *outNewMouseY ) {
	*outNewMouseX = screenWidth / 2;
	*outNewMouseY = screenHeight / 2;

	warpMouseToScreenPos( *outNewMouseX, *outNewMouseY );
}



void warpMouseToWorldPos( float inX, float inY ) {
	int worldX, worldY;
	worldToScreen( inX, inY, &worldX, &worldY );
	warpMouseToScreenPos( worldX, worldY );
}





int numPixelsDrawn = 0;
extern int totalLoadedTextureBytes;





void saveFrameRateSettings() {
	SettingsManager::setSetting( "targetFrameRate", targetFrameRate );

	int settingValue = 0;
	if( countingOnVsync ) {
		settingValue = 1;
	}

	SettingsManager::setSetting( "countingOnVsync", settingValue );
}




void GameSceneHandler::drawScene() {
	numPixelsDrawn = 0;
	/*
	glClearColor( mBackgroundColor->r,
				  mBackgroundColor->g,
				  mBackgroundColor->b,
				  mBackgroundColor->a );
	*/




	// do this here, because it involves game_getCurrentTime() calls
	// which are recorded, and aren't available for playback in fireRedraw
	mNumFrames ++;

	if( mPrintFrameRate ) {

		if( mFrameBatchStartTimeSeconds == -1 ) {
			mFrameBatchStartTimeSeconds = game_getCurrentTime();
		}

		if( mNumFrames % mFrameBatchSize == 0 ) {
			// finished a batch

			double timeDelta =
					game_getCurrentTime() - mFrameBatchStartTimeSeconds;

			double actualFrameRate =
					(double)mFrameBatchSize / (double)timeDelta;

			//AppLog::getLog()->logPrintf(
			//    Log::DETAIL_LEVEL,
			printf(
					"Frame rate = %f frames/second\n", actualFrameRate );

			mFrameBatchStartTimeSeconds = game_getCurrentTime();

			// don't update reported framerate if this batch involved a pause
			// or if still loading
			if( !mPausedDuringFrameBatch && !mLoadingDuringFrameBatch ) {
				mLastFrameRate = actualFrameRate;
			}
			else {
				// consider measuring next frame batch
				// (unless a pause or loading occurs there too)
				mPausedDuringFrameBatch = false;
				mLoadingDuringFrameBatch = false;
			}
		}
	}





	redoDrawMatrix();


	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );


	if( demoMode ) {

		if( ! isDemoCodePanelShowing() ) {

			// stop demo mode when panel done
			demoMode = false;

			mScreen->addMouseHandler( this );
			mScreen->addKeyboardHandler( this );

			screen->startRecordingOrPlayback();
		}
	}
	else if( writeFailed ) {
		drawString( translate( "writeFailed" ), true );
	}
	else if( !screen->isPlayingBack() && measureFrameRate ) {
		if( !measureRecorded ) {
			screen->useFrameSleep( false );
		}


		if( numFramesSkippedBeforeMeasure < numFramesToSkipBeforeMeasure ) {
			numFramesSkippedBeforeMeasure++;

			drawString( translate( "measuringFPS" ), true );
		}
		else if( ! startMeasureTimeRecorded ) {
			startMeasureTime = Time::getCurrentTime();
			startMeasureTimeRecorded = true;

			drawString( translate( "measuringFPS" ), true );
		}
		else {

			numFramesMeasured++;

			double totalTime = Time::getCurrentTime() - startMeasureTime;

			double timePerFrame = totalTime / ( numFramesMeasured );

			double frameRate = 1 / timePerFrame;


			int closestTargetFrameRate = 0;
			double closestFPSDiff = 9999999;

			for( int i=0; i<possibleFrameRates.size(); i++ ) {

				int v = possibleFrameRates.getElementDirect( i );

				double diff = fabs( frameRate - v );

				if( diff < closestFPSDiff ) {
					closestTargetFrameRate = v;
					closestFPSDiff = diff;
				}
			}

			double overAllowFactor = 1.05;



			if( numFramesMeasured > 10 &&
				frameRate > overAllowFactor * closestTargetFrameRate ) {

				secondsToMeasure = warningSecondsToMeasure;
			}
			else {
				secondsToMeasure = noWarningSecondsToMeasure;
			}

			if( totalTime <= secondsToMeasure ) {
				char *message = autoSprintf( "%s\n%0.2f\nFPS",
											 translate( "measuringFPS" ),
											 frameRate );


				drawString( message, true );

				delete [] message;
			}

			if( totalTime > secondsToMeasure ) {

				if( ! measureRecorded ) {

					if( targetFrameRate == idealTargetFrameRate ) {
						// not invoking halfFrameRate

						AppLog::infoF( "Measured frame rate = %f fps\n",
									   frameRate );
						AppLog::infoF(
								"Closest possible frame rate = %d fps\n",
								closestTargetFrameRate );

						if( frameRate >
							overAllowFactor * closestTargetFrameRate ) {

							AppLog::infoF(
									"Vsync to enforce closested frame rate of "
									"%d fps doesn't seem to be in effect.\n",
									closestTargetFrameRate );

							AppLog::infoF(
									"Will sleep each frame to enforce desired "
									"frame rate of %d fps\n",
									idealTargetFrameRate );

							targetFrameRate = idealTargetFrameRate;

							screen->useFrameSleep( true );
							countingOnVsync = false;
						}
						else {
							AppLog::infoF(
									"Vsync seems to be enforcing an allowed frame "
									"rate of %d fps.\n", closestTargetFrameRate );

							targetFrameRate = closestTargetFrameRate;

							screen->useFrameSleep( false );
							countingOnVsync = true;
						}
					}
					else {
						// half frame rate must be set

						AppLog::infoF(
								"User has halfFrameRate set, so we're going "
								"to manually sleep to enforce a target "
								"frame rate of %d fps.\n", targetFrameRate );
						screen->useFrameSleep( true );
						countingOnVsync = false;
					}


					screen->setFullFrameRate( targetFrameRate );
					measureRecorded = true;
				}

				if( !countingOnVsync ) {
					// show warning message
					char *message =
							autoSprintf( "%s\n%s\n\n%s\n\n\n%s",
										 translate( "vsyncWarning" ),
										 translate( "vsyncWarning2" ),
										 translate( "vsyncWarning3" ),
										 translate( "vsyncContinueMessage" ) );
					drawString( message, true );

					delete [] message;
				}
				else {
					// auto-save it now
					saveFrameRateSettings();
					screen->startRecordingOrPlayback();
					measureFrameRate = false;
				}
			}
		}

		return;
	}
	else if( !loadingMessageShown ) {
		drawString( translate( "loading" ), true );

		loadingMessageShown = true;
	}
	else if( loadingFailedFlag ) {
		drawString( loadingFailedMessage, true );
	}
	else if( !writeFailed && !loadingFailedFlag && !frameDrawerInited ) {
		drawString( translate( "loading" ), true );

		initFrameDrawer( pixelZoomFactor * gameWidth,
						 pixelZoomFactor * gameHeight,
						 targetFrameRate,
						 screen->getCustomRecordedGameData(),
						 screen->isPlayingBack() );

		int readCursorMode = SettingsManager::getIntSetting( "cursorMode", -1 );


		if( readCursorMode < 0 ) {
			// never set before

			// check if we are ultrawidescreen
			char ultraWide = false;

			const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

			int currentW = currentScreenInfo->current_w;
			int currentH = currentScreenInfo->current_h;

			double aspectRatio = (double)currentW / (double)currentH;

			// give a little wiggle room above 16:9
			// ultrawide starts at 21:9
			if( aspectRatio > 18.0 / 9.0 ) {
				ultraWide = true;
			}

			if( ultraWide ) {
				// drawn cursor, because system native cursor
				// is off-target on ultrawide displays

				setCursorMode( 1 );

				double startingScale = 1.0;

				int forceBigPointer =
						SettingsManager::getIntSetting( "forceBigPointer", 0 );
				if( forceBigPointer ||
					screenWidth > 1920 || screenHeight > 1080 ) {

					startingScale *= 2;
				}
				setEmulatedCursorScale( startingScale );
			}
		}
		else {
			setCursorMode( readCursorMode );

			double readCursorScale =
					SettingsManager::
					getDoubleSetting( "emulatedCursorScale", -1.0 );

			if( readCursorScale >= 1 ) {
				setEmulatedCursorScale( readCursorScale );
			}
		}



		frameDrawerInited = true;

		// this is a good time, a while after launch, to do the post
		// update step
		postUpdate();
	}
	else if( !writeFailed && !loadingFailedFlag  ) {
		// demo mode done or was never enabled

		// carry on with game


		// auto-pause when minimized
		if( pauseOnMinimize && screen->isMinimized() ) {
			mPaused = true;
		}


		// don't update while paused
		char update = !mPaused;

		drawFrame( update );

		if( cursorMode > 0 ) {
			// draw emulated cursor

			// draw using same projection used to drawFrame
			// so that emulated cursor lines up with screen position of buttons

			float xf, yf;
			screenToWorld( lastMouseX, lastMouseY, &xf, &yf );

			double x = xf;
			double y = yf;

			double sizeFactor = 25 * emulatedCursorScale;

			// white border of pointer

			setDrawColor( 1, 1, 1, 1 );

			double vertsA[18] =
					{ // body of pointer
							x, y,
							x, y - sizeFactor * 0.8918,
							x + sizeFactor * 0.6306, y - sizeFactor * 0.6306,
							// left collar of pointer
							x, y,
							x, y - sizeFactor * 1.0,
							x + sizeFactor * 0.2229, y - sizeFactor * 0.7994,
							// right collar of pointer
							x + sizeFactor * 0.4077, y - sizeFactor * 0.7229,
							x + sizeFactor * 0.7071, y - sizeFactor * 0.7071,
							x, y };

			drawTriangles( 3, vertsA );

			// neck of pointer
			double vertsB[8] = {
					x + sizeFactor * 0.2076, y - sizeFactor * 0.7625,
					x + sizeFactor * 0.376, y - sizeFactor * 1.169,
					x + sizeFactor * 0.5607, y - sizeFactor * 1.0924,
					x + sizeFactor * 0.3924, y - sizeFactor * 0.6859 };

			drawQuads( 1, vertsB );


			// black fill of pointer
			setDrawColor( 0, 0, 0, 1 );

			double vertsC[18] =
					{ // body of pointer
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966,
							x + sizeFactor * 0.04, y - sizeFactor * 0.814,
							x + sizeFactor * 0.5473, y - sizeFactor * 0.6038,
							// left collar of pointer
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966,
							x + sizeFactor * 0.04, y - sizeFactor * 0.9102,
							x + sizeFactor * 0.2382, y - sizeFactor * 0.7319,
							// right collar of pointer
							x + sizeFactor * 0.3491, y - sizeFactor * 0.6859,
							x + sizeFactor * 0.6153, y - sizeFactor * 0.6719,
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966 };

			drawTriangles( 3, vertsC );

			// neck of pointer
			double vertsD[8] = {
					x + sizeFactor * 0.2229, y - sizeFactor * 0.6949,
					x + sizeFactor * 0.3976, y - sizeFactor * 1.1167,
					x + sizeFactor * 0.5086, y - sizeFactor * 1.0708,
					x + sizeFactor * 0.3338, y - sizeFactor * 0.649 };

			drawQuads( 1, vertsD );
		}


		if( recordAudio ) {
			// frame-accurate audio recording
			int samplesPerFrame = soundSampleRate / targetFrameRate;

			// stereo 16-bit
			int bytesPerSample = 4;

			int numSampleBytes = bytesPerSample * samplesPerFrame;

			Uint8 *bytes = new Uint8[ numSampleBytes ];

			if( !bufferSizeHinted ) {
				hintBufferSize( numSampleBytes );

				soundSpriteMixingBufferL = new double[ samplesPerFrame ];
				soundSpriteMixingBufferR = new double[ samplesPerFrame ];

				bufferSizeHinted = true;
			}

			audioCallback( NULL, bytes, numSampleBytes );

			delete [] bytes;
		}


		if( screen->isPlayingBack() && screen->shouldShowPlaybackDisplay() ) {

			char *progressString = autoSprintf(
					"%s %.1f\n%s\n%s",
					translate( "playbackTag" ),
					screen->getPlaybackDoneFraction() * 100,
					translate( "playbackToggleMessage" ),
					translate( "playbackEndMessage" ) );

			drawString( progressString );

			delete [] progressString;

		}


		if( screen->isPlayingBack() &&
			screen->shouldShowPlaybackDisplay() &&
			showMouseDuringPlayback() ) {


			// draw mouse position info

			if( mouseDown ) {
				if( isLastMouseButtonRight() ) {
					mouseRightDown = true;
					setDrawColor( 1, 0, 1, 0.5 );
				}
				else {
					mouseRightDown = false;
					setDrawColor( 1, 0, 0, 0.5 );
				}
			}
			else {
				setDrawColor( 1, 1, 1, 0.5 );
			}

			// step mouse click animation even after mouse released
			// (too hard to see it otherwise for fast clicks)
			mouseDownSteps ++;


			float sizeFactor = 5.0f;
			float clickSizeFactor = 5.0f;
			char showClick = false;
			float clickFade = 1.0f;

			int mouseClickDisplayDuration = 20 * targetFrameRate / 60.0;

			if( mouseDownSteps < mouseClickDisplayDuration ) {

				float mouseClickProgress =
						mouseDownSteps / (float)mouseClickDisplayDuration;

				clickSizeFactor *= 5 * mouseClickProgress;
				showClick = true;

				clickFade *= 1.0f - mouseClickProgress;
			}


			// mouse coordinates in screen space
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();


			// viewport is square of largest dimension, centered on screen

			int bigDimension = screenWidth;

			if( screenHeight > bigDimension ) {
				bigDimension = screenHeight;
			}

			float excessX = ( bigDimension - screenWidth ) / 2;
			float excessY = ( bigDimension - screenHeight ) / 2;

			glOrtho( -excessX, -excessX + bigDimension,
					 -excessY + bigDimension, -excessY,
					 -1.0f, 1.0f );

			glViewport( -excessX,
						-excessY,
						bigDimension,
						bigDimension );

			glMatrixMode(GL_MODELVIEW);


			double verts[8] =
					{lastMouseX - sizeFactor, lastMouseY - sizeFactor,
					 lastMouseX - sizeFactor, lastMouseY + sizeFactor,
					 lastMouseX + sizeFactor, lastMouseY + sizeFactor,
					 lastMouseX + sizeFactor, lastMouseY - sizeFactor};

			drawQuads( 1, verts );


			double centerSize = 2;


			if( showClick ) {
				double clickVerts[8] =
						{lastMouseDownX - clickSizeFactor,
						 lastMouseDownY - clickSizeFactor,
						 lastMouseDownX - clickSizeFactor,
						 lastMouseDownY + clickSizeFactor,
						 lastMouseDownX + clickSizeFactor,
						 lastMouseDownY + clickSizeFactor,
						 lastMouseDownX + clickSizeFactor,
						 lastMouseDownY - clickSizeFactor};

				if( mouseDown ) {
					if( isLastMouseButtonRight() ) {
						setDrawColor( 1, 0, 1, clickFade );
					}
					else {
						setDrawColor( 1, 0, 0, clickFade );
					}
				}
				else {
					if( mouseRightDown ) {
						setDrawColor( 1, 1, 0, clickFade );
					}
					else {
						setDrawColor( 0, 1, 0, clickFade );
					}
				}

				drawQuads( 1, clickVerts );

				// draw pin-point at center of click
				double clickCenterVerts[8] =
						{lastMouseDownX - centerSize,
						 lastMouseDownY - centerSize,
						 lastMouseDownX - centerSize,
						 lastMouseDownY + centerSize,
						 lastMouseDownX + centerSize,
						 lastMouseDownY + centerSize,
						 lastMouseDownX + centerSize,
						 lastMouseDownY - centerSize};

				drawQuads( 1, clickCenterVerts );
			}


			// finally, darker black center over whole thing
			double centerVerts[8] =
					{lastMouseX - centerSize, lastMouseY - centerSize,
					 lastMouseX - centerSize, lastMouseY + centerSize,
					 lastMouseX + centerSize, lastMouseY + centerSize,
					 lastMouseX + centerSize, lastMouseY - centerSize};

			setDrawColor( 0, 0, 0, 0.5 );
			drawQuads( 1, centerVerts );

		}

	}


	if( visibleWidth != -1 && visibleHeight != -1 ) {
		// draw letterbox

		// On most platforms, glViewport will clip image for us.
		// glScissor is also supposed to do this, but it is buggy on some
		// platforms
		// thus, to be safe, we keep glScissor off and manually draw letterboxes
		// just in case glViewport doesn't clip the image.

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();


		// viewport is square of largest dimension, centered on screen

		int bigDimension = screenWidth;

		if( screenHeight > bigDimension ) {
			bigDimension = screenHeight;
		}

		float excessX = ( bigDimension - screenWidth ) / 2;
		float excessY = ( bigDimension - screenHeight ) / 2;

		glOrtho( -excessX, -excessX + bigDimension,
				 -excessY + bigDimension, -excessY,
				 -1.0f, 1.0f );

		glViewport( -excessX,
					-excessY,
					bigDimension,
					bigDimension );

		glMatrixMode(GL_MODELVIEW);

		setDrawColor( 0, 0, 0, 1.00 );

		int extraWidth =
				lrint(
						screenWidth -
						( (double)visibleWidth / (double)visibleHeight ) *
						screenHeight );

		if( extraWidth > 0 ) {


			// left/right bars
			drawRect( 0,
					  0,
					  extraWidth / 2,
					  screenHeight );

			drawRect( screenWidth - extraWidth / 2,
					  0,
					  screenWidth,
					  screenHeight );
		}
		else {

			int extraHeight =
					lrint(
							screenHeight -
							( (double)visibleHeight / (double)visibleWidth ) *
							screenWidth );

			if( extraHeight > 0 ) {

				// top/bottom bars

				drawRect( 0,
						  0,
						  screenWidth,
						  extraHeight / 2 );

				drawRect( 0,
						  screenHeight - extraHeight / 2,
						  screenWidth,
						  screenHeight );
			}
		}
	}


	if( shouldTakeScreenshot ) {
		takeScreenShot();

		if( !outputAllFrames ) {
			// just one
			shouldTakeScreenshot = false;
		}
		manualScreenShot = false;
	}

	frameNumber ++;
	//printf( "%d pixels drawn (%.2F MB textures resident)\n",
	//        numPixelsDrawn, totalLoadedTextureBytes / ( 1024.0 * 1024.0 ) );
}


void screenToWorld( int inX, int inY, float *outX, float *outY ) {

	if( mouseWorldCoordinates ) {

		// relative to center,
		// viewSize spreads out across screenWidth only (a square on screen)
		float x = (float)( inX - (screenWidth/2) ) / (float)screenWidth;
		float y = -(float)( inY - (screenHeight/2) ) / (float)screenWidth;

		*outX = x * viewSize + viewCenterX;
		*outY = y * viewSize + viewCenterY;
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}

}


void worldToScreen( float inX, float inY, int *outX, int *outY ) {
	if( mouseWorldCoordinates ) {
		// inverse of screenToWorld
		inX -= viewCenterX;
		inX /= viewSize;

		inX *= screenWidth;
		inX += screenWidth/2;

		*outX = round( inX );


		inY -= viewCenterY;
		inY /= viewSize;

		inY *= -screenWidth;
		inY += screenHeight/2;

		*outY = round( inY );
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}
}






void getLastMouseScreenPos( int *outX, int *outY ) {
	*outX = lastMouseX;
	*outY = lastMouseY;
}



void GameSceneHandler::mouseMoved( int inX, int inY ) {
	if( ignoreNextMouseEvent ) {
		if( inX == xCoordToIgnore && inY == yCoordToIgnore ) {
			// seeing the event that triggered the ignore
			ignoreNextMouseEvent = false;
			return;
		}
		else {
			// stale pending event before the ignore
			// skip it too
			return;
		}
	}

	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerMove( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
}



void GameSceneHandler::mouseDragged( int inX, int inY ) {
	if( ignoreNextMouseEvent ) {
		if( inX == xCoordToIgnore && inY == yCoordToIgnore ) {
			// seeing the event that triggered the ignore
			ignoreNextMouseEvent = false;
			return;
		}
		else {
			// stale pending event before the ignore
			// skip it too
			return;
		}
	}

	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerDrag( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
}




void GameSceneHandler::mousePressed( int inX, int inY ) {
	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerDown( x, y );

	lastMouseX = inX;
	lastMouseY = inY;

	mouseDown = true;
	mouseDownSteps = 0;
	lastMouseDownX = inX;
	lastMouseDownY = inY;
}



void GameSceneHandler::mouseReleased( int inX, int inY ) {
	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerUp( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
	mouseDown = false;

	// start new animation for release
	mouseDownSteps = 0;
	lastMouseDownX = inX;
	lastMouseDownY = inY;
}



void GameSceneHandler::fireRedraw() {

	if( !loadingDone ) {
		mLoadingDuringFrameBatch = true;
	}

	if( mPaused ) {
		// ignore redraw event

		mPausedDuringFrameBatch = true;

		if( mPausedSleepTime > (unsigned int)( 5 * targetFrameRate ) ) {
			// user has touched nothing for 5 seconds

			// sleep to avoid wasting CPU cycles
			Thread::staticSleep( 500 );
		}

		mPausedSleepTime++;

		return;
	}


}



static unsigned char lastKeyPressed = '\0';


void GameSceneHandler::keyPressed(
		unsigned char inKey, int inX, int inY ) {

	if( writeFailed || loadingFailedFlag ) {
		exit( 0 );
	}

	if( measureFrameRate && measureRecorded ) {
		if( inKey == 'y' || inKey == 'Y' ) {
			saveFrameRateSettings();
			screen->startRecordingOrPlayback();
			measureFrameRate = false;
		}
		else if( inKey == 27 ) {
			exit( 0 );
		}
	}



	// reset to become responsive while paused
	mPausedSleepTime = 0;


	if( mPaused && inKey == '%' && ! mBlockQuitting ) {
		// % to quit from pause
		exit( 0 );
	}


	if( inKey == 9 && isCommandKeyDown() &&
		screen->isPlayingBack() ) {

		printf( "Caught alt-tab during playback, pausing\n" );

		// alt-tab pressed during playback
		// but we aren't actually being minimized during playback
		// (because there's nothing to bring us back)
		// Still, force a pause, so that user's unpause action after
		// tabbing back in replays correctly
		mPaused = true;
	}


	if( enableSpeedControlKeys ) {

		if( inKey == '^' ) {
			// slow
			mScreen->setMaxFrameRate( 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '&' ) {
			// half
			mScreen->setMaxFrameRate( targetFrameRate / 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '*' ) {
			// normal
			mScreen->setMaxFrameRate( targetFrameRate );

			if( countingOnVsync ) {
				mScreen->useFrameSleep( false );
			}
			else {
				mScreen->useFrameSleep( true );
			}
		}
		if( inKey == '(' ) {
			// fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == ')' ) {
			// fast fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 4 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '-' ) {
			// fast fast fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 8 );
			mScreen->useFrameSleep( true );
		}
	}

	if( !hardToQuitMode ) {
		// escape only

		if( inKey == 27 ) {
			// escape always toggles pause
			mPaused = !mPaused;
		}
	}
	else {
		// # followed by ESC
		if( lastKeyPressed == '#' && inKey == 27 ) {
			exit( 0 );
		}
		lastKeyPressed = inKey;
	}

	keyDown( inKey );
}



void GameSceneHandler::keyReleased(
		unsigned char inKey, int inX, int inY ) {

	keyUp( inKey );
}



// need to track these separately from SDL_GetModState so that
// we replay isCommandKeyDown properly during recorded game playback
static char rCtrlDown = false;
static char lCtrlDown = false;

static char rAltDown = false;
static char lAltDown = false;

static char rMetaDown = false;
static char lMetaDown = false;

static char lShiftDown = false;
static char rShiftDown = false;



void GameSceneHandler::specialKeyPressed(
		int inKey, int inX, int inY ) {

	if( writeFailed || loadingFailedFlag ) {
		exit( 0 );
	}

	switch( inKey ) {
		case MG_KEY_RCTRL:
			rCtrlDown = true;
			break;
		case MG_KEY_LCTRL:
			lCtrlDown = true;
			break;
		case MG_KEY_RALT:
			rAltDown = true;
			break;
		case MG_KEY_LALT:
			lAltDown = true;
			break;
		case MG_KEY_RMETA:
			rMetaDown = true;
			break;
		case MG_KEY_LMETA:
			lMetaDown = true;
			break;
		case MG_KEY_RSHIFT:
			rShiftDown = true;
			break;
		case MG_KEY_LSHIFT:
			lShiftDown = true;
			break;
	}


	specialKeyDown( inKey );
}



void GameSceneHandler::specialKeyReleased(
		int inKey, int inX, int inY ) {


	switch( inKey ) {
		case MG_KEY_RCTRL:
			rCtrlDown = false;
			break;
		case MG_KEY_LCTRL:
			lCtrlDown = false;
			break;
		case MG_KEY_RALT:
			rAltDown = false;
			break;
		case MG_KEY_LALT:
			lAltDown = false;
			break;
		case MG_KEY_RMETA:
			rMetaDown = false;
			break;
		case MG_KEY_LMETA:
			lMetaDown = false;
			break;
		case MG_KEY_RSHIFT:
			rShiftDown = false;
			break;
		case MG_KEY_LSHIFT:
			lShiftDown = false;
			break;
	}


	specialKeyUp( inKey );
}



char isCommandKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_CTRL )
		||
		( modState & KMOD_ALT )
		||
		( modState & KMOD_META ) ) {

		return true;
	}

	if( screen->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rCtrlDown || lCtrlDown ||
			rAltDown || lAltDown ||
			rMetaDown || lMetaDown ) {
			return true;
		}
	}

	return false;
}



char isShiftKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_SHIFT ) ) {

		return true;
	}

	if( screen->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rShiftDown || lShiftDown ) {
			return true;
		}
	}

	return false;
}



char isLastMouseButtonRight() {
	return screen->isLastMouseButtonRight();
}


// FOVMOD NOTE:  Change 1/1 - Take these lines during the merge process
int getLastMouseButton() {
	return screen->getLastMouseButton();
}



void obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead ) {

	screen->obscureRecordedNumericTyping( inObscure, inCharToRecordInstead );
}




void GameSceneHandler::actionPerformed( GUIComponent *inTarget ) {
}



static Image *readTGAFile( File *inFile ) {

	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );

	TGAImageConverter converter;

	Image *result = converter.deformatImage( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}



Image *readTGAFile( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFile( &tgaFile );
}



Image *readTGAFileBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFile( &tgaFile );
}




static RawRGBAImage *readTGAFileRaw( InputStream *inStream ) {
	TGAImageConverter converter;

	RawRGBAImage *result = converter.deformatImageRaw( inStream );


	return result;
}




static RawRGBAImage *readTGAFileRaw( File *inFile ) {

	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );


	RawRGBAImage *result = readTGAFileRaw( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}



RawRGBAImage *readTGAFileRaw( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}



RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}




RawRGBAImage *readTGAFileRawFromBuffer( unsigned char *inBuffer,
										int inLength ) {

	ByteBufferInputStream tgaStream( inBuffer, inLength );

	return readTGAFileRaw( &tgaStream );
}





void writeTGAFile( const char *inTGAFileName, Image *inImage ) {
	File tgaFile( NULL, inTGAFileName );
	FileOutputStream tgaStream( &tgaFile );

	TGAImageConverter converter;

	return converter.formatImage( inImage, &tgaStream );
}



SpriteHandle fillSprite( RawRGBAImage *inRawImage ) {
	if( inRawImage->mNumChannels != 4 ) {
		printf( "Sprite not a 4-channel image, "
				"failed to load.\n" );

		return NULL;
	}

	return fillSprite( inRawImage->mRGBABytes,
					   inRawImage->mWidth,
					   inRawImage->mHeight );
}



SpriteHandle loadSprite( const char *inTGAFileName,
						 char inTransparentLowerLeftCorner ) {

	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRaw( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from graphics/%s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFile( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
										  inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}



SpriteHandle loadSpriteBase( const char *inTGAFileName,
							 char inTransparentLowerLeftCorner ) {
	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRawBase( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from %s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFileBase( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
										  inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}




const char *translate( const char *inTranslationKey ) {
	return TranslationManager::translate( inTranslationKey );
}



static Image **screenShotImageDest = NULL;


void saveScreenShot( const char *inPrefix, Image **outImage ) {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( inPrefix );
	shouldTakeScreenshot = true;
	manualScreenShot = true;

	screenShotImageDest = outImage;
}



void startOutputAllFrames() {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( "frame" );

	outputAllFrames = true;
	shouldTakeScreenshot = true;
}


void stopOutputAllFrames() {
	outputAllFrames = false;
	shouldTakeScreenshot = false;
}





// if manualScreenShot false, then any blend settings (for saving blended
// double-frames) are applied
// can return NULL in this case (when this frame should not be output
// according to blending settings)
//
// Region in screen pixels
static Image *getScreenRegionInternal(
		int inStartX, int inStartY, int inWidth, int inHeight,
		char inForceManual = false ) {

	int numBytes = inWidth * inHeight * 3;

	unsigned char *rgbBytes =
			new unsigned char[ numBytes ];

	// w and h might not be multiples of 4
	GLint oldAlignment;
	glGetIntegerv( GL_PACK_ALIGNMENT, &oldAlignment );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );

	glReadPixels( inStartX, inStartY, inWidth, inHeight,
				  GL_RGB, GL_UNSIGNED_BYTE, rgbBytes );

	glPixelStorei( GL_PACK_ALIGNMENT, oldAlignment );


	if( ! inForceManual &&
		! manualScreenShot &&
		blendOutputFramePairs &&
		frameNumber % 2 != 0 &&
		lastFrame_rgbaBytes != NULL ) {

		// save blended frames on odd frames
		if( blendOutputFrameFraction > 0 ) {
			float blendA = 1 - blendOutputFrameFraction;
			float blendB = blendOutputFrameFraction;

			for( int i=0; i<numBytes; i++ ) {
				rgbBytes[i] =
						(unsigned char)(
								blendA * rgbBytes[i] +
								blendB * lastFrame_rgbaBytes[i] );
			}
		}

	}
	else if( ! inForceManual &&
			 ! manualScreenShot &&
			 blendOutputFramePairs &&
			 frameNumber % 2 == 0 ) {

		// skip even frames, but save them for next blending

		if( lastFrame_rgbaBytes != NULL ) {
			delete [] lastFrame_rgbaBytes;
			lastFrame_rgbaBytes = NULL;
		}

		lastFrame_rgbaBytes = rgbBytes;

		return NULL;
	}


	Image *screenImage = new Image( inWidth, inHeight, 3, false );

	double *channelOne = screenImage->getChannel( 0 );
	double *channelTwo = screenImage->getChannel( 1 );
	double *channelThree = screenImage->getChannel( 2 );

	// image of screen is upside down
	int outputRow = 0;
	for( int y=inHeight - 1; y>=0; y-- ) {
		for( int x=0; x<inWidth; x++ ) {

			int outputPixelIndex = outputRow * inWidth + x;


			int regionPixelIndex = y * inWidth + x;
			int byteIndex = regionPixelIndex * 3;

			// optimization found:  should unroll this loop over 3 channels
			// divide by 255, with a multiply
			channelOne[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
			channelTwo[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
			channelThree[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
		}
		outputRow++;
	}

	delete [] rgbBytes;

	return screenImage;
}


Image *getScreenRegionRaw(
		int inStartX, int inStartY, int inWidth, int inHeight ) {

	return getScreenRegionInternal( inStartX, inStartY, inWidth, inHeight,
									true );
}





static int nextShotNumber = -1;
static char shotDirExists = false;

static int outputFrameCount = 0;


void takeScreenShot() {


	File shotDir( NULL, "screenShots" );

	if( !shotDirExists && !shotDir.exists() ) {
		shotDir.makeDirectory();
		shotDirExists = shotDir.exists();
	}

	if( nextShotNumber < 1 ) {
		if( shotDir.exists() && shotDir.isDirectory() ) {

			int numFiles;
			File **childFiles = shotDir.getChildFiles( &numFiles );

			nextShotNumber = 1;

			char *formatString = autoSprintf( "%s%%d.%s", screenShotPrefix,
											  screenShotExtension );

			for( int i=0; i<numFiles; i++ ) {

				char *name = childFiles[i]->getFileName();

				int number;

				int numRead = sscanf( name, formatString, &number );

				if( numRead == 1 ) {

					if( number >= nextShotNumber ) {
						nextShotNumber = number + 1;
					}
				}
				delete [] name;

				delete childFiles[i];
			}

			delete [] formatString;

			delete [] childFiles;
		}
	}


	if( nextShotNumber < 1 ) {
		return;
	}

	char *fileName = autoSprintf( "%s%05d.%s",
								  screenShotPrefix, nextShotNumber,
								  screenShotExtension );



	File *file = shotDir.getChildFile( fileName );

	delete [] fileName;


	if( outputAllFrames ) {
		printf( "Output Frame %d (%.2f sec)\n", outputFrameCount,
				outputFrameCount / (double) targetFrameRate );
		outputFrameCount ++;
	}


	Image *screenImage =
			getScreenRegionInternal( 0, 0, screenWidth, screenHeight );

	if( screenImage == NULL ) {
		// a skipped frame due to blending settings
		delete file;

		return;
	}





	if( screenShotImageDest != NULL ) {
		// skip writing to file
		*screenShotImageDest = screenImage;
	}
	else {
		FileOutputStream tgaStream( file );
		screenShotConverter.formatImage( screenImage, &tgaStream );
		delete screenImage;
	}

	delete file;

	nextShotNumber++;
}




Image *getScreenRegion( double inX, double inY,
						double inWidth, double inHeight ) {

	double endX = inX + inWidth;
	double endY = inY + inHeight;

	// rectangle specified in integer screen coordinates

	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	GLdouble winStartX, winStartY, winStartZ;
	GLdouble winEndX, winEndY, winEndZ;

	gluProject( inX, inY, 0,
				modelview, projection, viewport,
				&winStartX, &winStartY, &winStartZ );

	gluProject( endX, endY, 0,
				modelview, projection, viewport,
				&winEndX, &winEndY, &winEndZ );




	char oldManual = manualScreenShot;
	manualScreenShot = true;


	Image *result =
			getScreenRegionInternal(
					lrint( winStartX ), lrint( winStartY ),
					lrint( winEndX - winStartX ), lrint( winEndY - winStartY ) );

	manualScreenShot = oldManual;

	return result;
}






int nextWebRequestHandle = 0;




int startWebRequest( const char *inMethod, const char *inURL,
					 const char *inBody ) {

	WebRequestRecord r;

	r.handle = nextWebRequestHandle;
	nextWebRequestHandle ++;


	if( screen->isPlayingBack() ) {
		// stop here, don't actually start a real web request
		return r.handle;
	}


	r.request = new WebRequest( inMethod, inURL, inBody, webProxy );

	webRequestRecords.push_back( r );

	return r.handle;
}



static WebRequest *getRequestByHandle( int inHandle ) {
	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->request;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getRequestByHandle:  "
				   "Requested WebRequest handle not found\n" );
	return NULL;
}



int stepWebRequest( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// don't step request, because we're only simulating the response
		// of the server

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return 1;
		}
		else if( nextType == 1 ) {
			// recording said our result was ready
			// but it may not be the actual next result, due to timing
			// keep processing results until we see an actual 2 in the recording
			return 0;
		}
		else {
			return nextType;
		}
	}


	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {

		int stepResult = r->step();

		screen->registerWebEvent( inHandle, stepResult );

		return stepResult;
	}

	return -1;
}



// gets the response body as a \0-terminated string, destroyed by caller
char *getWebResult( int inHandle ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return screen->getWebEventResultBody( inHandle );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		char *result = r->getResult();

		if( result != NULL ) {
			screen->registerWebEvent( inHandle,
					// the type for "result" is 2
									  2,
									  result );
		}

		return result;
	}

	return NULL;
}




unsigned char *getWebResult( int inHandle, int *outSize ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return (unsigned char *)
					screen->getWebEventResultBody( inHandle, outSize );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		unsigned char *result = r->getResult( outSize );

		if( result != NULL ) {
			screen->registerWebEvent( inHandle,
					// the type for "result" is 2
									  2,
									  (char*)result,
									  *outSize );
		}

		return result;
	}

	return NULL;
}



int getWebProgressSize( int inHandle ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType > 2 ) {
			return nextType;
		}
		else {
			AppLog::error(
					"Expecting a web result progress event in playback file, "
					"but found none." );

			return 0;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		int progress = r->getProgressSize();
		if( progress > 2 ) {
			screen->registerWebEvent( inHandle,
					// the type for "progress" is
					// the actual size
									  progress );
			return progress;
		}
		else {
			// progress of 2 or less is returned as 0, to keep consistency
			// for recording and playback
			return 0;
		}
	}

	return 0;
}




// frees resources associated with a web request
// if request is not complete, this cancels it
// if hostname lookup is not complete, this call might block.
void clearWebRequest( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real request, do nothing
		return;
	}


	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->request;

			webRequestRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - clearWebRequest:  "
				   "Requested WebRequest handle not found\n" );
}





int nextSocketConnectionHandle = 0;



int openSocketConnection( const char *inNumericalAddress, int inPort ) {
	SocketConnectionRecord r;

	r.handle = nextSocketConnectionHandle;
	nextSocketConnectionHandle++;


	if( screen->isPlayingBack() ) {
		// stop here, don't actually open a real socket
		return r.handle;
	}

	HostAddress address( stringDuplicate( inNumericalAddress ), inPort );


	char timedOut;

	// non-blocking connet
	r.sock = SocketClient::connectToServer( &address, 0, &timedOut );

	if( r.sock != NULL ) {
		socketConnectionRecords.push_back( r );

		return r.handle;
	}
	else {
		return -1;
	}
}



static Socket *getSocketByHandle( int inHandle ) {
	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->sock;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getSocketByHandle:  "
				   "Requested Socket handle not found\n" );
	return NULL;
}





// non-blocking send
// returns number sent (maybe 0) on success, -1 on error
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength ) {
	if( screen->isPlayingBack() ) {
		// play back result of this send

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		while( nextType == 2 && nextNumBodyBytes == 0 ) {
			// skip over any lingering waiting-for-read events
			// sometimes there are extra in recording that aren't needed
			// on playback for some reason
			screen->getSocketEventTypeAndSize( inHandle,
											   &nextType, &nextNumBodyBytes );
		}


		if( nextType == 0 ) {
			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numSent = 0;


		if( sock->isConnected() ) {

			numSent = sock->send( inData, inDataLength, false, false );

			if( numSent == -2 ) {
				// would block
				numSent = 0;
			}
		}

		int type = 0;

		if( numSent == -1 ) {
			type = 1;
			numSent = 0;
		}

		screen->registerSocketEvent( inHandle, type, numSent, NULL );

		return numSent;
	}

	return -1;
}



// non-blocking read
// returns number of bytes read (maybe 0), -1 on error
int readFromSocket( int inHandle,
					unsigned char *inDataBuffer, int inBytesToRead ) {

	if( screen->isPlayingBack() ) {
		// play back result of this read

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		if( nextType == 2 ) {

			if( nextNumBodyBytes == 0 ) {
				return 0;
			}
			// else there are body bytes waiting

			if( nextNumBodyBytes > inBytesToRead ) {
				AppLog::errorF( "gameSDL - readFromSocket:  "
								"Expecting to read at most %d bytes, but "
								"recording has %d bytes waiting\n",
								inBytesToRead, nextNumBodyBytes );
				return -1;
			}

			unsigned char *bodyBytes =
					screen->getSocketEventBodyBytes( inHandle );

			memcpy( inDataBuffer, bodyBytes, nextNumBodyBytes );
			delete [] bodyBytes;


			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numRead = 0;

		if( sock->isConnected() ) {

			numRead = sock->receive( inDataBuffer, inBytesToRead, 0 );

			if( numRead == -2 ) {
				// would block
				numRead = 0;
			}
		}

		int type = 2;
		if( numRead == -1 ) {
			type = 3;
		}

		unsigned char *bodyBytes = NULL;
		if( numRead > 0 ) {
			bodyBytes = inDataBuffer;
		}

		screen->registerSocketEvent( inHandle, type, numRead, bodyBytes );

		return numRead;
	}

	return -1;
}



void closeSocket( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real socket, do nothing
		return;
	}

	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->sock;

			socketConnectionRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - closeSocket:  "
				   "Requested Socket handle not found\n" );
}









int startAsyncFileRead( const char *inFilePath ) {

	int handle = nextAsyncFileHandle;
	nextAsyncFileHandle ++;

	AsyncFileRecord r = {
			handle,
			stringDuplicate( inFilePath ),
			-1,
			NULL,
			false };

	asyncLock.lock();
	asyncFiles.push_back( r );
	asyncLock.unlock();

	newFileToReadSem.signal();

	return handle;
}



char checkAsyncFileReadDone( int inHandle ) {

	char ready = false;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle &&
			r->doneReading ) {

			ready = true;
			break;
		}
	}
	asyncLock.unlock();


	if( screen->isPlayingBack() ) {
		char playbackSaysReady = screen->getAsyncFileDone( inHandle );

		if( ready && playbackSaysReady ) {
			return true;
		}
		else if( ready && !playbackSaysReady ) {
			return false;
		}
		else if( ! ready && playbackSaysReady ) {
			// need to return ready before end of this frame
			// so behavior matches recording behavior

			// wait for read to finish, synchronously
			while( !ready ) {
				newFileDoneReadingSem.wait();

				asyncLock.lock();

				for( int i=0; i<asyncFiles.size(); i++ ) {
					AsyncFileRecord *r = asyncFiles.getElement( i );

					if( r->handle == inHandle &&
						r->doneReading ) {

						ready = true;
						break;
					}
				}
				asyncLock.unlock();
			}

			return true;
		}
	}
	else {
		if( ready ) {
			screen->registerAsyncFileDone( inHandle );
		}
	}


	return ready;
}



unsigned char *getAsyncFileData( int inHandle, int *outDataLength ) {

	unsigned char *data = NULL;

	asyncLock.lock();

	for( int i=0; i<asyncFiles.size(); i++ ) {
		AsyncFileRecord *r = asyncFiles.getElement( i );

		if( r->handle == inHandle ) {

			data = r->data;
			*outDataLength = r->dataLength;

			if( r->filePath != NULL ) {
				delete [] r->filePath;
			}

			asyncFiles.deleteElement( i );
			break;
		}
	}
	asyncLock.unlock();

	return data;
}







timeSec_t game_timeSec() {
	return screen->getTimeSec();
}



double game_getCurrentTime() {
	return screen->getCurrentTime();
}



double getRecentFrameRate() {
	if( screen->isPlayingBack() ) {

		return screen->getRecordedFrameRate();
	}
	else {
		screen->registerActualFrameRate( sceneHandler->mLastFrameRate );

		return sceneHandler->mLastFrameRate;
	}
}



void loadingFailed( const char *inFailureMessage ) {
	loadingFailedFlag = true;

	if( loadingFailedMessage != NULL ) {
		delete [] loadingFailedMessage;
	}
	loadingFailedMessage = stringDuplicate( inFailureMessage );
}



void loadingComplete() {
	loadingDone = true;
}


char getCountingOnVsync() {
	return countingOnVsync;
}



char isHardToQuitMode() {
	return hardToQuitMode;
}



// platform-specific clipboard code



#ifdef LINUX
static char clipboardSupportKnown = false;
static char clipboardSupport = false;
#endif


char isClipboardSupported() {
#ifdef LINUX
	// only check once, since system forks a process each time
    if( !clipboardSupportKnown ) {

        if( system( "which xclip > /dev/null 2>&1" ) ) {
            // xclip not installed
            AppLog::error( "xclip must be installed for clipboard to work" );
            clipboardSupport = false;
            }
        else {
            clipboardSupport = true;
            }
        clipboardSupportKnown = true;
        }
    return clipboardSupport;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}


char isURLLaunchSupported() {
#ifdef LINUX
	return true;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}









#ifdef LINUX
// X windows clipboard tutorial found here
// http://michael.toren.net/mirrors/doc/X-copy+paste.txt

// X11 has it's own Time type
// avoid conflicts with our Time class from above by replacing the word
// (Trick found here:  http://stackoverflow.com/questions/8865744 )
#define Time X11_Time

#include <X11/Xlib.h>
#include <X11/Xatom.h>


char *getClipboardText() {

    FILE* pipe = popen( "xclip -silent -selection clipboard -o", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    SimpleVector<char> textVector;

    char buffer[512];
    char *line = fgets( buffer, sizeof( buffer ), pipe );

    while( line != NULL ) {
        textVector.appendElementString( buffer );
        line = fgets( buffer, sizeof( buffer ), pipe );
        }

    pclose( pipe );


    return textVector.getElementString();
    }


void setClipboardText( const char *inText  ) {
    // x copy paste is a MESS
    // after claiming ownership of the clipboard, application needs
    // to listen to x events forever to handle any consumers of the clipboard
    // data.  Yuck!

    // farm this out to xclip with -silent flag
    // it forks its own process and keeps it live as long as the clipboard
    // data is still needed (kills itself when the clipboard is claimed
    // by someone else with new data)

    FILE* pipe = popen( "xclip -silent -selection clipboard -i", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }


void launchURL( char *inURL ) {
    char *call = autoSprintf( "xdg-open \"%s\" &", inURL );
    system( call );
    delete [] call;
    }




#elif defined(__mac__)

// pbpaste command line trick found here:
// http://www.alecjacobson.com/weblog/?p=2376

char *getClipboardText() {
    FILE* pipe = popen( "pbpaste", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    char buffer[ 128 ];

    char *result = stringDuplicate( "" );

    // read until pipe closed
    while( ! feof( pipe ) ) {
        if( fgets( buffer, 128, pipe ) != NULL ) {
            char *newResult = concatonate( result, buffer );
            delete [] result;
            result = newResult;
            }
        }
    pclose( pipe );


    return result;
    }



void setClipboardText( const char *inText  ) {
    FILE* pipe = popen( "pbcopy", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }



void launchURL( char *inURL ) {
    char *call = autoSprintf( "open \"%s\"", inURL );
    system( call );
    delete [] call;
    }




#elif defined(WIN_32)

// simple windows clipboard solution found here:
// https://www.allegro.cc/forums/thread/606034

#include <windows.h>

char *getClipboardText() {
    char *fromClipboard = NULL;
    if( OpenClipboard( NULL ) ) {
        HANDLE hData = GetClipboardData( CF_TEXT );
        char *buffer = (char*)GlobalLock( hData );
        if( buffer != NULL ) {
            fromClipboard = stringDuplicate( buffer );
            }
        GlobalUnlock( hData );
        CloseClipboard();
        }

    if( fromClipboard == NULL ) {
        fromClipboard = stringDuplicate( "" );
        }

    return fromClipboard;
    }


void setClipboardText( const char *inText  ) {
    if (OpenClipboard(NULL)) {

        EmptyClipboard();
        HGLOBAL clipBuffer = GlobalAlloc( GMEM_DDESHARE, strlen(inText) + 1 );
        char *buffer = (char*)GlobalLock( clipBuffer );

        strcpy( buffer, inText );
        GlobalUnlock( clipBuffer );
        SetClipboardData( CF_TEXT, clipBuffer );
        CloseClipboard();
        }
    }


void launchURL( char *inURL ) {
    // for some reason, on Windows, need extra set of "" before quoted URL
    // found here:
    // https://stackoverflow.com/questions/3037088/
    //         how-to-open-the-default-web-browser-in-windows-in-c

    // the wmic method allows spawning a browser without it lingering as
    // a child process
    // https://steamcommunity.com/groups/steamworks/
    //         discussions/0/154645427521397803/
    char *call = autoSprintf(
        "wmic process call create 'cmd /c start \"\" \"%s\"'", inURL );
    system( call );
    delete [] call;
    }



#else
// unsupported platform
char *getClipboardText() {
	return stringDuplicate( "" );
}

void launchURL( char *inURL ) {
}
#endif





#define macLaunchExtension ".app"
#define winLaunchExtension ".exe"

#define steamGateClientName "steamGateClient"


#ifdef LINUX

#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "./%s", getLinuxAppName() );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }

char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "./%s", steamGateClientName );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from launching steamGateClient\n" );
    return true;
    }


#elif defined(__mac__)

#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    // Gatekeeper on 10.12 prevents relaunch from working
    // to be safe, just have user manually relaunch on Mac
    return false;

    /*
    char *launchTarget =
        autoSprintf( "%s_$d%s", getAppName(),
                     getAppVersion(), macLaunchExtension );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[4] = { (char*)"open", (char*)"-n",
                               launchTarget, NULL };

        execvp( "open", arguments );
        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;

    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    */
    }


char runSteamGateClient() {
    // have never tested this on Mac, who knows?
    return false;
    }



#elif defined(WIN_32)

#include <windows.h>
#include <process.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "%s%s", getAppName(), winLaunchExtension );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }



char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "%s%s", steamGateClientName, winLaunchExtension );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from running steamGateClient\n" );
    return true;
    }


#else
// unsupported platform
char relaunchGame() {
	return false;
}

char runSteamGateClient() {
	return false;
}
#endif




void quitGame() {
	exit( 0 );
}




// true if platform supports sound recording, false otherwise
char isSoundRecordingSupported() {
#ifdef LINUX
	// check for arecord existence
    // The redirect to /dev/null ensures that your program does not produce
    // the output of these commands.
    // found here:
    // http://stackoverflow.com/questions/7222674/
    //             how-to-check-if-command-is-available-or-existant
    //
    int ret = system( "arecord --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
#elif defined(__mac__)
	return false;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}



#ifdef LINUX

static FILE *arecordPipe = NULL;
const char *arecordFileName = "inputSoundTemp.wav";
static int arecordSampleRate = 0;

// starts recording asynchronously
// keeps recording until stop called
char startRecording16BitMonoSound( int inSampleRate ) {
    if( arecordPipe != NULL ) {
        pclose( arecordPipe );
        arecordPipe = NULL;
        }

    arecordSampleRate = inSampleRate;

    char *arecordLine =
        autoSprintf( "arecord -f S16_LE -c1 -r%d %s",
                     inSampleRate, arecordFileName );

    arecordPipe = popen( arecordLine, "w" );

    delete [] arecordLine;

    if( arecordPipe == NULL ) {
        return false;
        }
    else {
        return true;
        }
    }



// returns array of samples destroyed by caller
int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    if( arecordPipe == NULL ) {
        return NULL;
        }

    // kill arecord to end the recording gracefully
    // this is reasonable to do because I can't imagine situations
    // where more than one arecord is running
    system( "pkill arecord" );

    pclose( arecordPipe );
    arecordPipe = NULL;

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }




#elif defined(__mac__)

const char *arecordFileName = "inputSound.wav";

// mac implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {
    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    *outNumSamples = 0;
    return NULL;
    }

#elif defined(WIN_32)

#include <mmsystem.h>

const char *arecordFileName = "inputSound.wav";
static int arecordSampleRate = 0;

// windows implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {

    arecordSampleRate = inSampleRate;

    if( mciSendString( "open new type waveaudio alias my_sound",
                       NULL, 0, 0 ) == 0 ) {

        char *settingsString =
            autoSprintf( "set my_sound alignment 2 bitspersample 16"
                         " samplespersec %d"
                         " channels 1"
                         " bytespersec %d"
                         " time format milliseconds format tag pcm",
                         inSampleRate,
                         ( 16 * inSampleRate ) / 8 );

        mciSendString( settingsString, NULL, 0, 0 );

        delete [] settingsString;

        mciSendString( "record my_sound", NULL, 0, 0 );
        return true;
        }

    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    mciSendString( "stop my_sound", NULL, 0, 0 );

    char *saveCommand = autoSprintf( "save my_sound %s", arecordFileName );

    mciSendString( saveCommand, NULL, 0, 0 );
    delete [] saveCommand;

    mciSendString( "close my_sound", NULL, 0, 0 );

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }

#else

const char *arecordFileName = "inputSound.wav";

// default implementation does nothing
char startRecording16BitMonoSound( int inSampleRate ) {
	return false;
}

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
	return NULL;
}

#endif



// same for all platforms
// load a .wav file
int16_t *load16BitMonoSound( int *outNumSamples, int *outSampleRate ) {

	File wavFile( NULL, arecordFileName );

	if( ! wavFile.exists() ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "File does not exist in game folder: %s\n",
						arecordFileName );
		return NULL;
	}

	char *fileName = wavFile.getFullFileName();

	FILE *file = fopen( fileName, "rb" );

	delete [] fileName;

	if( file == NULL ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to open sound file for reading: %s\n",
						arecordFileName );
		return NULL;
	}

	fseek( file, 0L, SEEK_END );
	int fileSize  = ftell( file );
	rewind( file );

	if( fileSize <= 44 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file too small to contain a WAV header: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	// skip 20 bytes of header to get to format flag
	fseek( file, 20, SEEK_SET );

	unsigned char readBuffer[4];

	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not in PCM format: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not  in mono: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}

	fread( readBuffer, 1, 4, file );

	// little endian
	*outSampleRate =
			(int)( readBuffer[3] << 24 |
				   readBuffer[2] << 16 |
				   readBuffer[1] << 8 |
				   readBuffer[0] );


	fseek( file, 34, SEEK_SET );


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 16 && readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not 16-bit: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}



	/*
	  // this is not reliable as arecord leaves this blank when
	  // recording a stream

	fseek( file, 40, SEEK_SET );


	fread( readBuffer, 1, 4, file );

	// little endian
	int numSampleBytes =
		(int)( readBuffer[3] << 24 |
			   readBuffer[2] << 16 |
			   readBuffer[1] << 8 |
			   readBuffer[0] );

	*/

	fseek( file, 44, SEEK_SET );


	int currentPos = ftell( file );

	int numSampleBytes = fileSize - currentPos;

	*outNumSamples = numSampleBytes / 2;

	int numSamples = *outNumSamples;


	unsigned char *rawSamples = new unsigned char[ 2 * numSamples ];

	int numRead = fread( rawSamples, 1, numSamples * 2, file );


	if( numRead != numSamples * 2 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to read %d samples from file: %s\n",
						numSamples, arecordFileName );

		delete [] rawSamples;
		fclose( file );
		return NULL;
	}


	int16_t *returnSamples = new int16_t[ numSamples ];

	int r = 0;
	for( int i=0; i<numSamples; i++ ) {
		// little endian
		returnSamples[i] =
				( rawSamples[r+1] << 8 ) |
				rawSamples[r];

		r += 2;
	}
	delete [] rawSamples;



	fclose( file );

	return returnSamples;
}




#ifdef LINUX

char isPrintingSupported() {
    int ret = system( "convert --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
    }


void printImage( Image *inImage, char inFullColor ) {
    const char *fileName = "printImage_temp.tga";

    writeTGAFile( fileName, inImage );

    const char *colorspaceFlag = "-colorspace gray";

    if( inFullColor ) {
        colorspaceFlag = "";
        }


    char *command =
        autoSprintf( "convert -density 72x72 "
                     " %s %s ps:- | lpr",
                     colorspaceFlag, fileName );


    system( command );

    delete [] command;

    // File file( NULL, fileName );
    // file.remove();
    }


#else

char isPrintingSupported() {
	return false;
}

void printImage( Image *inImage, char inFullColor ) {
}

#endif




#ifndef LIGHTNING_INCLUDED
#define LIGHTNING_INCLUDED

#include <math.h>

typedef struct LightSource {
	int holder;
	int x;
	int y;
	int value;
} LightSource;

typedef struct LightBlocker {
	int x;
	int y;
	int radius;
} LightBlocker;

typedef struct ColorInfo {
	float r;
	float b;
	float g;
	float a;
	bool additive;
	int lux;
	int shadow;
} ColorInfo;

ColorInfo getDrawSpecifics(int cellX, int cellY, float darkness, int time);

float DayLight(int time_current, int night_frequency);

bool IsShadow(int cellX, int cellY);

void getIlluminationLevel(int cellX, int cellY, int *lux, int *shadow);

void updateLightBlocker( int cellX, int cellY, int blockStatus );

void updateLightSource( int cellX, int cellY, int lightValue );

void updateHeldLightSources( int actorID, int cellX, int cellY, int lightValue );


void SetTimeSettings(float darkness, int frequency, int deviation);

#endif

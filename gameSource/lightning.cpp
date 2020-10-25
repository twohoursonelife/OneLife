//#include <iostream>

#include "lightning.h"
#include <math.h>
#include "minorGems/util/SimpleVector.h"

static SimpleVector<LightSource> heldLightSources;
static SimpleVector<LightSource> mapLightSources;
static SimpleVector<LightBlocker> lightBlockers;

//if increasing or decresing this value it is recomended to use only factors and multiples of 36
//this way it will match some nights with the other clients despite having a different value
int night_frequency_overwrite = -1;

//between 0 and 1; If one the player can't see at midnight
float night_darkness = 0.75f;

//for smooth transition modify be gaussian function instead
int curve_deviation = 10;

float DayLight(int time_current, int night_frequency) {
	if (time_current == -1) { return 0; }

	float frequency = night_frequency;
	if (night_frequency_overwrite != -1) {
		frequency = night_frequency_overwrite;
	}
	int world_time_span = abs(86400/frequency);
	
	//makes the next value start at 0
	//this should be a integer. this is not the case because c++ math is weird
	//fmod don't work with negative values. this is weirder than I first supposed
	float world_time = fmod(time_current+(world_time_span/2), world_time_span);
	
	//change hours to a range between -1 and 1
	float converted_hours = ((2*world_time) - world_time_span) / world_time_span;
	
	//uses a gaussian function to simulate daylight cicle
	float d_curve = exp((0-curve_deviation)*converted_hours*converted_hours);
	
	float current_darkness = d_curve * night_darkness;
	return current_darkness;
}

float distance(int ux, int uy, int vx, int vy) {

	//simple equation for distance of two points in the cartesian plane
	int dif_x = ux - vx;
	int dif_y = uy - vy;

	int squared_difs = dif_x * dif_x + dif_y * dif_y;

	float distance = sqrt(squared_difs);
	return distance;
}

bool checkLightCollision( float a, float b, float c, float radius ) {
	//abc represents a triangle
	//a is the distance from the origin to the destination OD
	//b is the distance from the origin to the center of the circle OC
	//c is the distance from the destination to the center of the circle CD

	//don't count collision if light is coming from the inside of the circle
	if ( b <= radius ) {
		//return false;
	}
	if ( c <= 1 ) {
		//return true;
	}
	
	float triangle_b = a;
	float collision_line;
	if (a > b && a > c) {
		float s = (a + b + c) / 2;
		float s_products = s * (s - a) * (s - b) * (s - c);

		float triangle_a = sqrt(s_products);
		//the collision line is equal to the height of the triangle
		collision_line = (2 * triangle_a) / triangle_b;
	}
	else {
		//the collision line is simply the distance of the point to the center of the circle
		collision_line = c;
	}

	if (collision_line <= radius) {
		return true;
	}
	else {
		return false;
	}
}

int getIlluminationLevel(int cellX, int cellY) {
	SimpleVector<LightSource> allLightSources;
	allLightSources.push_back_other(&mapLightSources);
	allLightSources.push_back_other(&heldLightSources);

	int cX = cellX;
	int cY = cellY;
	
	int lightValue = 0;
	for (int i = 0; i < allLightSources.size(); i++) {
		LightSource source = allLightSources.getElementDirect(i);
		int currentLightValue = 0;
		int sX = source.x;
		int sY = source.y;

		float dist_source = distance(sX, sY, cX, cY);
		float light_intensity = source.value + 0.5f;//magic number kek
		
		if ( false ) {
			//simplified version
			if (dist_source <= light_intensity / 2) {
				lightValue = 5; //max
				break;
			}
			else {
				lightValue = 0; //min
				continue;
			}
		}
		if ( false ) {
			//def lightness
			if (dist_source <= light_intensity / 5) {
				lightValue = 5;
				break;
			}
			//def intern shadow
			else if (dist_source <= light_intensity / 3) {
				lightValue = 2;
				break;
			}
			//def extern shadow
			else if (dist_source <= light_intensity / 2) {
				lightValue = 1;
				break;
			}
			//def darkness
			else {
				lightValue = 0;
				continue;
			}
		}
		
		if (dist_source <= light_intensity / 2) {
			currentLightValue = 1;
		}
		if (dist_source <= light_intensity / 3) {
			currentLightValue = 2;
		}
		if (dist_source <= light_intensity / 4) {
			currentLightValue = 3;
		}
		if (dist_source <= light_intensity / 5) {
			currentLightValue = 4;
		}
		if (dist_source <= light_intensity / 9) {
			currentLightValue = 5;
		}
		
		//this light source is not contributing to the lightValue, skip
		if (currentLightValue == 0) { continue; }
		
		if (false) {
			bool sourceIsBlocked = false;
			for (int j = 0; j < lightBlockers.size(); j++) {
				LightBlocker blocker = lightBlockers.getElementDirect(j);
				int bX = blocker.x;
				int bY = blocker.y;
				
				float dist_blocker = distance(bX, bY, cX, cY);
				float dist_cross = distance(sX, sY, bX, bY);
				if ( dist_source <= dist_cross) { continue; }
				else if (dist_source <= dist_blocker){ continue; }
				else if (dist_cross == 0 ) { continue; }
				else {
					//fixed radius for now
					sourceIsBlocked = checkLightCollision(dist_source, dist_cross, dist_blocker, 0.75f);
				}
				if (sourceIsBlocked) {
					//it will become negative sometimes, but that's ignored ahead
					currentLightValue -= 1;
					break;
				}
			}
		}
		
		//always take the highest light level measured
		if (currentLightValue > lightValue) {
			lightValue = currentLightValue;
		}
	}
	return lightValue;
}

bool Shadow(int cellX, int cellY) {
	SimpleVector<LightSource> allLightSources;
	allLightSources.push_back_other(&mapLightSources);
	allLightSources.push_back_other(&heldLightSources);

	int cX = cellX;
	int cY = cellY;
	
	bool outOfReach = true;
	bool shadow = true;
	
	//see if it shadow stays true after checking all light sources
	for (int i = 0; i < allLightSources.size(); i++) {
		LightSource source = allLightSources.getElementDirect(i);
		int sX = source.x;
		int sY = source.y;
		
		float dist_source = distance(sX, sY, cX, cY);
		float light_intensity = source.value + sqrt(0.5f); //magic number kek
		
		//this light source can't cast a shadow because it is out of reach to begin with
		if (dist_source > light_intensity / 2) {
			continue;
		}
		outOfReach = false;
		
		bool sourceIsBlocked = false;
		//check if light coming from this source can't reach the cell because there is a blocker
		for (int j = 0; j < lightBlockers.size(); j++) {
			LightBlocker blocker = lightBlockers.getElementDirect(j);
			
			int bX = blocker.x;
			int bY = blocker.y;
			
			float dist_blocker = distance(bX, bY, cX, cY);
			float dist_cross = distance(sX, sY, bX, bY);
			if ( dist_source <= dist_cross) { continue; }
			else if (dist_source <= dist_blocker){ continue; }
			else if (dist_cross == 0 ) { continue; }
			else {
				//fixed radius for now
				sourceIsBlocked = checkLightCollision(dist_source, dist_cross, dist_blocker, 0.5f);
			}
			if (sourceIsBlocked) {
				break;
			}
		}
		if (!sourceIsBlocked) {
			shadow = false;
		}
	}
	
	//don't draw shadows for objects out of reach
	if (outOfReach) { shadow = false; }
	return shadow;
}


bool lightSourceExists(int cellX, int cellY) {
	int size = mapLightSources.size();
	if (size > 0) {
		for (int i = 0; i < mapLightSources.size(); i++) {
			LightSource s = mapLightSources.getElementDirect(i);

			if (s.x == cellX && s.y == cellY) {
				return true;
			}
		}
	}
	return false;
}

bool lightBlockerExists(int cellX, int cellY) {
	int size = lightBlockers.size();
	if (size > 0) {
		for (int i = 0; i < lightBlockers.size(); i++) {
			LightBlocker b = lightBlockers.getElementDirect(i);

			if (b.x == cellX && b.y == cellY) {
				return true;
			}
		}
	}
	return false;
}


void removeLightSource(int cellX, int cellY) {
	for (int i = 0; i < mapLightSources.size(); i++) {
		LightSource s = mapLightSources.getElementDirect(i);

		if (s.x == cellX && s.y == cellY) {
			mapLightSources.deleteElement(i);
			break;
		}
	}
}

void updateLightSource(int cellX, int cellY, int lightValue) {
	if (mapLightSources.size() > 300) { mapLightSources.deleteAll(); }
	
	bool alreadyExist = lightSourceExists(cellX, cellY);
	if (lightValue == 0 && alreadyExist) {
		removeLightSource(cellX, cellY);
		return;
	}
	else if (lightValue == 0) {
		return;
	}
	else if (alreadyExist) {
		removeLightSource(cellX, cellY);
	}
	
	LightSource s;
	s.x = cellX;
	s.y = cellY;
	s.value = lightValue;

	mapLightSources.push_back(s);
}


void removeHeldLightSources(int holderID) {
	for (int i = 0; i < heldLightSources.size(); i++) {
		LightSource s = heldLightSources.getElementDirect(i);

		if (s.holder == holderID) {
			heldLightSources.deleteElement(i);
			break;
		}
	}
}

void updateHeldLightSources(int actorID, int cellX, int cellY, int lightValue) {

	removeHeldLightSources(actorID);
	if (lightValue == 0) { return; }

	LightSource s;
	s.holder = actorID;
	s.x = cellX;
	s.y = cellY;
	s.value = lightValue;

	heldLightSources.push_back(s);
}


void removeLightBlocker( int cellX, int cellY) {
	for (int i = 0; i < lightBlockers.size(); i++) {
		LightBlocker b = lightBlockers.getElementDirect(i);

		if (b.x == cellX && b.y == cellY) {
			lightBlockers.deleteElement(i);
			break;
		}
	}
}

void updateLightBlocker( int cellX, int cellY, int blockStatus ) {
	if (lightBlockers.size() > 150) { lightBlockers.deleteAll(); }
	
	bool alreadyExist = lightBlockerExists(cellX, cellY);
	if (blockStatus == false && alreadyExist) {
		removeLightBlocker(cellX, cellY);
		return;
	}
	else if (blockStatus == false) {
		return;
	}
	else if (alreadyExist) {
		removeLightBlocker(cellX, cellY);
	}

	LightBlocker b;
	b.x = cellX;
	b.y = cellY;
	//b.radius = sqrt(0.5f);

	lightBlockers.push_back(b);
}


//could be used along with ini settings
void SetTimeSettings(float darkness, int frequency, int deviation) {
	night_darkness = darkness;
	curve_deviation = deviation;
	night_frequency_overwrite = frequency;
}

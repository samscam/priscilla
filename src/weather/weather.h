#ifndef PRISCILLA_WEATHER
#define PRISCILLA_WEATHER


enum PrecipType {
	Rain, Snow, Sleet
};

struct weather {
  const char* summary;
  const char* type;
  float precipChance;
	float precipIntensity;
  PrecipType precipType;
  float maxTmp;
  float minTmp;
	bool thunder;
	float windSpeed;
};



#include "darksky.h"

#endif

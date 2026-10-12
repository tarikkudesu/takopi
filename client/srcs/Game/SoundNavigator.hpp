#ifndef __SOUND_NAVIGATOR_HPP__
#define __SOUND_NAVIGATOR_HPP__

#include "../Enums.hpp"

class Strategy;

class SoundNavigator
{
	public:
		SoundNavigator();
		~SoundNavigator();

		// Translates server sound direction (1-8) to command:
		// 1: front
		// 2: front-left, 3: left, 4: back-left
		// 5: back
		// 6: back-right, 7: right, 8: front-right
		// 0: same tile
		static String stepSoundDirection( int soundDirection, Strategy *ctx = NULL );

		// Translates vision tile index in field of view to movement command
		static String stepTowardVisionTile( size_t tileIndex, Strategy *ctx = NULL );

		// Random wander step
		static String wander( Strategy *ctx = NULL );
};

#endif

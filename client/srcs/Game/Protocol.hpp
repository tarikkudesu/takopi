#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include "../Enums.hpp"

/*************************************************************************
 *                    WIRE FORMAT PARSING / BUILDING                     *
 *************************************************************************/

class Protocol
{
	private:
		Protocol();
		Protocol( const Protocol &copy );
		Protocol							&operator=( const Protocol &assign );
		~Protocol();

	public:
		static String						resourceName( e_resource res );
		static e_resource					resourceFromName( const String &name );

		static t_tile_content				parseTileContent( const String &tileStr );
		static std::vector<t_tile_content>	parseVision( const String &line );
		static bool							parseInventory( const String &line, int inventory[RESOURCE_COUNT] );
		static bool							parseBroadcast( const String &line, int &outDirection, String &outText );
		static bool							parseDeplacement( const String &line, int &outDirection );
		static bool							parseNiveauActuel( const String &line, int &outLevel );
		static bool							isDeath( const String &line );
		static bool							isElevationEnCours( const String &line );
};

#endif

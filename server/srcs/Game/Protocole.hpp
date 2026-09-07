#ifndef __PROTOCOLE_HPP__
# define __PROTOCOLE_HPP__

# include "Game.hpp"

typedef enum e_protocol_verb
{
	PROTOCOL_WELCOME,
	PROTOCOL_GRAPHIC,
	PROTOCOL_MSZ,
	PROTOCOL_BCT,
	PROTOCOL_MCT,
	PROTOCOL_TNA,
	PROTOCOL_PNW,
	PROTOCOL_PPO,
	PROTOCOL_PLV,
	PROTOCOL_PIN,
	PROTOCOL_PEX,
	PROTOCOL_PBC,
	PROTOCOL_PIC,
	PROTOCOL_PIE,
	PROTOCOL_PFK,
	PROTOCOL_PDR,
	PROTOCOL_PGT,
	PROTOCOL_PDI,
	PROTOCOL_ENW,
	PROTOCOL_EHT,
	PROTOCOL_EBO,
	PROTOCOL_EDI,
	PROTOCOL_SGT,
	PROTOCOL_SST,
	PROTOCOL_SEG,
	PROTOCOL_SMG,
	PROTOCOL_SUC,
	PROTOCOL_SBP,
	PROTOCOL_UNKNOWN
} t_protocol_verb;

class Protocole
{
	private:
		Protocole() = delete;
		Protocole( const Protocole &copy ) = delete;
		Protocole	&operator=( const Protocole &assign ) = delete;
		~Protocole() = delete;

		static bool		parseUnsigned(const String &token, int &value);
		static bool		parsePlayerId(const String &token, int &playerId);
		static int		protocolOrientation(e_direction direction);
		static int		protocolResource(e_resource resource);

	public:
		static t_protocol_verb	parseVerb(const String &value);
		static String	verb(t_protocol_verb value);
		static String	executeRequest(const String &request, Game &game);
		static String	snapshot(const Game &game);
		static String	welcome();

		static String	fullMap(const Game &game);
		static String	mapSize(const Game &game);
		static String	teams(const Game &game);
		static String	timeUnit(const Game &game);
		static String	playerLevel(const Player &player);
		static String	tile(const Game &game, int x, int y);
		static String	playerPosition(const Player &player);
		static String	playerInventory(const Player &player);
		static String	playerNew(const Game &game, const Player &player);

		static String	incantationStart(const s_incantation_context &context);
		static String	playerBroadcast(int playerId, const String &message);
		static String	resourceDropped(int playerId, e_resource resource);
		static String	resourceTaken(int playerId, e_resource resource);
		static String	incantationEnd(int x, int y, bool success);
		static String	serverMessage(const String &message);
		static String	gameEnd(const String &teamName);
		static String	playerExpelled(int playerId);
		static String	playerDeath(int playerId);
		static String	forkStart(int playerId);
		static String	eggNew(const Egg &egg);
		static String	eggConsumed(int eggId);
		static String	eggHatched(int eggId);
		static String	eggDeath(int eggId);
		static String	unknownCommand();
		static String	badParameters();
};

#endif

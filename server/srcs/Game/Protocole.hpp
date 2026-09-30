#ifndef __PROTOCOLE_HPP__
# define __PROTOCOLE_HPP__

# include "Game.hpp"

class Protocole
{
	private:
		Protocole() = delete;
		Protocole( const Protocole &copy ) = delete;
		Protocole	&operator=( const Protocole &assign ) = delete;
		~Protocole() = delete;

		static bool					parsePlayerId(const String &token, int &playerId);
		static bool					parseUnsigned(const String &token, int &value);
		static int					protocolOrientation(Direction direction);
		static int					protocolResource(Resource resource);

	public:
		static String				executeRequest(const String &request, Game &game);
		static ProtocoleVerb		parseVerb(const String &value);
		static String				verb(ProtocoleVerb value);
		static String				snapshot(const Game &game);
		static String				welcome();

		static String				fullMap(const Game &game);
		static String				mapSize(const Game &game);
		static String				teams(const Game &game);
		static String				timeUnit(const Game &game);
		static String				playerLevel(const Player &player);
		static String				tile(const Game &game, int x, int y);
		static String				playerPosition(const Player &player);
		static String				playerInventory(const Player &player);
		static String				playerNew(const Game &game, const Player &player);

		static String				incantationStart(const IncantationContext &context);
		static String				playerBroadcast(int playerId, const String &message);
		static String				resourceDropped(int playerId, Resource resource);
		static String				resourceTaken(int playerId, Resource resource);
		static String				incantationEnd(int x, int y, bool success);
		static String				serverMessage(const String &message);
		static String				gameEnd(const String &teamName);
		static String				playerExpelled(int playerId);
		static String				playerDeath(int playerId);
		static String				forkStart(int playerId);
		static String				eggNew(const Egg &egg);
		static String				eggConsumed(int eggId);
		static String				eggHatched(int eggId);
		static String				eggDeath(int eggId);
		static String				unknownCommand();
		static String				badParameters();
};

#endif

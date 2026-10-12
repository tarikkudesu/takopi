#ifndef __BROADCAST_MESSENGER_HPP__
#define __BROADCAST_MESSENGER_HPP__

#include "../Enums.hpp"
#include <vector>

enum e_msg_cmd
{
	MSG_NONE = 0,
	MSG_OFFER,     // Leader announces missing stones and needed player slots
	MSG_JOIN,      // Worker announces intention to join leader
	MSG_HERE,      // Worker confirms arrived on leader's tile
	MSG_FULL,      // Leader announces team/tile is complete; others cancel
	MSG_INCANT,    // Leader announces incantation is firing
	MSG_ABORT,     // Elevation cancelled
	MSG_EGG        // Bot announces laying an egg for team expansion
};

struct s_parsed_msg
{
	int         senderId;
	e_msg_cmd   cmd;
	int         targetLevel;
	int         leaderId;
	int         slotsNeeded;
	int         missingStones[RESOURCE_COUNT];
	bool        valid;
};

class BroadcastMessenger
{
	private:
		String          __teamName;
		int             __secretHash;

		int             computeHash( const String &team ) const;

	public:
		BroadcastMessenger();
		BroadcastMessenger( const String &team );
		~BroadcastMessenger();

		void            setTeam( const String &team );
		int             getSecretHash() const;

		// Message builders
		String          buildOffer( int myId, int level, const int missingStones[RESOURCE_COUNT], int slotsNeeded ) const;
		String          buildJoin( int myId, int level, int leaderId ) const;
		String          buildHere( int myId, int level, int leaderId ) const;
		String          buildFull( int myId, int level ) const;
		String          buildIncant( int myId, int level ) const;
		String          buildAbort( int myId, int level ) const;
		String          buildEgg( int myId ) const;

		// Message parser
		bool            parse( const String &rawText, s_parsed_msg &outMsg ) const;
};

#endif

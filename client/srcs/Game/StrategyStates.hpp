#ifndef __STRATEGY_STATES_HPP__
#define __STRATEGY_STATES_HPP__

#include "IState.hpp"

class StateSolo : public IState
{
	public:
		virtual ~StateSolo() {}
		virtual String execute( Strategy *ctx );
};

class StateSurvive : public IState
{
	public:
		virtual ~StateSurvive() {}
		virtual String execute( Strategy *ctx );
};

class StateForage : public IState
{
	public:
		virtual ~StateForage() {}
		virtual String execute( Strategy *ctx );
};

class StateLeaderGather : public IState
{
	public:
		virtual ~StateLeaderGather() {}
		virtual String execute( Strategy *ctx );
};

class StateJoinLeader : public IState
{
	public:
		virtual ~StateJoinLeader() {}
		virtual String execute( Strategy *ctx );
};

class StateIncanting : public IState
{
	public:
		virtual ~StateIncanting() {}
		virtual String execute( Strategy *ctx );
};

#endif

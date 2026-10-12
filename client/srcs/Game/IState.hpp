#ifndef __ISTATE_HPP__
#define __ISTATE_HPP__

#include "../Enums.hpp"

class Strategy;

class IState
{
	public:
		virtual ~IState() {}
		virtual String execute( Strategy *context ) = 0;
};

#endif

#ifndef __MZU_HPP__
#define __MZU_HPP__

#include "../zappy.hpp"

class mzu
{
	private:
		mzu();
		mzu(const mzu &copy);
		mzu &operator=(const mzu &assign);
		~mzu();

	public:
		static void debug(String __log_message);
		static void info(String __log_message);
		static void warn(String __log_message);
		static void error(String __log_message);
		static void fatal(String __log_message);
		static void terr(const String &__error_message);

		static String logDate();
		static void trimSpaces(String &str);
		static String intToString(int number);
		static long stringToInt(const String &str);
		static std::vector<String> splitBySpaces(const String &input);
		static std::vector<String> splitByChar(const String &input, char del);
};

#endif

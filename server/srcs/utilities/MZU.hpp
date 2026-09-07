#ifndef __MZU_HPP__
#define __MZU_HPP__

#include "../zappy.hpp"

class mzu
{
	private:
		static std::ofstream __logFile;
		static bool __debug;
		static bool __info;
		static bool __warn;
		static bool __error;
		static bool __fatal;
		static void writeLog(const String &level, const String &message);

		mzu() = delete;
		mzu(const mzu &copy) = delete;
		mzu &operator=(const mzu &assign) = delete;
		~mzu() = delete;

	public:
		static void logs(const std::vector<std::string> &args);
		static void debug(String __log_message);
		static void info(String __log_message);
		static void warn(String __log_message);
		static void error(String __log_message);
		static void fatal(String __log_message);
		static void running(String __log_message);
		static void terr(const String &__error_message);
		
		static void close();
		static String logDate();
		static void trimSpaces(String &str);
		static void bzero(void *s, size_t n);
		static String intToString(int number);
		static long stringToInt(const String &str);
		static std::vector<String> splitBySpaces(const String &input);
};

#endif

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
		class persist : public std::exception
		{
		public:
			persist();
			virtual const char *what(void) const throw();
		};
		class Close : public std::exception
		{
		public:
			Close();
			virtual const char *what(void) const throw();
		};
		class Exit
		{
		public:
			Exit();
			virtual const char *what(void) const throw();
		};

		static bool __criticalOverLoad;
		static bool __debug;
		static bool __info;
		static bool __warn;
		static bool __error;
		static bool __fatal;
		static void logs(const std::vector<std::string> &args);
		static void debug(String __log_message);
		static void info(String __log_message);
		static void warn(String __log_message);
		static void error(String __log_message);
		static void fatal(String __log_message);
		static void running(String __log_message);
		static void terr(const String &__error_message);

		static String logDate();
		static void trimSpaces(String &str);
		static void bzero(void *s, size_t n);
		static String intToString(int number);
		static size_t strlen(const char *str);
		static char *strdup(String const &s1);
		static int hexToInt(const String &str);
		static void toUpperString(String &input);
		static void toLowerString(String &input);
		static String generateTimeBasedFileName();
		static String buildIMFDate(size_t elapsed);
		static long stringToInt(const String &str);
		static String readFielContent(String fileName);
		static std::vector<String> splitBySpaces(const String &input);
		static bool samePath(const String &path1, const String &path2);
		static String buildListingBody(String path, const t_svec &list);
		static String joinPaths(const String &path1, const String &path2);
		static bool containsPath(const String &path, const String &subPath);
		static String mergeByChar(const std::vector<String> &input, char del);
		static std::vector<String> splitByChar(const String &input, char del);
		static void replaceString(String &original, const String toReplace, const String replacement);
};

#endif

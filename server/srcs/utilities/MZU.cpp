#include "MZU.hpp"

bool mzu::__debug = false;
bool mzu::__info = false;
bool mzu::__warn = false;
bool mzu::__error = false;
bool mzu::__fatal = false;
std::ofstream mzu::__logFile;

/************************************************************************************************
 *											   LOGS 											*
 ************************************************************************************************/

void mzu::logs(const std::vector<String> &args)
{
	mzu::__logFile.open("zappy.log", std::ios::out | std::ios::trunc);
	if (!mzu::__logFile.is_open())
	{
		std::cerr << "error: couldn't open zappy.log" << std::endl;
		exit(EXIT_FAILURE);
	}
	mzu::__logFile << std::unitbuf;
	for (std::vector<String>::const_iterator it = args.begin(); it != args.end(); it++)
	{
		if (*it == "debug")
			mzu::__debug = true;
		else if (*it == "info")
			mzu::__info = true;
		else if (*it == "warn")
			mzu::__warn = true;
		else if (*it == "error")
			mzu::__error = true;
		else if (*it == "fatal")
			mzu::__fatal = true;
		else if (*it == "all")
		{
			mzu::__info = true;
			mzu::__warn = true;
			mzu::__error = true;
			mzu::__fatal = true;
			mzu::__debug = true;
		}
	}
	if (READ_SIZE < 1024)
	{
		mzu::terr("read size less then 1024 is not recommended");
		exit(EXIT_FAILURE);
	}
}
void mzu::writeLog(const String &level, const String &message)
{
	if (mzu::__logFile.is_open())
		mzu::__logFile << mzu::logDate() << " [" << level << "] " << message << std::endl;
}
void mzu::debug(String __log_message)
{
	if (mzu::__debug)
		mzu::writeLog("DEBUG", __log_message);
}
void mzu::info(String __log_message)
{
	if (mzu::__info)
		mzu::writeLog("INFO", __log_message);
}
void mzu::warn(String __log_message)
{
	if (mzu::__warn)
		mzu::writeLog("WARN", __log_message);
}
void mzu::error(String __log_message)
{
	if (mzu::__error)
		mzu::writeLog("ERROR", __log_message);
}
void mzu::fatal(String __log_message)
{
	if (mzu::__fatal)
		mzu::writeLog("FATAL", __log_message);
}
void mzu::running(String __log_message)
{
	mzu::writeLog("RUNNING", __log_message);
}
void mzu::terr(const String &__error_message)
{
	mzu::writeLog("ERROR", __error_message);
}
/*************************************************************************************************
 *											 UTILITIES											 *
 *************************************************************************************************/

void mzu::bzero(void *s, size_t n)
{
	char *bytePtr = static_cast<char *>(s);
	for (size_t i = 0; i < n; ++i)
		bytePtr[i] = 0;
}
String mzu::logDate()
{
	char buffer[30];
	std::time_t t = std::time(NULL);
	std::tm *tm = std::gmtime(&t);
	std::strftime(buffer, sizeof(buffer), "[%d/%b/%Y:%H:%M:%S]", tm);
	return String(buffer);
}
void mzu::trimSpaces(String &str)
{
	if (str.empty())
		return;
	size_t start = 0;
	size_t end = str.length() - 1;
	while (start <= end && std::isspace(str[start]))
		++start;
	while (end >= start && std::isspace(str[end]))
		--end;
	if (start > end)
		str.clear();
	else
		str = str.substr(start, end - start + 1);
}
std::vector<String> mzu::splitBySpaces(const String &input)
{
	String word;
	std::istringstream iss(input);
	std::vector<String> result;
	while (iss >> word)
		result.push_back(word);
	return result;
}
String mzu::intToString(int number)
{
	std::ostringstream oss;
	oss << number;
	return oss.str();
}
long mzu::stringToInt(const String &str)
{
	std::istringstream iss(str);
	long number = 0;
	iss >> number;
	return number;
}

void mzu::close()
{
	mzu::__logFile.close();
}
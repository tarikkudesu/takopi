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

t_args mzu::argparse(const t_svec &args)
{
	if (!RESPAWN_TIME)
	{
		mzu::terr("respawn time interval must not be 0");
		mzu::close();
		exit(EXIT_FAILURE);
	}
	if (READ_SIZE < 1024)
	{
		mzu::terr("read size less then 1024 is not recommended");
		mzu::close();
		exit(EXIT_FAILURE);
	}
	mzu::__logFile.open("zappy.log", std::ios::out | std::ios::trunc);
	if (!mzu::__logFile.is_open())
	{
		mzu::terr("couldn't open zappy.log");
		exit(EXIT_FAILURE);
	}
	if (args.size() == 0)
	{
		mzu::terr("no arguments provided");
		mzu::close();
		exit(EXIT_FAILURE);
	}
	mzu::__logFile << std::unitbuf;
	try
	{
		t_args res;
		size_t i = 0;
	
		if (args.size() == 0)
			throw std::invalid_argument("no arguments provided");

		while (i < args.size() - 1)
		{
			const String &flag = args[i];
	
			if (flag == "debug" || flag == "info" || flag == "warn" || flag == "error" || flag == "fatal" || flag == "all")
			{
				if (flag == "debug" || flag == "all")
					mzu::__debug = true;
				if (flag == "info" || flag == "all")
					mzu::__info = true;
				if (flag == "warn" || flag == "all")
					mzu::__warn = true;
				if (flag == "error" || flag == "all")
					mzu::__error = true;
				if (flag == "fatal" || flag == "all")
					mzu::__fatal = true;
				++i;
				continue;
			}
	
			if (flag == "-n")
			{
				t_svec teams;
				if (res.count("teams"))
					throw std::invalid_argument("duplicate option: -n");
				for (++i; i < args.size() - 1 && args[i][0] != '-'; ++i)
					teams.push_back(args[i]);
				if (teams.empty())
					throw std::invalid_argument("-n requires at least one team");
				res["teams"] = teams;
				continue;
			}
	
			String key;
			if (flag == "-p")      		key = "port";
			else if (flag == "-x") 		key = "width";
			else if (flag == "-y") 		key = "height";
			else if (flag == "-c") 		key = "clients_per_team";
			else if (flag == "-t") 		key = "time";
			else
				throw std::invalid_argument("unknown option: " + flag);
	
			if (res.count(key))
				throw std::invalid_argument("duplicate option: " + flag);
			if (i + 1 >= args.size() - 1)
				throw std::invalid_argument("missing value for " + flag);
	
			const String &val = args[i + 1];
			res[key] = t_svec(1, val);
			i += 2;
		}
		return res;
	}
	catch(const std::exception& e)
	{
		mzu::terr(e.what());
		mzu::close();
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
	std::cerr << mzu::logDate() << " [ERROR] " << __error_message << std::endl;
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
t_svec mzu::splitBySpaces(const String &input)
{
	String word;
	std::istringstream iss(input);
	t_svec result;
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
#include "MZU.hpp"

mzu::mzu() {}
mzu::mzu(const mzu &copy) { (void)copy; }
mzu &mzu::operator=(const mzu &assign)
{
	(void)assign;
	return *this;
}
mzu::~mzu() {}

/************************************************************************************************
 *											   LOGS 											*
 ************************************************************************************************/

void mzu::debug(String __log_message)
{
	(void)__log_message;
}
void mzu::info(String __log_message)
{
	std::cout << BLUE << mzu::logDate() << GREEN << " [INFO] " << RESET << __log_message << std::endl;
}
void mzu::warn(String __log_message)
{
	std::cout << BLUE << mzu::logDate() << YELLOW << " [WARN] " << RESET << __log_message << std::endl;
}
void mzu::error(String __log_message)
{
	std::cerr << BLUE << mzu::logDate() << RED << " [ERROR] " << RESET << __log_message << std::endl;
}
void mzu::fatal(String __log_message)
{
	std::cerr << BLUE << mzu::logDate() << RED << " [FATAL] " << RESET << __log_message << std::endl;
}
void mzu::terr(const String &__error_message)
{
	std::cerr << RED << "error: " << RESET << __error_message << std::endl;
}

/*************************************************************************************************
 *											 UTILITIES											 *
 *************************************************************************************************/

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
std::vector<String> mzu::splitByChar(const String &input, char del)
{
	std::vector<String> result;
	String temp;
	for (size_t i = 0; i < input.size(); ++i)
	{
		if (input[i] == del)
		{
			if (!temp.empty())
				result.push_back(temp);
			temp.clear();
		}
		else
			temp += input[i];
	}
	if (!temp.empty())
		result.push_back(temp);
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

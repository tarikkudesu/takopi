#include "BasicString.hpp"

BasicString::BasicString() : __buff(NULL), __size(0)
{
}

BasicString::BasicString(const String &str) : __buff(NULL), __size(str.length())
{
	if (!str.length())
		return;
	__buff = new char[__size];
	for (size_t i = 0; i < __size; i++)
		__buff[i] = str.at(i);
}

BasicString::BasicString(char *buff, size_t size) : __buff(NULL), __size(size)
{
	if (buff == NULL)
		return;
	__buff = new char[__size];
	for (size_t i = 0; i < __size; i++)
		__buff[i] = buff[i];
}

BasicString::BasicString(const BasicString &copy) : __buff(NULL), __size(0)
{
	if (copy.__size == 0)
		return;
	__size = copy.__size;
	__buff = new char[__size];
	for (size_t i = 0; i < __size; i++)
		__buff[i] = copy.__buff[i];
}

BasicString::~BasicString()
{
	if (__buff)
		delete[] __buff;
}
bool BasicString::empty() const
{
	return !__size;
}
void BasicString::clear()
{
	if (__buff)
		delete[] __buff;
	__buff = NULL;
	__size = 0;
}
BasicString &BasicString::operator=(const BasicString &assign)
{
	if (this != &assign)
	{
		delete[] __buff;
		__buff = NULL;
		__size = assign.__size;
		if (__size != 0)
		{
			__buff = new char[__size];
			for (size_t i = 0; i < __size; i++)
				__buff[i] = assign.__buff[i];
		}
	}
	return *this;
}
void BasicString::erase(size_t start, size_t end)
{
	if (start > end || end > __size) 
		return;
	size_t len_to_remove = end - start;
	size_t new_size = __size - len_to_remove;
	char *new_buff = new char[new_size];
	for (size_t i = 0; i < start; i++)
		new_buff[i] = __buff[i];
	for (size_t i = end; i < __size; i++)
		new_buff[i - len_to_remove] = __buff[i];
	delete[] __buff;
	__buff = new_buff;
	__size = new_size;
}
void BasicString::join(const BasicString &j)
{
	char *new_buff = new char[__size + j.__size];
	for (size_t i = 0; i < __size; i++)
		new_buff[i] = __buff[i];
	for (size_t i = 0; i < j.__size; i++)
		new_buff[__size + i] = j.__buff[i];
	delete[] __buff;
	__buff = new_buff;
	__size = __size + j.__size;
}
char *BasicString::getBuff() const
{
	return __buff;
}
size_t BasicString::length() const
{
	return __size;
}
BasicString BasicString::duplicate() const
{
	return BasicString(__buff, __size);
}
BasicString BasicString::substr(size_t start, size_t length)
{
	if (start >= __size)
		return BasicString(NULL, 0);
	if (start + length > __size)
		length = __size - start;
	return BasicString(__buff + start, length);
}
std::string BasicString::to_string() const
{
	std::string result;
	result.reserve(__size);
	for (size_t i = 0; i < __size; i++)
		result += __buff[i];
	return result;
}
size_t BasicString::find(const String &f) const
{
	if (f.length() == 0 || f.length() > this->__size)
		return String::npos;
	for (size_t i = 0; i <= __size - f.length(); i++)
	{
		bool found = true;
		for (size_t j = 0; j < f.length(); j++)
		{
			if (__buff[i + j] != f[j])
			{
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return String::npos;
}
std::ostream &operator<<(std::ostream &o, const BasicString &str)
{
	char *buff = str.getBuff();
	size_t size = str.length();
	for (size_t i = 0; i < size; i++)
		o << buff[i];
	return o;
}
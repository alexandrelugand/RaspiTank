#pragma once
#include <string>
#include <stdarg.h>

using namespace std;

inline const char* string_format(const string fmt, ...)
{
	int size = 100;
	string str;
	va_list ap;
	while (1) 
	{
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
		va_end(ap);
		if (n > -1 && n < size) {
			str.resize(n);
			return str.c_str();
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}
	return str.c_str();
}

inline const char* string_format(const char* fmt, ...)
{
	int size = 100;
	string str;
	va_list ap;
	while (1)
	{
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size) {
			str.resize(n);
			return str.c_str();
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}
	return str.c_str();
}
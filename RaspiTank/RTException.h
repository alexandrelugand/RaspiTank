#pragma once
#include "stack_exception.h"
#include <string>

using namespace std;

namespace RaspiTank
{
	class rt_error : public exception
	{
	protected:
		string msg;

	public:
		explicit rt_error(const string& str) throw()
		{
			msg = str;
		}

		explicit rt_error(const char* str) throw()
		{
			msg = str;
		}
		virtual ~rt_error() throw() {};

		virtual const char*	what() const throw()
		{
			return msg.c_str();
		}
	};

	class RTException : public stack_exception<rt_error>
	{
	public:
		explicit RTException(const string& str) throw() : stack_exception<rt_error>(str)
		{
#ifdef DEBUG
			show_stack = true;
#else
			show_stack = false;
#endif
		}

		explicit RTException(const char* str) throw() : stack_exception<rt_error>(string(str))
		{
#ifdef DEBUG
			show_stack = true;
#else
			show_stack = false;
#endif
		}

		virtual ~RTException() throw() {};
	};
}

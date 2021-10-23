//
// Created by olivier on 20/10/2021.
//

#ifndef OPENLIFE_EXCEPTION_H
#define OPENLIFE_EXCEPTION_H

#include <exception>
#include <cstdarg>

namespace openLife
{
	class Exception : public std::exception
	{
		public:
			Exception(const char* message, ...);
			Exception(const char* message, va_list args);
			~Exception();

			void setMessage(const char* message, va_list args);
			char* getMessage();


		protected:
			char* message;
	};
}

#endif //OPENLIFE_EXCEPTION_H

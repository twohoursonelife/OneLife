//
// Created by olivier on 22/10/2021.
//

#ifndef SERVER_EXCEPTION_H
#define SERVER_EXCEPTION_H

#include "third_party/openLife/base/entity/exception.h"

namespace server
{
	class ConfigurationException : public openLife::Exception
	{
		public:
			ConfigurationException(const char* message, ...);
			~ConfigurationException();

			char* getMessage();

		private:
			char* message;
	};
}

#endif //SERVER_EXCEPTION_H

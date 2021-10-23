//
// Created by olivier on 22/10/2021.
//

#include "configuration.h"

#include <iostream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

server::ConfigurationException::ConfigurationException(const char *message, ...):
	openLife::Exception(message)
{
	va_list args;
	va_start(args, message);
	openLife::Exception::setMessage(message, args);
	va_end(args);
}

server::ConfigurationException::~ConfigurationException()
{

}

char* server::ConfigurationException::getMessage()
{
	return openLife::Exception::getMessage();
}
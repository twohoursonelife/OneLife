//
// Created by olivier on 20/10/2021.
//

#include "exception.h"

#include <iostream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

openLife::Exception::Exception(const char *message, ...)
{
	this->message = nullptr;
	if(message != nullptr)
	{
		va_list args;
		va_start(args, message);
		this->setMessage(message, args);
		va_end(args);
	}
	else
	{
		int msgMaxSize = 64*sizeof(char);
		this->message = (char*) malloc(msgMaxSize);
		memset(this->message, 0, msgMaxSize);
		strcpy(this->message, "Throw of an exception without description");
	}
}

openLife::Exception::~Exception()
{
	if(this->message)free(this->message);
}

void openLife::Exception::setMessage(const char *message, va_list args)
{
	int msgMaxSize = 255 * sizeof(char);//TODO: reallocate this->message if too short
	this->message = (char*)malloc(255 * sizeof(char));
	memset(this->message, 0, msgMaxSize);

	int strValueMaxSize = 32;//TODO: find max size from strlen(unsigned max long long int -1)
	char value[strValueMaxSize];
	memset(value, 0, strValueMaxSize);

	unsigned int idx = 0, vl = 0;
	for(unsigned int i=0; i<strlen(message); i++)
	{
		if(message[i]=='%')
		{
			i++;
			switch(message[i])
			{
				case 'i':
					sprintf(value, "%i", va_arg(args, int));
					vl = strlen(value);
					strncpy(&(this->message[idx]), value, vl);
					idx += vl;
					vl = 0;
					memset(value, 0, strValueMaxSize);
					break;
				case 'f':
					sprintf(value, "%f", va_arg(args, double));
					vl = strlen(value);
					strncpy(&(this->message[idx]), value, vl);
					idx += vl;
					vl = 0;
					memset(value, 0, strValueMaxSize);
					break;
				case 'c':
					sprintf(value, "%c", va_arg(args, int));
					vl = strlen(value);
					strncpy(&(this->message[idx]), value, vl);
					idx += vl;
					vl = 0;
					break;
				case 's':
					sprintf(value, "%s", va_arg(args, char*));
					vl = strlen(value);
					strncpy(&(this->message[idx]), value, vl);
					idx += vl;
					vl = 0;
					break;
				default:
					this->message[idx] = message[i];
					break;
			}
		}
		else this->message[idx++] = message[i];
	}
	va_end(args);
}

char* openLife::Exception::getMessage()
{
	return this->message;
}

//
// Created by olivier on 23/10/2021.
//

#include "textFile.h"

#include <cstdlib>
#include <cstring>
#include "../../../base/entity/exception.h"

openLife::io::TextFile::TextFile()
{
	this->fp = nullptr;
	this->buffer = nullptr;
}

openLife::io::TextFile::~TextFile()
{
	if(this->buffer) free(this->buffer);
	if(this->fp) fclose(this->fp);
}

void openLife::io::TextFile::open(const char *filename)
{
	if(this->fp) throw new openLife::Exception("openLife: this instance already bound to a resource");
	this->fp = fopen(filename, "rw+");
	if(!this->fp) throw new openLife::Exception("openLife: failed to open \"%s\" file", filename);
}

void openLife::io::TextFile::close()
{
	if(this->fp) fclose(this->fp);
	this->fp = nullptr;
}

size_t openLife::io::TextFile::getSize()
{
	size_t size;
	fseek(this->fp , 0 , SEEK_END);
	size = ftell(this->fp);
	rewind(this->fp);
	return size;
}

void openLife::io::TextFile::writeContentIn(char** buffer, size_t size)
{
	size_t fileSize = this->getSize();

	if(size)
	{
		if(fileSize>=size) throw new openLife::Exception("OpenLife: buffer is not large enough to contain file content");
		memset(*buffer, 0, size);
	}
	else
	{
		size_t bufferSize = sizeof(char)*fileSize;
		*buffer = (char*) malloc(bufferSize);
		memset(*buffer, 0, bufferSize);
		if (!*buffer) throw new openLife::Exception("OpenLife: failed to allocate memory.");
	}


	size_t nbrChar = fread (*buffer, 1, fileSize, this->fp);
	if (nbrChar != fileSize) throw new openLife::Exception("OpenLife: read char don't match with file size %lu/%lu", nbrChar, size);
}
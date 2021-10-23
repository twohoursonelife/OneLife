//
// Created by olivier on 22/10/2021.
//

#include "settings.h"

#include <cstdio>
#include <cstdlib>
#include "server/application/exception/configuration.h"
#include "third_party/openLife/base/process/io/textFile.h"

int* server::Settings::test = nullptr;

void server::Settings::handle(int *test)
{
	server::Settings::test = test;
}

void server::Settings::getServerFileConfiguration(const char* filename)
{
	openLife::io::TextFile file;
	char* buffer = nullptr;
	file.open(filename);
	file.writeContentIn(&buffer);
	printf("\nFile content:\n%s", buffer);
	file.close();
	free(buffer);
}

void server::Settings::getBiomeIdFileConfiguration(const char *filename)
{
	openLife::io::TextFile file;
	char* buffer = nullptr;
	file.open(filename);
	file.writeContentIn(&buffer);
	printf("\nFile content:\n%s", buffer);
	file.close();
	free(buffer);
}
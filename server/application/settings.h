//
// Created by olivier on 22/10/2021.
//

#ifndef ONELIFE_SETTINGS_H
#define ONELIFE_SETTINGS_H

namespace server
{
	class Settings
	{
	public:
		static void handle(int* test);
		static void getServerFileConfiguration(const char* filename);
		static void getBiomeIdFileConfiguration(const char* filename);

	private:
		static int* test;
	};
}

#endif //ONELIFE_SETTINGS_H

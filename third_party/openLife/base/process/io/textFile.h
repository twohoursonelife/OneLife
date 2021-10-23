//
// Created by olivier on 23/10/2021.
//

#ifndef OPENLIFE_FILE_H
#define OPENLIFE_FILE_H

#include <cstdio>

namespace openLife::io
{
	class TextFile
	{
		public:
			TextFile();
			~TextFile();

			void open(const char* filename);
			void close();
			size_t getSize();

			void writeContentIn(char** buffer, size_t size=0);

		private:
			FILE* fp;
			char* buffer;
	};
}



#endif //ONELIFE_FILE_H

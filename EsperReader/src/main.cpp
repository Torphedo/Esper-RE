#include "main.h"
#include "deck.h"
#include "winAPI.h"

char* filepathptr;
std::string filepath;

int main()
{
	if (FileSelectDialog(fileTypes) != -1)
	{
		ParseDeckFile(filepathptr);

		std::cout << "\n\nPress any key to exit.\n";
		char _dummy = _getch();
		return 0;
	}
	else
	{
		return 1;
	}
}

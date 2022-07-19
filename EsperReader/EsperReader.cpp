#include <sys/stat.h>
#include <iostream>
#include <conio.h>
#include "deck.h"
using namespace std;

int main(int argc, char* argv[])
{
	struct stat meta;

	if (argc != 2)
	{
		cout << "Please drag a Phantom Dust deck file onto the program.\nPress any key to exit.\n";
		_getch();
		return -1;
	}
	else
	{
		stat(argv[1], &meta);
		cout << "The size of " << argv[1] << " is " << meta.st_size << " bytes." << endl;
		// In the future, I plan to check file extension / magic, and
		// call different parsers depending on the result.
		ParseDeckFile(argv[1]);
		cout << "\n\nPress any key to exit.\n";
		_getch();
		return 1;
	}
}

#include <sys/stat.h>
#include <string>
#include <iostream>
#include <conio.h>
#include "deck.h"
#include "alr.h"
using namespace std;
using std::string;

int main(int argc, char* argv[])
{
	struct stat meta;
	string file_path = argv[1];

	if (argc != 2)
	{
		cout << "Please drag & drop a Phantom Dust deck file or ALR file onto the program.\nPress any key to exit.\n";
		_getch();
		return -1;
	}

	stat(argv[1], &meta);
	cout << "The size of " << argv[1] << " is " << meta.st_size << " bytes.\n\n";

	// Get file extension
	size_t i = file_path.rfind('.', file_path.length());
	string FileExtension = file_path.substr(i + 1, file_path.length() - i);

	if (FileExtension == "alr")
	{
		ParseAlrFile(argv[1]);
	}
	else
	{
		ParseDeckFile(argv[1]);
	}

	cout << "\n\nPress any key to exit.\n";
	_getch();
	return 1;
}

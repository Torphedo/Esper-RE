#include <main.h>
#include <deck.h>
#include <alr.h>
#include <winAPI.h>

char* filepathptr;
std::string filepath;

int main()
{
	if (FileSelectDialog(fileTypes) != -1)
	{

		// Get file extension
		size_t i = filepath.rfind('.', filepath.length());
		std::string FileExtension = filepath.substr(i + 1, filepath.length() - i);

		if (FileExtension == "alr")
		{
			struct stat meta;
			stat(filepathptr, &meta);
			std::cout << "The size of " << filepath << " is " << meta.st_size << " bytes.\n\n";
			ParseAlrFile(filepathptr);
		}
		else
		{
			ParseDeckFile(filepathptr);
		}

		std::cout << "\n\nPress any key to exit.\n";
		char _dummy = _getch();
		return 0;
	}
	else
	{
		return 1;
	}
}

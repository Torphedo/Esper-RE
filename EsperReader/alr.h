#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

typedef struct ALR_Header
{
	int Magic;
	int HeaderSize;
	int Flags;
	int WhitespaceEndAddr;
	int InfoSectionsNum;
	int Unknown;
	char pad[8];
}alr_header;

void ParseAlrHeader(fstream& BinaryALR, char* filename)
{
	alr_header Header;

	BinaryALR.open(filename, ios::in | ios::binary);
	BinaryALR.read((char*)&Header, (sizeof(Header)));

	// Print file data
	cout << "Block Magic: " << Header.Magic;
	cout << "\nHeader Block Size: " << Header.HeaderSize;
	cout << "\nBlock Flags: " << Header.Flags;
	cout << "\nWhitespace End Address: " << Header.WhitespaceEndAddr;
	cout << "\n# of listed block offsets: " << Header.InfoSectionsNum;
	cout << "\nUnknown Value: " << Header.Unknown << "\n";

	// Dynamic int array with size = InfoSectionsNum. Each int is a pointer to a data block.
	int *array = new int[Header.InfoSectionsNum];
	BinaryALR.read((char*)array, (sizeof(int) * Header.InfoSectionsNum));
	for (int n = 0; n < 141; n++)
	{
		cout << "\nOffset " << (n + 1) << ": " << array[n];
	}
	delete[] array;
	return;
}

void ParseAlrFile(char* filename)
{
	fstream AlrFile;
	ParseAlrHeader(AlrFile, filename);
	AlrFile.close();
}

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

using namespace std;

typedef struct ALR_Header
{
	int ID;
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

	BinaryALR.open(filename, ios::in | ios::binary);  // Open file
	BinaryALR.read((char*)&Header, (sizeof(Header))); // Read bytes into ALR_Header struct

	// Print file data
	cout << "========== Header ==========\n\n";
	cout << "Block ID: " << Header.ID;
	cout << "\nHeader Block Size: " << Header.HeaderSize;
	cout << "\nBlock Flags: " << Header.Flags;
	cout << "\nWhitespace End Address: " << Header.WhitespaceEndAddr;
	cout << "\n# of listed block offsets: " << Header.InfoSectionsNum;
	cout << "\nUnknown Value: " << Header.Unknown << "\n\n";

	// Dynamic int array with size = InfoSectionsNum. Each int is a pointer to a data block.
	int *ptrarray = new int[Header.InfoSectionsNum];
	BinaryALR.read((char*)ptrarray, (sizeof(int) * Header.InfoSectionsNum));
	cout << "---------- Pointer Array ----------\n";
	for (int n = 0; n < Header.InfoSectionsNum; n++)
	{
		cout << "\nPointer " << (n + 1);

		// Different spacings for different #, to make everything print evenly
		if (n < 9) {
			cout << ":   0x";
		}
		else if (n < 99) {
			cout << ":  0x";
		}
		else {
			cout << ": 0x";
		}

		// Convert to hex, fill leading space with 0s, set back to decimal afterwards
		// so that the pointer # isn't in hex.
		cout << setfill('0') << setw(sizeof(ptrarray[n]) + 2) << hex << ptrarray[n] << dec;
	}
	delete[] ptrarray;
	return;
}

void ParseAlrFile(char* filename)
{
	fstream AlrFile;
	ParseAlrHeader(AlrFile, filename);
	AlrFile.close();
}

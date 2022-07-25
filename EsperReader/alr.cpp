#include "alr.h"

int ParseAlrHeader(fstream& BinaryALR, char* filename)
{
	alr_header Header;

	BinaryALR.open(filename, ios::in | ios::binary);  // Open file
	BinaryALR.read((char*)&Header, (sizeof(Header))); // Read bytes into ALR_Header struct

	if (sizeof(Header) + Header.InfoSectionsNum * 4 != Header.HeaderSize) {
		/*  This logic needs reworking later.What if there's an extra
			pointer in the array, but InfoSectionsNum isn't updated?
			This will only detect size errors if InfoSectionsNum is correct. */
		cout << "Fatal error: ALR header size mismatch!\n";
		cout << "\nExpected size: " << Header.HeaderSize << " bytes";
		cout << "\nActual size:   " << sizeof(Header) + (Header.InfoSectionsNum * 4) << " bytes";
		return -1;
	}

	// Print file data
	cout << "========== Header ==========\n\n";
	cout << "Block ID: " << hex << Header.ID << dec;
	cout << "\nHeader Block Size: " << Header.HeaderSize << " bytes";
	cout << "\nBlock Flags: " << Header.Flags;
	cout << "\nWhitespace End Address: 0x" << setfill('0') << setw(8) << hex << Header.WhitespaceEndAddr << dec;
	cout << "\nPointer Array Size: " << Header.InfoSectionsNum;
	cout << "\nUnknown Value: " << Header.Unknown << "\n\n";

	// Dynamic int array with size = InfoSectionsNum. Each int is a pointer to a data block.
	int* ptrarray = new int[Header.InfoSectionsNum];
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
	return 1;
}

int ParseBlock15_ALR(fstream& BinaryALR, char* filename)
{
	alr_block015 blockdata;
	BinaryALR.read((char*)&blockdata, (sizeof(blockdata))); // Read bytes into ALR_Block015 struct

	// Print file data
	cout << "\n\n========== 0x15 Block ==========\n\n";
	cout << "Block ID: " << hex << blockdata.ID << dec;
	cout << "\nBlock Size: " << blockdata.BlockSize << " bytes";
	cout << "\nData Block Array Size: " << blockdata.InfoSectionsNum;

	alr_datablock015 datablockarray;
	for (int n = 0; n < blockdata.InfoSectionsNum; n++)
	{
		cout << "\n\n----- Data Block " << n << " -----\n" << hex;
		BinaryALR.read((char*)&datablockarray, 28);
		cout << "Flags: " << datablockarray.Flags;
		cout << "\nUnknown 1: 0x" << datablockarray.Unknown1 << dec;
		cout << "\nConstant Zero: " << datablockarray.UnknownZero; // Always 0
		cout << "\nUnknown 2: 0x" << hex << datablockarray.Unknown2 << dec;
		cout << "\nUnknown 3: " << datablockarray.Unknown3; // Usually 0
		cout << "\nID: " << hex << datablockarray.ID;
		cout << "\nUnknown 4: " << datablockarray.Unknown4 << dec;
	}
	return 1;
}

int ParseBlock05_ALR(fstream& BinaryALR, char* filename)
{
	return 1;
}

void ParseAlrFile(char* filename)
{
	fstream AlrFile;
	ParseAlrHeader(AlrFile, filename);
	ParseBlock15_ALR(AlrFile, filename);
	AlrFile.close();
}

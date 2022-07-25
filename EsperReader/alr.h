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

typedef struct ALR_Block015
{
	int ID;
	int BlockSize;
	int InfoSectionsNum;
}alr_block015;

typedef struct ALR_DataBlock015
{
	int Flags; // Usually 01 00 04 00
	int Unknown1; // Maybe some sort of pointer???
	int UnknownZero; // This appears to always be 0.
	int Unknown2;
	int Unknown3; // Often but not always 0. See enm00a0.alr.

	// Unknown, but appears to be a unique identifier. Unique enough that you can search
	// the game's memory for this single word and only get matches from the ALR data.
	int ID;
	int Unknown4;
}alr_datablock015;

int ParseAlrHeader(fstream& BinaryALR, char* filename);
int ParseBlock15_ALR(fstream& BinaryALR, char* filename);
int ParseBlock05_ALR(fstream& BinaryALR, char* filename);
void ParseAlrFile(char* filename);

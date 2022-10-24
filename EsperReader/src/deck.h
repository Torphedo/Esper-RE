#pragma once

#include "main.h"

struct Deck
{
	char Header[4];
	char Name[16];
	short CardData[30];
	short SchoolCount;
	short Metadata; // Purpose unknown. Made up of 2 bytes, which seem to always be 0x00 or 0x40.
	int MissionClears;
	int MissionAttempts;
	int MultiplayerWins;
	int MultiplayerWinRate;
};

void ParseDeckFile(char* filename);

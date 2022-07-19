#include <iostream>
#include <fstream>
#include "deckSkillNames.h"

using namespace std;

typedef struct Deck
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
}deckbin;

void ParseDeckFile(char* filename)
{
	fstream DeckBinary;
	deckbin deck;

	DeckBinary.open(filename, ios::in | ios::binary);
	DeckBinary.read((char*)&deck, (sizeof(deck)));
	cout << "Name: " << deck.Name << "\nSchool Count: " << deck.SchoolCount << "\nUnknown Metadata: " << deck.Metadata;
	cout << "\nMission Clears: " << deck.MissionClears << "\nMission Attempts: " << deck.MissionAttempts;
	cout << "\nMultiplayer Wins: " << deck.MultiplayerWins << "\nMultiplayer Win Rate: " << deck.MultiplayerWinRate << "%\n";

	for (int n = 0; n < 30; n++)
	{
		if (n < 9)
		{
			cout << "\nCard #" << (n + 1) << ":   ";
		}
		else
		{
			cout << "\nCard #" << (n + 1) << ":  ";
		}
		printSkillNames(deck.CardData[n]);
	}

	DeckBinary.close();
	return;
}
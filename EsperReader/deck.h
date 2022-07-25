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

	DeckBinary.open(filename, ios::in | ios::binary); // Open file
	DeckBinary.read((char*)&deck, (sizeof(deck)));    // Read bytes into deck struct

	cout << "Name: " << deck.Name;
	cout << "\nSchool Count: " << deck.SchoolCount;
	cout << "\nUnknown Metadata: " << deck.Metadata;
	cout << "\nMission Clears: " << deck.MissionClears;
	cout << "\nMission Attempts: " << deck.MissionAttempts;
	cout << "\nMultiplayer Wins: " << deck.MultiplayerWins;
	cout << "\nMultiplayer Win Rate: " << deck.MultiplayerWinRate << "%\n";

	for (int n = 0; n < 30; n++)
	{
		cout << "\nCard #" << (n + 1); // (n + 1) is used because the counter starts at 0, not 1

		// Slightly different spacing to keep things aligned
		if (n < 9)
		{
			cout << ":   ";
		}
		else
		{
			cout << ":  ";
		}

		if (deck.CardData[n] == -1)
		{
			cout << "Aura Particle";
		}
		else if (deck.CardData[n] >= 394) // IDs 394 - 499 are all copies of Psycho Wave
		{
			cout << "Psycho Wave";
		}
		else
		{
			cout << SkillIDs[deck.CardData[n]]; // Print skill name from string array by ID
		}
	}

	DeckBinary.close();
	return;
}

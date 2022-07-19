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
	short Metadata; // Purpose unknown.
	char pad[16];
}deckbin;

void ParseDeckFile(char* filename)
{
	fstream DeckBinary;
	deckbin deck;

	DeckBinary.open(filename, ios::in | ios::binary);
	DeckBinary.read((char*)&deck, (sizeof(deck)));
	cout << "Name: " << deck.Name << "\nSchool Count: " << deck.SchoolCount << "\nUnknown Metadata: " << deck.Metadata << "\n";

	for (int n = 0; n < 30; n++)
	{
		cout << "\nCard #" << (n + 1) << ": ";
		printSkillNames(deck.CardData[n]);
	}

	DeckBinary.close();
	return;
}
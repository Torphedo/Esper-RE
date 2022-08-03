#include <deck.h>

void ParseDeckFile(char* filename)
{
	fstream DeckBinary;
	Deck deckbin;

	DeckBinary.open(filename, ios::in | ios::binary); // Open file
	DeckBinary.read((char*)&deckbin, (sizeof(deckbin)));    // Read bytes into deckbin struct

	cout << "Name: " << deckbin.Name;
	cout << "\nSchool Count: " << deckbin.SchoolCount;
	cout << "\nUnknown Metadata: " << deckbin.Metadata;
	cout << "\nMission Clears: " << deckbin.MissionClears;
	cout << "\nMission Attempts: " << deckbin.MissionAttempts;
	cout << "\nMultiplayer Wins: " << deckbin.MultiplayerWins;
	cout << "\nMultiplayer Win Rate: " << deckbin.MultiplayerWinRate << "%\n";

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

		if (deckbin.CardData[n] == -1)
		{
			cout << "Aura Particle";
		}
		else if (deckbin.CardData[n] >= 394) // IDs 394 - 499 are all copies of Psycho Wave
		{
			cout << "Psycho Wave";
		}
		else
		{
			cout << SkillIDs[deckbin.CardData[n]]; // Print skill name from string array by ID
		}
	}

	DeckBinary.close();
	return;
}
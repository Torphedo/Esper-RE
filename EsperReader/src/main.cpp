#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <iomanip>

#include "winAPI.h"
#include "deck.h"
#include "skill_names.h"

std::string filepath;

#define CARDS_MAX 30

void PrintDeckFile(deck_t deck)
{
	printf("Name: %s\n", deck.Name);
	printf("School Count: %hi\n", deck.SchoolCount);
	printf("Unknown Metadata: %hi\n", deck.Metadata);
	printf("Mission Clears: %i\n", deck.MissionClears);
	printf("Mission Attempts: %i\n", deck.MissionAttempts);
	printf("Multiplayer Wins: %i\n", deck.MultiplayerWins);
	printf("Multiplayer Win Rate: %i%%", deck.MultiplayerWinRate);

	printf("\n\n/////////////////////////////////////////////////////////////////////////////////\n");

	for (unsigned int i = 0; i < CARDS_MAX; i++)
	{
		std::cout << "|| Card #" << (i + 1); // (n + 1) is used because the counter starts at 0, not 1

		// Slightly different spacing to keep things aligned
		if (i < 9) { std::cout << ":  "; }
		else { std::cout << ": "; }

		int spacing = 32;

		if (deck.CardData[i] == -1) {
			std::cout << "Aura Particle";
			spacing -= (int)SkillIDs[0].size();
		}
		else if (deck.CardData[i] >= 394) // IDs 394 - 499 are all copies of Psycho Wave
		{
			std::cout << "Psycho Wave";
			spacing -= (int)SkillIDs[394].size();
		}
		else {
			std::cout << SkillIDs[deck.CardData[i]]; // Print skill name from string array by ID
			spacing -= (int)SkillIDs[deck.CardData[i]].size();
		}
		std::cout << std::setw(spacing) << std::setfill(' ');
		if (i % 2) { std::cout << "||" << "\n"; }
	}

	printf("/////////////////////////////////////////////////////////////////////////////////\n");

	return;
}

int main()
{
	if (FileSelectDialog() != -1)
	{
		deck_t deck = { 0 };

		FILE* DeckBinary = fopen(filepath.c_str(), "rb");
		if (DeckBinary)
		{
			fread(&deck, sizeof(deck_t), 1, DeckBinary);
			fclose(DeckBinary);
		}

		PrintDeckFile(deck);

		printf("\n\nPress any key to exit.\n");
		char _dummy = _getch();
		return 0;
	}
	else
	{
		return 1;
	}
}

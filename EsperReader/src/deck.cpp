#include <deck.h>
#include <deckSkillNames.h>

void ParseDeckFile(char* filename)
{
	FILE* DeckBinary;
	Deck deckbin;

	fopen_s(&DeckBinary, filename, "rb");
	if (DeckBinary)
	{
		fread_s(&deckbin, 100, 100, 1, DeckBinary);
		fclose(DeckBinary);
	}

	printf("Name: %s\n", deckbin.Name);
	printf("School Count: %hi\n", deckbin.SchoolCount);
	printf("Unknown Metadata: %hi\n", deckbin.Metadata);
	printf("Mission Clears: %i\n", deckbin.MissionClears);
	printf("Mission Attempts: %i\n", deckbin.MissionAttempts);
	printf("Multiplayer Wins: %i\n", deckbin.MultiplayerWins);
	printf("Multiplayer Win Rate: %i%%", deckbin.MultiplayerWinRate);

	printf("\n\n/////////////////////////////////////////////////////////////////////////////////\n");

	for (int n = 0; n < 30; n++)
	{
		std::cout << "|| Card #" << (n + 1); // (n + 1) is used because the counter starts at 0, not 1

		// Slightly different spacing to keep things aligned
		if (n < 9) { std::cout << ":  "; }
		else { std::cout << ": "; }

		int spacing = 32;

		if (deckbin.CardData[n] == -1) {
			std::cout << "Aura Particle";
			spacing -= (int)SkillIDs[0].size();
		}
		else if (deckbin.CardData[n] >= 394) // IDs 394 - 499 are all copies of Psycho Wave
		{
			std::cout << "Psycho Wave";
			spacing -= (int)SkillIDs[394].size();
		}
		else {
			std::cout << SkillIDs[deckbin.CardData[n]]; // Print skill name from string array by ID
			spacing -= (int)SkillIDs[deckbin.CardData[n]].size();
		}
		std::cout << std::setw(spacing) << std::setfill(' ');
		if (n % 2) { std::cout << "||" << "\n"; }
	}

	printf("/////////////////////////////////////////////////////////////////////////////////\n");

	return;
}

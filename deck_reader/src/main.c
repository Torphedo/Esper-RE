#include <stdio.h>
#include <conio.h>

#include "winAPI.h"
#include "skill_names.h"

typedef struct
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
}deck_t;

static void PrintDeckFile(deck_t deck)
{
	printf("Name: %s\n", deck.Name);
	printf("School Count: %hi\n", deck.SchoolCount);
	printf("Unknown Metadata: %hi\n", deck.Metadata);
	printf("Mission Clears: %i\n", deck.MissionClears);
	printf("Mission Attempts: %i\n", deck.MissionAttempts);
	printf("Multiplayer Wins: %i\n", deck.MultiplayerWins);
	printf("Multiplayer Win Rate: %i%%\n", deck.MultiplayerWinRate);

    printf("\n//============================================================================\\\\\n");

    static const short card_count = 30;

	for (unsigned int i = 0; i < card_count; i++)
    {
        printf("|| Card #%-2d", i + 1);

		if (deck.CardData[i] == -1) {
            printf(": %-26s", SkillIDs[0]);
		}
		else if (deck.CardData[i] >= 394) // IDs 394 - 499 are all copies of Psycho Wave
		{
			printf(": %-26s", SkillIDs[394]);
		}
		else {
            printf(": %-26s", SkillIDs[deck.CardData[i]]);
		}
		if (i % 2) { printf("||\n"); }
	}

	printf("\\\\============================================================================//\n");
}

int main()
{
    char* filepath = FileSelectDialog();
	if (filepath != NULL)
	{
		deck_t deck = { 0 };

		FILE* deck_file = fopen(filepath, "rb");
		if (deck_file)
		{
			fread(&deck, sizeof(deck_t), 1, deck_file);
			fclose(deck_file);
		}
		PrintDeckFile(deck);

		printf("\n\nPress any key to exit.\n");
		int _dummy = _getch(); // Pause
		return 0;
	}
	else
	{
		return 1;
	}
}

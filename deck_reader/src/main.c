#include <stdio.h>
#include <stdint.h>

#include "winAPI.h"
#include "skill_names.h"

typedef struct {
    char Header[4];
    char Name[16];
    int16_t CardData[30];
    uint16_t SchoolCount;
    uint16_t Metadata; // Purpose unknown. Made up of 2 bytes, which seem to always be 0x00 or 0x40.
    uint32_t MissionClears;
    uint32_t MissionAttempts;
    uint32_t MultiplayerWins;
    uint32_t MultiplayerWinRate;
}deck_t;

static void PrintDeckFile(deck_t deck) {
    printf("Name: %s\n", deck.Name);
    printf("School Count: %hi\n", deck.SchoolCount);
    printf("Unknown Metadata: %hi\n", deck.Metadata);
    printf("Mission Clears: %i\n", deck.MissionClears);
    printf("Mission Attempts: %i\n", deck.MissionAttempts);
    printf("Multiplayer Wins: %i\n", deck.MultiplayerWins);
    printf("Multiplayer Win Rate: %i%%\n", deck.MultiplayerWinRate);

    printf("\n//============================================================================\\\\\n");

    static const short card_count = 30;

    for (unsigned int i = 0; i < card_count; i++) {
	printf("|| Card #%-2d", i + 1);

	// ID of -1 is for Aura Particle
	if (deck.CardData[i] == -1) {
            printf(": %-26s", SkillIDs[0]);
	}
	// IDs 394 - 499 are all copies of Psycho Wave
	else if (deck.CardData[i] >= 394) {
	    printf(": %-26s", SkillIDs[394]);
	}
	else {
	    printf(": %-26s", SkillIDs[deck.CardData[i]]);
	}

	if (i % 2) { printf("||\n"); }
    }

    printf("\\\\============================================================================//\n");
}

int main(int argc, char** argv) {
    if (argc == 1) {
	printf("Please pass in a filename on the command-line or drag-and-drop a file onto the program.\n");
    }
    char* filepath = argv[1];
    if (filepath != NULL) {
	deck_t deck = {0};

	FILE* deck_file = fopen(filepath, "rb");
	if (deck_file) {
	    fread(&deck, sizeof(deck_t), 1, deck_file);
	    fclose(deck_file);
	}
	PrintDeckFile(deck);

	printf("\n\nPress Enter to exit.\n");
	char dummy = 0;
	scanf("%c", &dummy);
	return 0;
    }
    else {
	return 1;
    }
}

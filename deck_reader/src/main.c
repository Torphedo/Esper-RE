#include <stdio.h>

#include <common/int.h>
#include <formats/deck.h>

static const char* SkillIDs[395] = {
    #include "skill_names.txt"
};

void print_deck(deck d) {
    printf("name: %s\n", d.name);
    printf("School Count: %hi\n", d.school_count);
    printf("Unknown meta: %hi\n", d.meta);
    printf("Mission Clears: %i\n", d.mission_clears);
    printf("Mission Attempts: %i\n", d.mission_attempts);
    printf("Multiplayer Wins: %i\n", d.multiplayer_wins);
    printf("Multiplayer Win Rate: %i%%\n", d.multiplayer_win_rate);

    printf("\n//============================================================================\\\\\n");

    static const u16 card_count = 30;

    for (u32 i = 0; i < card_count; i++) {
	    printf("|| Card #%-2d", i + 1);

        // ID of -1 is for Aura Particle
        if (d.cards[i] == -1) {
            printf(": %-26s", SkillIDs[0]);
        }
        // IDs 394 - 499 are all copies of Psycho Wave
        else if (d.cards[i] >= 394) {
	        printf(": %-26s", SkillIDs[394]);
        }
        else {
	        printf(": %-26s", SkillIDs[d.cards[i]]);
        }
        if (i % 2) {
            printf("||\n");
        }
    }

    printf("\\\\============================================================================//\n");

    // Still unclear what exactly this field means. The only deck file I have
    // with a non-zero header is the "CLOSE_RANGE" file.
    if (d.header != 0) {
        printf("Non-zero header 0x%04X\n", d.header);
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("Please pass in a filename on the command-line or drag-and-drop a file onto the program.\n");
    }
    char* filepath = argv[1];

    deck d = {0};

    FILE* deck_file = fopen(filepath, "rb");
    if (deck_file) {
        fread(&d, sizeof(d), 1, deck_file);
        fclose(deck_file);
    }
    print_deck(d);
    
    printf("\n\nPress Enter to exit.\n");
    char dummy = 0;
    scanf("%c", &dummy);
    return 0;
}


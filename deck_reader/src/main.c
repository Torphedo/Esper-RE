#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <common/int.h>
#include <common/logging.h>
#include <formats/deck.h>

static const char* SkillIDs[395] = {
    #include "skill_names.txt"
};

void print_deck(deck d) {
    printf("1. Deck Name: %s\n", d.name);
    printf("2. School Count: %hi\n", d.school_count);
    printf("3. Unknown meta: %hi\n", d.meta);
    printf("4. Mission Clears: %i\n", d.mission_clears);
    printf("5. Mission Attempts: %i\n", d.mission_attempts);
    printf("6. Multiplayer Wins: %i\n", d.multiplayer_wins);
    printf("7. Multiplayer Win Rate: %i%%\n", d.multiplayer_win_rate);
    printf("8. Skills: \n");

    printf("\n//==============================================================================\\\\\n");

    static const u16 skill_count = 30;

    for (u32 i = 0; i < skill_count; i++) {
	printf("|| Skill #%-2d", i + 1);

        // ID of -1 is for Aura Particle
        if (d.skills[i] == -1) {
            printf(": %-26s", SkillIDs[0]);
        }
        // IDs 394 - 499 are all copies of Psycho Wave
        else if (d.skills[i] >= 394) {
	    printf(": %-26s", SkillIDs[394]);
        }
        else {
	    printf(": %-26s", SkillIDs[d.skills[i]]);
        }
        if (i % 2) {
            printf("||\n");
        }
    }

    printf("\\\\==============================================================================//\n");

    // Still unclear what exactly this field means. The only deck file I have
    // with a non-zero header is the "CLOSE_RANGE" file.
    if (d.header != 0) {
        LOG_MSG(warning, "[Non-zero] ");
    }
    printf("header: 0x%04X\n", d.header);
}

void pause() {
    printf("Press Enter to continue.");
    char dummy = 0;
    scanf("%c", &dummy);
}

void clear_stdin() {
    int c = 0;
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}

void version() {
    const char* version_string = "1.1.1";
    const char* url = "https://github.com/Torphedo/Esper-RE";
    printf("deck_reader (Esper-RE tools) v%s\n", version_string);
    printf("Open-source @ %s\n", url);
    printf("Written by Torphedo\n");
}

void help() {
    LOG_MSG(info, "Drag-and-drop a file onto the program, or use it on command-line:\n");
    printf("\tdeck_reader [deck file]\n");
    LOG_MSG(info, "Commands:\n");
    printf("\t1-8: edit a property of the deck (each field is numbered on-screen)\n");
    printf("\tw: overwrite the deck file with the new data\n");
    printf("\tq: quit\n");
}

int main(int argc, char** argv) {
    enable_win_ansi(); // Allow ANSI escape codes on Windows
    if (argc == 1) {
        LOG_MSG(error, "Pass in a filename on command-line, or drag-and-drop a file onto the program.\n\n");
        pause(); // Make the message visible to people who double-click the EXE
        return 1;
    }
    if (strcmp(argv[1], "--help") == 0) {
        help();
        return 0;
    } else if (strcmp(argv[1], "--version") == 0) {
        version();
        return 0;
    }
    char* filepath = argv[1];
    deck d = {0};

    FILE* deck_file = fopen(filepath, "rb");
    if (deck_file == NULL) {
        LOG_MSG(error, "Couldn't open %s as a file\n", filepath);
        return 1;
    }
    fread(&d, sizeof(d), 1, deck_file);
    fclose(deck_file);

    bool running = true;
    while (running) {
        print_deck(d);
        LOG_MSG(info, "Commands:\n");
        printf("\t1-8: edit a property of the deck (use the numbers above)\n");
        printf("\tw: overwrite the deck file with the new data\n");
        printf("\tq: quit\n");
        LOG_MSG(info, "Enter a command, or type \"q\" and then Enter to quit.\n");

        // Accounts for everything we just printed, plus the input prompt.
        u32 printed_lines = 33;
        printf("> ");
        char c = 0;
        scanf("%c", &c);
        clear_stdin();

        switch (c) {
        case '1':
            printf("Enter new deck name [limit 15 characters]: ");
            // The number in the format specifier stops an overflow, and the
            // "\n" in brackets makes it scan until a newline (instead of going
            // until whitespace).
            scanf("%15[^\n]s", d.name);
            break;
        case '2':
            printf("Enter new school count: ");
            u32 school_count = 0; // Temp var to handle int size issues
            if (scanf("%d", &school_count) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            d.school_count = school_count;
            break;
        case '3':
            printf("Enter new value for the unknown metadata (hex): 0x");
            u32 meta = 0; // Temp var to handle int size issues
            if (scanf("%hx", &meta) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            d.meta = meta;
            break;
        case '4':
            printf("Enter new mission clear count: ");
            if (scanf("%d", &d.mission_clears) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            break;
        case '5':
            printf("Enter new mission attempts count: ");
            if (scanf("%d", &d.mission_attempts) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            break;
        case '6':
            printf("Enter new multiplayer win count: ");
            if (scanf("%d", &d.multiplayer_wins) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            break;
        case '7':
            printf("Enter new multiplayer win rate (in %%): ");
            if (scanf("%d", &d.multiplayer_win_rate) != 1) {
                LOG_MSG(warning, "No number read.\n");
                pause();
                printed_lines += 2;
                break;
            }
            break;
        case '8':
            printf("Enter the skill number you want to change: ");
            s32 idx = -1;
            scanf("%d", &idx);
            if (idx <= 0 || idx > 30) {
                LOG_MSG(warning, "Invalid number entered (must be in range [1 - 30])\n");
                pause();
                printed_lines += 2;
                break;
            }
            clear_stdin();
            
            u32 id = -2;
            printf("Enter the new skill ID: ");
            scanf("%d", &id);
            printed_lines++;
            if (id == -2) {
                LOG_MSG(warning, "No skill ID entered\n");
                pause();
                printed_lines += 2;
                break;
            }
            d.skills[idx - 1] = id;

            break;
        case 'w':
            {
                FILE* f = fopen(filepath, "wb");
                if (f == NULL) {
                    LOG_MSG("Failed to open deck file %s\n", filepath);
                    pause();
                    printed_lines += 2;
                    break;
                }
                fwrite(&d, sizeof(d), 1, f);
                fclose(f);
            }
            LOG_MSG(info, "Saved deck file as %s\n", filepath);
            pause();
            printed_lines += 2;
            break;
        case 'q':
            running = false;
            return 0;
        default:
            LOG_MSG(error, "Unknown command.\n");
            pause();
            printed_lines += 2;
            break;
        }

        // Clear if we did a scan this cycle
        if (c >= '0' && c <= '9') {
            printed_lines++;
            clear_stdin();
        }
        printf("\033[%dA", printed_lines); // Go up to where we started printing
        printf("\033[0J"); // Clear until end of screen
    }


    return 0;
}


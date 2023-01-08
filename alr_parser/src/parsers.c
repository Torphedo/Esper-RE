#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "data_structures.h"
#include "parsers.h"
#include "lib/arena.h"
#include "lib/logging.h"

bool info_mode = false;

arena_t* global_arena = 0;

// File pointer to print animation data as text to a file.
FILE* animation_out;

void set_info_mode()
{
	info_mode = true;
}

// Writes each distinct section of an ALR to separate files on disk
bool split_alr(char* alr_filename)
{
	FILE* alr = fopen(alr_filename, "rb");
	if (alr != NULL)
	{
		header_t header;

		// Read header
		fread(&header, sizeof(header_t), 1, alr);
        uint32_t* pointer_array = malloc(header.pointer_array_size * sizeof(uint32_t));
		if (pointer_array != NULL)
		{
			// Read in pointer array
			fread(pointer_array, sizeof(uint32_t), header.pointer_array_size, alr);

            // Write each large block of data to a separate file.
            for (unsigned int i = 0; i < header.pointer_array_size; i++)
            {
                // Length between the current and next pointer.
                size_t size;
                if (i < header.pointer_array_size - 1) {
                    size = pointer_array[i + 1] - pointer_array[i];
                }
                else {
                    size = header.unknown_section_ptr - pointer_array[i];
                }

                char* buffer = malloc(size);
                fseek(alr, (long) pointer_array[i], SEEK_SET);
                fread(buffer, size, 1, alr);

                // There's no user input here and the filename can never be >15 characters
                char filename[15];
                sprintf(filename, "%u.bin", i);
                FILE* dump = fopen(filename, "wb");
                fwrite(buffer, size, 1, dump);
                fclose(dump);
            }
			free(pointer_array);
		}
		else {
			log_error(CRITICAL, "Failed to allocate for pointer array!\n");
			return false;
		}
		fclose(alr);
	}
	else {
		log_error(CRITICAL, "Failed to open %s!\n", alr_filename);
	}
	return true;
}

// Reads 0x5 animation / mesh blocks
static void block_animation(arena_t* arena, unsigned int texture_buffer_ptr)
{
	if (!info_mode) {
		fprintf(animation_out, "\n=== Animation Block ===\n");
	}
    uint64_t block_start_pos = arena->pos;
    arena->pos += 4;
	anim_header* header = arena_alloc(arena, sizeof(anim_header));

	if (header->ArraySize1 > 0) {
		if (!info_mode) {
			fprintf(animation_out, "Array Type 1 (animation frames):\n\n");
		}
		switch (header->array_width_2) {
		case 8: {
			anim_array_type1* float_array = arena_alloc(arena, header->ArraySize1 * sizeof(anim_array_type1));
			if (!info_mode) {
				for (unsigned int i = 0; i < header->ArraySize1; i++) {
					fprintf(animation_out, "%02u: %f\n", (unsigned int)float_array[i].index, float_array[i].X);
				}
			}
			break;
		}
		case 12: {
			anim_array_type2* float_array = arena_alloc(arena, header->ArraySize1 * sizeof(anim_array_type2));
			if (!info_mode) {
				for (unsigned int i = 0; i < header->ArraySize1; i++) {
					fprintf(animation_out, "%02u: %f %f\n", (unsigned int)float_array[i].index, float_array[i].X, float_array[i].Y);
				}
			}
			break;
		}
		case 16: {
			anim_array_type3* float_array = arena_alloc(arena, header->ArraySize1 * sizeof(anim_array_type3));
			if (!info_mode) {
				for (unsigned int i = 0; i < header->ArraySize1; i++) {
					fprintf(animation_out, "%02u: %f %f %f\n", (unsigned int)float_array[i].index, float_array[i].X, float_array[i].Y, float_array[i].Z);
				}
			}
			break;
		}
		default: {
			log_error(CRITICAL, "Unknown animation data structure: element size %hi\n", header->array_width_2);
			exit(1);
		}
		}
	}

	if (header->ArraySize2 != 0) {
		if (!info_mode) {
			fprintf(animation_out, "\nArray Type 2 (unknown):\n");
		    char* secondary_array = arena_alloc(arena, header->ArraySize2 * header->array_width_1);

            for (unsigned int i = 0; i < header->ArraySize2; i++)
            {
                fprintf(animation_out, "\n%02u: ", (uint8_t)secondary_array[i * header->array_width_1]);
                for (unsigned short j = 0; j < header->array_width_1 - 1; j++) {
                    fprintf(animation_out, "%02hhx ", secondary_array[i * header->array_width_1 + j + 1]);
                }
            }
            fprintf(animation_out, "\n");
        }
	}
    arena->pos = block_start_pos + header->size; // Jump to next block
}

// Reads 0x10 blocks and uses them to read out RGBA data in the ALR to TGA files on disk.
static void block_texture(arena_t* arena, unsigned int texture_buffer_ptr)
{
    texture_buffer_ptr += 0x4000;
	// TGA header with all relevant settings until the shorts for width & height.
	static const char tga_header[12] = {
			0x00, 0x00, 0x02, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
	};
	static const char layout_settings[2] = {
			0x20, // 32 bits per pixel (RGBA, 1 byte per color)
			0x20  // Start pixels from top left instead of bottom left
	};

	// We subtract 4 because 4 bytes were already read for the block ID
	uint64_t block_start_pos = arena->pos;
    arena->pos += 4;

	texture_block_header* header = arena_alloc(arena, sizeof(texture_block_header));

    if (info_mode) { printf("\n=== Texture Info ===\n"); }

    log_error(INFO, "DDS Count: %d Image Count: %d\n\n", header->DDS_count, header->texture_count);

    // Skip over some filenames that don't appear to correspond to any image data.
    // (0x20 each, plus an extra string matching the name of the ALR).
    static const unsigned int alr_name_size = 0x10;
    static const unsigned int dds_filename_size = 0x20;

    arena_push(arena, alr_name_size);
    for (unsigned int i = 0; i < header->DDS_count; i++)
    {
        char* dds_name = arena_alloc(arena, dds_filename_size);
        log_error(INFO, "Texture %2d: %s\n", i, dds_name);
    }
    printf("\n");

	// Skip over what appears to be dimension metadata which is duplicated in the
	// next section in a more useful format representable as a struct.
	arena->pos += header->DDS_count * 0x14;

	for (unsigned int i = 0; i < header->texture_count; i++)
	{
		texture_header* texture = arena_alloc(arena, sizeof(texture_header));
		log_error(INFO, "Image %2d, %-32s: Width %4hi, Height %4hi, Texture %2d\n", i, texture->filename, texture->width, texture->height, texture->index);
        if (!info_mode)
		{
			uint64_t cached_pos = arena->pos; // Store current position so that we can jump to the pixel data
			size_t texture_size = (size_t)texture->width * (size_t)texture->height * 4;
            arena->pos = texture_buffer_ptr;
            char* texture_data = arena_alloc(arena, texture_size);

            // Writing out an uncompressed TGA file
            FILE* tex_out = fopen((const char*) &texture->filename, "wb");
            fwrite(&tga_header, sizeof(tga_header), 1, tex_out);
            fwrite(&texture->width, sizeof(short), 1, tex_out);
            fwrite(&texture->height, sizeof(short), 1, tex_out);
            fwrite(&layout_settings, sizeof(layout_settings), 1, tex_out);
            fwrite(texture_data, (size_t)texture->width * (size_t)texture->height * 4, 1, tex_out);
            fclose(tex_out);

            arena->pos = cached_pos;
			// Increment texture buffer location so that the next read begins where we left off
			texture_buffer_ptr += texture_size;
		}
	}

    arena->pos = block_start_pos + header->size; // Set position to end of block
}

// Allows us to skip over any block that doesn't have a parser yet
static void block_skip(arena_t* arena, unsigned int texture_buffer_ptr)
{
	uint32_t size = *(uint32_t*)(arena_pos(arena) + sizeof(uint32_t)); // Read size from block
	// Skip to the end of the block
    arena->pos += size;
}

// Array of function pointers. When an ALR block is read, the appropriate function
// is called using its ID as an index into this array. This is used as a replacement
// for a very long switch statement.
static void (*function_ptrs[23]) (arena_t*, unsigned int) = {
        block_skip,         // 0x0
        block_skip,
        block_skip,
        block_skip,
        block_skip,
        block_animation,    // 0x5
        block_skip,
        block_skip,
        block_skip,
        block_skip,
        block_skip,         // 0xA
        block_skip,
        block_skip,
        block_skip,
        block_skip,
        block_skip,
        block_texture,      // 0x10
        block_skip,
        block_skip,
        block_skip,
        block_skip,
        block_skip,         // 0x15
        block_skip
};

bool block_parse_all(char* alr_filename)
{

    if (!info_mode) {
        animation_out = fopen("animation_out.txt", "wb");
    }

    // Read header
    FILE* alr = fopen(alr_filename, "rb");
    if (alr != NULL) {

        struct stat st;
        if (stat(alr_filename, &st) != 0) {
            log_error(CRITICAL, "block_parse_all(): Failed to get file metadata for ALR %s", alr_filename);
            exit(-1);
        }
        unsigned int filesize = st.st_size;

        // Allocate and read the entire file at once to minimize disk latency
        global_arena = create_arena(filesize, RESERVE, 0);
        fread(global_arena->base_addr, filesize, 1, alr);
        fclose(alr);

        header_t* header = arena_alloc(global_arena, sizeof(header_t));
        unsigned int* pointer_array = arena_alloc(global_arena, header->pointer_array_size * sizeof(unsigned int));
        if (info_mode)
        {
            log_error(INFO, "Unknown Buffer Address: 0x%x\n", header->unknown_section_ptr);
            log_error(INFO, "File Count: %d\n\n", header->pointer_array_size);
            for (unsigned int i = 0; i < header->pointer_array_size; i++)
            {
                log_error(INFO, "File %u: 0x%x\n", i, pointer_array[i]);
            }
        }
        for (unsigned int i = 0; i < header->pointer_array_size; i++)
        {
            unsigned int current_block_id = *(global_arena->base_addr + pointer_array[i]);
            global_arena->pos = pointer_array[i];
            while (current_block_id != 0)
            {
                if (current_block_id > 0x16)
                {
                    log_error(CRITICAL, "Invalid block ID 0x%x in file %d (offset 0x%x)!\n", current_block_id, i, global_arena->pos);
                    exit((int)current_block_id);
                }
                if (info_mode)
                {
                    log_error(INFO, "0x%x block at 0x%x\n", current_block_id, global_arena->pos);
                }
                (*function_ptrs[current_block_id]) (global_arena, header->unknown_section_ptr);
                current_block_id = *((unsigned int*) arena_pos(global_arena)); // Read next block's ID
            }
        }
        destroy_arena(global_arena);
    }
    else {
        log_error(CRITICAL, "Couldn't open %s.\n", alr_filename);
        return false;
    }
    if (!info_mode) {
        fclose(animation_out);
    }
    return true;
}

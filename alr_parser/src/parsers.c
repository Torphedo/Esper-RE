#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "data_structures.h"
#include "parsers.h"
#include "images.h"
#include "arena.h"
#include "logging.h"

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
        if (animation_out == NULL) {
            animation_out = fopen("animation_out.txt", "wb");
        }
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
	uint64_t block_start_pos = arena->pos;
    // We add 4 because 4 bytes were already read for the block ID
    arena->pos += 4;
    if (info_mode) { printf("\n=== Texture Info ===\n"); }

	texture_block_header* header = arena_alloc(arena, sizeof(texture_block_header));
    log_error(INFO, "DDS Count: %d Image Count: %d\n\n", header->DDS_count, header->texture_count);

    // Skip over some filenames that don't appear to correspond to any image data.
    // (0x20 each, plus an extra string matching the name of the ALR).
    static const uint8_t alr_name_size = 0x10;
    static const uint8_t dds_filename_size = 0x20;

    arena_push(arena, alr_name_size);

    uint32_t pointer_count = arena->base_addr[0x10];
    // Get contents of 0x15 sub-blocks
    block_sub_0x15* data_array = (block_sub_0x15*) (arena->base_addr + sizeof(header_t) + (pointer_count * sizeof(uint32_t) + sizeof(block_0x15_header)));
    char* dds_names = arena_alloc(arena, dds_filename_size * header->DDS_count);
    dds_meta* ddsMeta = arena_alloc(arena, header->DDS_count * sizeof(dds_meta));
    texture_header* textures = arena_alloc(arena, sizeof(texture_header) * header->texture_count);

    texture_info* info = malloc(sizeof(texture_info) * header->DDS_count);

    for (uint32_t i = 0; i < header->DDS_count; i++)
    {
        uint32_t pixel_count = textures[i].width * textures[i].height;
        uint32_t texture_id = textures[i].index;
        uint32_t res_size = 0;
        if (texture_id == header->DDS_count - 1)
        {
            res_size = arena->size - data_array[texture_id].data_ptr;
        }
        else {
            res_size = data_array[texture_id + 1].data_ptr - data_array[texture_id].data_ptr;
        }

        info[i].filename = &dds_names[i * dds_filename_size];
        info[i].bits_per_pixel = (res_size / pixel_count) * 8;
        info[i].mipmap_count = ddsMeta[i].mipmap_count;
        info[i].image_data = (char*) arena->base_addr + texture_buffer_ptr + data_array[texture_id].data_ptr;
        info[i].width = textures[i].width;
        info[i].height = textures[i].height;

        log_error(INFO, "Texture %2d (%d mipmaps): %s bpp: %d\n", i, info[i].mipmap_count, &dds_names[i * dds_filename_size], info[i].bits_per_pixel);

        if (data_array[i].pad != 0)
        {
            log_error(WARNING, "Discovered anomaly in format! Apparent padding in 0x15 member was 0x%x at index %d!\n", data_array[i].pad, i);
        }
        if (data_array[i].flags != 0x00040001)
        {
            log_error(WARNING, "Discovered anomaly in format! Flag value in 0x15 member was 0x%x at index %d!\n", data_array[i].flags, i);
        }

        write_tga(info[i]);
    }
    printf("\n");
    free(info);

	for (unsigned int i = 0; i < header->texture_count; i++)
	{
		log_error(INFO, "Image %2d, %-32s: Width %4hi, Height %4hi, Texture %2d\n", i, textures[i].filename, textures[i].width, textures[i].height, textures[i].index);
	}

    arena->pos = block_start_pos + header->size; // Set position to end of block
}

static void block_15(arena_t* arena, unsigned int big_buffer_pointer)
{
    uint64_t block_start_pos = arena->pos;
    block_0x15_header* header = arena_alloc(arena, sizeof(block_0x15_header));

    block_sub_0x15* data_array = arena_alloc(arena, sizeof(block_sub_0x15) * header->struct_array_size);

    printf("\n");
    log_error(INFO, "File Metadata\n");
    log_error(INFO, "Array Size: %d\n", header->struct_array_size);

    for (uint32_t i = 0; i < header->struct_array_size; i++)
    {
        log_error(INFO, "pointer: 0x%08x unknown: 0x%08x unknown2: 0x%08x ID: 0x%08x unknown3: 0x%08x\n", data_array[i].data_ptr, data_array[i].unknown, data_array[i].unknown2, data_array[i].ID, data_array[i].unknown3);
        if (data_array[i].pad != 0)
        {
            log_error(WARNING, "Discovered anomaly in format! Apparent padding in 0x15 member was 0x%x at index %d!\n", data_array[i].pad, i);
        }
        if (data_array[i].flags != 0x00040001)
        {
            log_error(WARNING, "Discovered anomaly in format! Flag value in 0x15 member was 0x%x at index %d!\n", data_array[i].flags, i);
        }
    }
    arena->pos = block_start_pos + header->block_size;
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
        block_15,         // 0x15
        block_skip
};

bool block_parse_all(char* alr_filename)
{
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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <arena.h>
#include <logging.h>

#include "parsers.h"
#include "alr.h"
#include "images.h"

bool info_mode = false;

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
		block_layout header;

		// Read header
		fread(&header, sizeof(block_layout), 1, alr);
        uint32_t* pointer_array = malloc(header.offset_array_size * sizeof(uint32_t));
		if (pointer_array != NULL)
		{
			// Read in pointer array
			fread(pointer_array, sizeof(uint32_t), header.offset_array_size, alr);

            // Write each large block of data to a separate file.
            for (unsigned int i = 0; i < header.offset_array_size; i++)
            {
                // Length between the current and next pointer.
                int32_t size;
                if (i < header.offset_array_size - 1) {
                    size = pointer_array[i + 1] - pointer_array[i];
                }
                else {
                    size = header.resource_offset - pointer_array[i];
                }

                if ( size < 0)
                {
                    log_error(WARNING, "split_alr(): Adjusting for negative block size at block %d.\n", i);
                    size *= -1;
                }

                if (size > 0x00FFFFF)
                {
                    log_error(CRITICAL, "split_alr(): Block size at block %d was unreasonably high (%d bytes)!\n", i, size);
                    return false;
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
                free(buffer);
            }
			free(pointer_array);
		}
		else {
			log_error(CRITICAL, "split_alr(): Failed to allocate for pointer array of size %d\n", header.offset_array_size);
			return false;
		}
		fclose(alr);
	}
	else {
		log_error(CRITICAL, "split_alr(): Failed to open %s\n", alr_filename);
	}
	return true;
}

// Reads 0x5 animation / mesh blocks
static void block_animation(arena_t* arena)
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
			break;
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
static void block_texture(arena_t* arena, unsigned int texture_buffer_ptr, image_type format)
{
	uint64_t block_start_pos = arena->pos;

	texture_metadata_header* header = arena_alloc(arena, sizeof(texture_metadata_header));
    log_error(INFO, "Surface Count: %d Image Count: %d\n\n", header->DDS_count, header->texture_count);

    // DDS filenames + the mystery data attached to them are 0x20 long each
    char* dds_names = arena_alloc(arena, 0x20 * header->DDS_count);

    // Get contents of 0x15 sub-blocks
    resource_entry* resources = (resource_entry*) (arena->base_addr + arena->base_addr[0x4] + sizeof(resource_layout_header));
    surface_info* surface = arena_alloc(arena, header->DDS_count * sizeof(surface_info));
    texture_header* textures = arena_alloc(arena, sizeof(texture_header) * header->texture_count);

    for (uint32_t i = 0; i < header->DDS_count; i++)
    {
        uint32_t pixel_count = textures[i].width * textures[i].height;
        uint32_t texture_id = textures[i].index;
        uint32_t res_size = 0;
        if (texture_id == header->DDS_count - 1)
        {
            res_size = arena->size - resources[texture_id].data_ptr;
        }
        else {
            res_size = resources[texture_id + 1].data_ptr - resources[texture_id].data_ptr;
        }

        uint8_t bits_per_pixel = (res_size / pixel_count) * 8;

        log_error(INFO, "Surface %2d (%s): %2d mipmap(s), estimated %2d bpp\n", i, &dds_names[i * 0x20], surface[i].mipmap_count, bits_per_pixel);

        if (resources[i].pad != 0)
        {
            log_error(DEBUG, "block_texture(): Resource meta anomaly, apparent padding at index %d was 0x%x\n", i, resources[i].pad);
        }
        if (resources[i].flags != 0x00040001)
        {
            log_error(DEBUG, "block_texture(): Resource meta anomaly, flag at index %d was 0x%x\n", i, resources[i].flags);
        }

        if (!info_mode) {
            texture_info info = {
                    .filename = &dds_names[i * 0x20],
                    .bits_per_pixel = bits_per_pixel,
                    .mipmap_count = surface[i].mipmap_count,
                    .image_data = (char *) arena->base_addr + texture_buffer_ptr + resources[texture_id].data_ptr,
                    .width = textures[i].width,
                    .height = textures[i].height,
                    .format = format
            };
            write_texture(info);
        }
    }
    printf("\n");

	for (unsigned int i = 0; i < header->texture_count; i++)
	{
		log_error(INFO, "%-32s: Width %4hi, Height %4hi (Surface %2d)\n", textures[i].filename, textures[i].width, textures[i].height, textures[i].index);
	}

    arena->pos = block_start_pos + header->size; // Set position to end of block
}

// Allows us to skip over any block that doesn't have a parser yet
static void block_skip(arena_t* arena)
{
	uint32_t size = *(uint32_t*)(arena_pos(arena) + sizeof(uint32_t)); // Read size from block
	// Skip to the end of the block
    arena->pos += size;
}

bool block_parse_all(char* alr_filename, flags options)
{
    FILE* alr = fopen(alr_filename, "rb");
    if (alr != NULL)
    {
        struct stat st;
        if (stat(alr_filename, &st) != 0) {
            log_error(CRITICAL, "block_parse_all(): Failed to get file metadata for %s", alr_filename);
            return false;
        }
        unsigned int filesize = st.st_size;

        // Allocate and read the entire file at once to minimize disk latency
        arena_t* arena = create_arena(filesize, ALLOCATE_ALL, 0);
        if (arena == NULL)
        {
            log_error(CRITICAL, "block_parse_all(): Failed to create arena with a size of %d bytes for the file %s\n", filesize, alr_filename);
        }
        fread(arena->base_addr, filesize, 1, alr);
        fclose(alr);
        log_error(DEBUG, "block_parse_all(): Loaded %s\n", alr_filename);

        uint8_t image_format = DDS;
        if (options.tga) { image_format = TGA; }

        block_layout* header = arena_alloc(arena, sizeof(block_layout));
        unsigned int* pointer_array = arena_alloc(arena, header->offset_array_size * sizeof(unsigned int));
        if (info_mode)
        {
            log_error(INFO, "Resource Section Offset: 0x%x\n", header->resource_offset);
            log_error(INFO, "File Count: %d\n\n", header->offset_array_size);
            for (unsigned int i = 0; i < header->offset_array_size; i++)
            {
                log_error(INFO, "File %u: 0x%x\n", i, pointer_array[i]);
            }
        }
        for (unsigned int i = 0; i < header->offset_array_size; i++)
        {
            unsigned int current_block_id = *(arena->base_addr + pointer_array[i]);
            arena->pos = pointer_array[i];
            while (current_block_id != 0)
            {
                if (current_block_id > 0x16)
                {
                    log_error(WARNING, "block_parse_all(): Invalid block ID 0x%x at 0x%x (internal file %d, %s)!\n", current_block_id, arena->pos, i, alr_filename);
                    return false;
                }
                if (info_mode)
                {
                    log_error(DEBUG, "block_parse_all(): 0x%x block at 0x%x\n", current_block_id, arena->pos);
                }

                switch (current_block_id)
                {
                    case 0x5:
                        block_animation(arena);
                        break;
                    case 0x10:
                        block_texture(arena, header->resource_offset, image_format);
                    default:
                        block_skip(arena);
                }

                current_block_id = *((unsigned int*) arena_pos(arena)); // Read next block's ID
            }
        }
        destroy_arena(arena);
    }
    else {
        log_error(CRITICAL, "block_parse_all(): Couldn't open %s.\n", alr_filename);
        return false;
    }
    if (!info_mode && animation_out != NULL) {
        fclose(animation_out);
    }
    return true;
}

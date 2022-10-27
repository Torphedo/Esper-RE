#include <stdlib.h>
#include <stdio.h>

#include "data_structures.h"
#include "parsers.h"

// Writes each distinct section of an ALR to separate files on disk
int split_alr(char* alr_filename)
{
	FILE* alr = fopen(alr_filename, "rb");
	if (alr != NULL)
	{
		header_t header;
		header_array pointers;
		// Read header
		fread(&header, sizeof(header_t), 1, alr);
		pointers.pointer_array = malloc(header.pointer_array_size * sizeof(unsigned int));
		if (pointers.pointer_array != NULL)
		{
			fread(pointers.pointer_array, sizeof(unsigned int), header.pointer_array_size, alr);

			// Write each large block of data to a separate file. 
			if (header.pointer_array_size > 0)
			{
				for (unsigned int i = 0; i < header.pointer_array_size; i++)
				{
					// There's no user input here and the filename is never > 15 characters
					char filename[15];
					sprintf(filename, "%u.bin", i);
					FILE* dump = fopen(filename, "wb");

					// Length between the current and next pointer. I'd prefer not to have
					// an if statement in the loop like this, but this should work fine for now.
					size_t size = 0;
					if (i < header.pointer_array_size - 1) {
						size = pointers.pointer_array[i + 1] - pointers.pointer_array[i];
					}
					else {
						size = header.unknown_section_ptr - pointers.pointer_array[i];
					}
					char* buffer = malloc(size);
					if (buffer != NULL) {
						fseek(alr, pointers.pointer_array[i], SEEK_SET);
						fread(buffer, size, 1, alr);
						fwrite(buffer, size, 1, dump);
						free(buffer);
					}

					fclose(dump);
				}
			}
		}
		else {
			printf("Failed to allocate for pointer array!\n");
		}
		fclose(alr);
	}
}

// Array of function pointers. When an ALR block is read, it executes a function
// using its ID as an index into this array. This is basically just a super
// efficient switch statement for all blocks.
void (*function_ptrs[23]) (FILE*, unsigned int) = {
	skip_block, // 0
	skip_block,
	skip_block,
	skip_block,
	skip_block,
	skip_block, // 5
	skip_block,
	skip_block,
	skip_block,
	skip_block,
	skip_block, // 0xA
	skip_block,
	skip_block,
	skip_block,
	skip_block,
	skip_block,
	texture_description, // 0x10
	skip_block,
	skip_block,
	skip_block,
	skip_block,
	skip_block, // 0x15
	skip_block
};

void parse_by_block(char* alr_filename, unsigned int info_mode)
{
	header_t header = {0};
	header_array pointers;

	// Read header
	FILE* alr = fopen(alr_filename, "rb");
	if (alr != NULL) {
		fread(&header, sizeof(header_t), 1, alr);
		pointers.pointer_array = malloc(header.pointer_array_size * sizeof(unsigned int));
		if (pointers.pointer_array != NULL)
		{
			fread(pointers.pointer_array, sizeof(unsigned int), header.pointer_array_size, alr);

			if (header.pointer_array_size > 0)
			{
				for (unsigned int i = 0; i < header.pointer_array_size; i++)
				{
					fseek(alr, pointers.pointer_array[i], SEEK_SET);
					unsigned int current_block_id = 0;
					fread(&current_block_id, sizeof(unsigned int), 1, alr);
					while (current_block_id != 0)
					{
						(*function_ptrs[current_block_id]) (alr, header.unknown_section_ptr);
						fread(&current_block_id, sizeof(unsigned int), 1, alr);
					}
				}
			}
		}
		fclose(alr);
	}
}

// Reads 0x10 blocks and uses them to read out RGBA data in the ALR to TGA files on disk.
void texture_description(FILE* alr, unsigned int texture_buffer_ptr)
{
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
	long block_start_pos = ftell(alr) - 4;

	texture_block_header header = { 0 };
	fread(&header, sizeof(texture_block_header), 1, alr);

	// Skip over some filenames that don't appear to correspond to any image data.
	// (0x20 each, plus an extra string matching the name of the ALR).
	static const unsigned int alr_name_size = 0x10;
	static const unsigned int alr_filename_size = 0x20;
	fseek(alr, header.image_array_size1 * alr_filename_size + alr_name_size, SEEK_CUR);

	// Skip over what appears to be dimension metadata which is duplicated in the
	// next section in a more useful format representable as a struct.
	fseek(alr, header.image_array_size1 * 0x14, SEEK_CUR);

	for (unsigned int i = 0; i < header.image_array_size2; i++)
	{
		texture_header texture = { 0 };
		fread(&texture, sizeof(texture_header), 1, alr);
		printf("%s: Width %hi, Height %hi\n", texture.filename, texture.width, texture.height);
		long cached_pos = ftell(alr); // Store current pos so we can jump to the pixel data
		size_t texture_size = (size_t)texture.width * (size_t)texture.height * 4;
		char* texture_data = malloc(texture_size);
		if (texture_data != NULL) {
			fseek(alr, texture_buffer_ptr, SEEK_SET); // Jump to texture buffer
			fread(texture_data, texture_size, 1, alr);

			// Writing out an uncompressed TGA file
			FILE* tex_out = fopen(&texture.filename, "wb");
			fwrite(&tga_header, sizeof(tga_header), 1, tex_out);
			fwrite(&texture.width, sizeof(short), 1, tex_out);
			fwrite(&texture.height, sizeof(short), 1, tex_out);
			fwrite(&layout_settings, sizeof(layout_settings), 1, tex_out);
			fwrite(texture_data, (size_t)texture.width * (size_t)texture.height * 4, 1, tex_out);
			fclose(tex_out);
			free(texture_data);
		}
		fseek(alr, cached_pos, SEEK_SET);
		// Increment texture buffer location so that the next read begins where we left off
		texture_buffer_ptr += texture_size;
	}

	fseek(alr, block_start_pos + header.size, SEEK_SET); // Set position to end of block
}

// Allows us to skip over any block that doesn't have a parser yet
void skip_block(FILE* alr, unsigned int texture_buffer_ptr)
{
	printf("Unimplemented block parser, skipping...\n");
	unsigned int size = 0;
	fread(&size, sizeof(unsigned int), 1, alr);
	size -= sizeof(unsigned int) * 2;
	// Skip past any remaining data in the block
	fseek(alr, size, SEEK_CUR);
	return;
}

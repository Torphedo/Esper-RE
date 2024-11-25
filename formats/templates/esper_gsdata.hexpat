struct header {
	u32 filesize[[color("00FF00")]];
	u32 text_pointer[[color("00FF00")]];
	// There's a lot more here, but I'm
	// skipping it to work on text stuff.
};

struct test_meta_element {
	u32 index [[color("FFFFFF")]];
	u32 element_1 [[color("00FF00")]];
	u32 element_2 [[color("0000FF")]];
};

struct text_meta {
	u32 unknown [[color("FF0000")]];
	u32 elements_maybe;
	u32 block_size[[color("00FF00")]];
	u32 unknown_2[[color("FF0000")]];
	u32 skill_count [[color("00FF00")]];
	test_meta_element arr[skill_count];
};

header header @0x0;

text_meta meta @header.text_pointer + 0x19860;


// Deck file format for use in ImHex hex editor.
// By Torphedo, Nov. 11 2022.

struct deck
{
	char name[16] [[color("00FF00")]];
	u16 cards[30] [[color("0000FF")]];
	u16 school_count [[color("00FF00")]];
	u16 unknown_meta[2] [[hidden]];
	u16 mission_clears;
	u16 mission_attempts;
	u16 multiplayer_wins;
	u16 multiplayer_win_rate;
};

// Reads into above struct starting at 0x4, because the first 4 bytes are always 0
deck deck @0x4;

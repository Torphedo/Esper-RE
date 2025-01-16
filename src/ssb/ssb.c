#include <formats/pd_common.h>

#include "ssb.h"

decoded_text decode_text(ssb_functable_entry val) {
    decoded_text out = {0};
    decode_single32(&out.data[0], val.text1);
    decode_single32(&out.data[6], val.text2);
    return out;
}


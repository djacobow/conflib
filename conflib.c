#include <string.h>
#include <stdio.h>

#include "conflib.h"

#define BITS_TO_MASK(b) (((uint64_t)1 << b) - 1)

static const uint32_t conflib_size_to_mask[_conflib_size_max] = {
    BITS_TO_MASK(0),
    BITS_TO_MASK(1),
    BITS_TO_MASK(2),
    BITS_TO_MASK(4),
    BITS_TO_MASK(8),
    BITS_TO_MASK(16),
    BITS_TO_MASK(32),
};

static conflib_size_e conflib_val_size(uint32_t v) {
    uint32_t m = 0x80000000;
    uint32_t i = 0;
    while (i<32) {
        if (v & m) {
			break;
		}
        m >>= 1;
        i++;
	}
    uint32_t bits = 32 - i;
    // printf("bits %u %x\n", bits, v);
    if (!bits) {
        return conflib_size_nil;
    } else if (bits <= 1) {
        return conflib_size_1b;
	} else if (bits <= 2) {
        return conflib_size_2b;
	} else if (bits <= 4) {
        return conflib_size_4b;
	} else if (bits <= 8) {
        return conflib_size_1B;
	} else if (bits <= 16) {
        return conflib_size_2B;
	}
    return conflib_size_4B;
}

void conflib_init(conflib_context_t *c, uint8_t *buf, uint32_t len) {
    c->lib = buf;
    c->lib_len = len;
}

static uint32_t _conflib_size_to_len(conflib_size_e s) {
    return s ? 0x1 << (s-1) : 0;
}

static conflib_entry_t _conflib_make_entry(uint16_t id, uint32_t val) {
    conflib_size_e size = conflib_val_size(val);
    uint32_t bitlen = _conflib_size_to_len(size);
    id = id & ((1 << CONFLIB_ADDR_BITS) -1);
    conflib_entry_t entry = {};
    entry.id = id;
    entry.size = size;
    entry.val = val;
    entry.buf = entry.id;
    entry.buf |= entry.size << CONFLIB_ADDR_BITS;
    entry.buf |= (uint64_t)entry.val  << (CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS);
    entry.len = CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS + bitlen;
    entry.valid = true;
    entry.bit_idx = 0;
    return entry;
}

static bool _conflib_get_next_entry(const conflib_context_t *c, uint32_t bit_idx, conflib_entry_t *e) {
    if (bit_idx < (c->lib_len* 8)) {
        uint32_t byte_idx = bit_idx >> 3;
        memcpy(&e->buf, c->lib + byte_idx, 8);
        uint32_t shf = bit_idx & 0x7;
        e->bit_idx = bit_idx;
        e->buf >>= shf;
        e->id   = e->buf & ((1 << CONFLIB_ADDR_BITS) -1);
        e->size = (e->buf >> CONFLIB_ADDR_BITS) & BITS_TO_MASK(CONFLIB_SIZE_BITS);
        uint32_t bitlen = _conflib_size_to_len(e->size);
        e->len = CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS + bitlen;
        e->val  = (e->buf >> (CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS)) & BITS_TO_MASK(bitlen);
        if (e->id) {
            e->valid = true;
            return false;
        }
	}
    e->valid = false;
    return true;
}
conflib_entry_t conflib_find_entry(const conflib_context_t *c, uint16_t search_id) {
    uint32_t lib_len_bits = c->lib_len * 8;
    uint32_t lib_idx_bits = 0;
    conflib_entry_t entry = {};
    while (lib_idx_bits < lib_len_bits) {
        bool done = _conflib_get_next_entry(c, lib_idx_bits, &entry);
        if (entry.id == search_id) {
            entry.valid = true;
            break;
        } else if (entry.id == 0) {
            entry.valid = false;
            break;
        }
        lib_idx_bits += entry.len;
	}
    return entry;
};


static bool _conflib_append_entry(conflib_context_t *c, uint32_t bit_idx, const conflib_entry_t *e) {

    if ((bit_idx + e->len) < (c->lib_len * 8)) {
        uint32_t byte_idx = bit_idx >> 3;
        uint32_t shf = bit_idx & 0x7;
        uint64_t buf = 0;
        memcpy(&buf, c->lib + byte_idx, 8);
        uint64_t mask = BITS_TO_MASK(e->len);
        mask <<= shf;
        buf &= ~mask;
        buf |= ((e->buf << shf) & mask);
        memcpy(c->lib + byte_idx, &buf, 8);
		return true;
    }
    return false;
}

void _conflib_bitmemcpy(void *to_base, uint32_t to_bit_idx, void *from_base, uint32_t from_bit_idx, uint32_t bits_n) {

    uint32_t bits_copied = 0;
    uint8_t *from_ptr = (uint8_t *)from_base + (from_bit_idx >> 3);
    uint8_t *to_ptr   = (uint8_t *)to_base + (to_bit_idx   >> 3);

    while (bits_copied < bits_n) {
		uint32_t from_shf = from_bit_idx & 0x7;
		uint32_t to_shf   = to_bit_idx & 0x7;

        uint16_t buf = 0;
        memcpy(&buf, from_ptr, 2);
        buf >>= from_shf;
        // printf("from_shift %d from_buf 0x%04x\n", from_shf, buf);
        buf <<= to_shf;
        //printf("to_shift   %d from_buf 0x%04x\n", to_shf, buf);
        uint16_t buf_msk = 0xff << to_shf;
        uint16_t to_buf = 0;
        memcpy(&to_buf, to_ptr, 2);
        //printf("to_shift   %d to_Buf   0x%04x\n", to_shf, to_buf);
        to_buf &= ~buf_msk;
        to_buf |= (buf & buf_msk);
        memcpy(to_ptr, &to_buf, 2);
        bits_copied += 8;
        from_ptr++;
        to_ptr++;
    }
};

static bool _conflib_remove_entry(conflib_context_t *c, const conflib_entry_t *e) {
    if (e->valid) {
        conflib_entry_t last = conflib_find_entry(c, 0);
        if (e->bit_idx != last.bit_idx) {
            uint32_t bits_n = last.bit_idx - (e->bit_idx + e->len);
            _conflib_bitmemcpy(c->lib, e->bit_idx, c->lib, e->bit_idx + e->len, bits_n);
            uint32_t new_end_bit_idx = e->bit_idx + bits_n;
            uint32_t new_end_byte_idx = new_end_bit_idx >> 3;
		    uint32_t new_end_shift = new_end_bit_idx & 0x7;
            uint32_t msk = BITS_TO_MASK(8) << new_end_shift;
            c->lib[new_end_byte_idx] &= ~msk;
            int32_t buf_bytes_rem = c->lib_len - new_end_byte_idx;
            if (buf_bytes_rem > 0) {						 
                memset(c->lib + new_end_byte_idx + 1, 0, buf_bytes_rem -1);
            }
		    return true;
		}
    }
    return false;
}

bool conflib_remove_id(conflib_context_t *c, uint16_t id) {
    conflib_entry_t entry = conflib_find_entry(c, id);
    return _conflib_remove_entry(c, &entry);
}

bool conflib_add_entry(conflib_context_t *c, uint16_t id, uint32_t nval) {
    if (id == 0) {
        // cannot set id 0
        return false;
    }
    conflib_entry_t new_entry = _conflib_make_entry(id, nval);
    conflib_entry_t existing = conflib_find_entry(c, id);
    if (false) {
        conflib_show_entry("add_entry, new entry", false, &new_entry);
        conflib_show_entry("add_entry, existing entry", false, &existing);
    }

    if (existing.valid) {
	if (existing.size == new_entry.size) {
            return _conflib_append_entry(c, existing.bit_idx, &new_entry);
        } else {
            _conflib_remove_entry(c, &existing);
            conflib_entry_t new_last = conflib_find_entry(c, 0);
            return _conflib_append_entry(c, new_last.bit_idx, &new_entry);
        }
    } else {
        uint32_t bit_idx_next = existing.bit_idx;
        if (existing.id != 0) {
            bit_idx_next += existing.len;
        }
        return _conflib_append_entry(c, bit_idx_next, &new_entry);
    }

    return false;
}

void conflib_show_entry(const char *s, bool show_header, const conflib_entry_t *e) {
    if (s) {
        show_header = false;
    }
    if (show_header) {
        printf(" ID   Value (d)   Val (x)   vlen   tlen        bidx\n");
        printf("---  ----------  --------  -----  -----  ----------\n");
    }
    uint32_t vlen = _conflib_size_to_len(e->size);
    uint32_t tlen = vlen + CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS;
    if (s) {
        printf("%s: %u, %u, %08x, %u, %u, %u\n",
            s, e->id, e->val, e->val, vlen, tlen, e->bit_idx);
    } else {
        printf("%3u  %10u  %08x  %5u  %5u  %10u\n",
            e->id, e->val, e->val, vlen, tlen, e->bit_idx);
    }
}

void conflib_show_all(const conflib_context_t *c) {
    uint32_t lib_len_bits = c->lib_len * 8;
    uint32_t lib_idx_bits = 0;
    conflib_entry_t entry = {};
    uint32_t ttlen = 0;
    uint32_t tvlen = 0;
    bool print_header = true;
    while (lib_idx_bits < lib_len_bits) {
        bool done = _conflib_get_next_entry(c, lib_idx_bits, &entry);
        lib_idx_bits += entry.len;
        uint32_t vlen = _conflib_size_to_len(entry.size);
        uint32_t tlen = vlen + CONFLIB_ADDR_BITS + CONFLIB_SIZE_BITS;
        conflib_show_entry(NULL, print_header, &entry);
        print_header = false;
        tvlen += vlen;
        ttlen += tlen;
        if (done) {
            break;
        }
    }
    printf("---  ----------  --------  -----  -----  ----------\n");
    printf("                 Totals:   %5u  %5u\n", tvlen, ttlen);
};

uint32_t conflib_get_value(const conflib_context_t *c, uint16_t search_id) {
    conflib_entry_t entry = conflib_find_entry(c, search_id);
    if (entry.valid) {
        return entry.val;
    }
    return 0;
}

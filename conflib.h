#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONFLIB_ADDR_BITS (8)
#define CONFLIB_SIZE_BITS (3)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum conflib_size_e {
    conflib_size_nil= 0,
    conflib_size_1b = 1,
    conflib_size_2b = 2,
    conflib_size_4b = 3,
    conflib_size_1B = 4,
    conflib_size_2B = 5,
    conflib_size_4B = 6,
    _conflib_size_max,
} conflib_size_e;

typedef struct conflib_entry_t {
    uint64_t buf;
    conflib_size_e size;
    uint32_t val;
    uint16_t id;
    uint32_t len;
    bool valid;
    uint32_t bit_idx;
} conflib_entry_t;

typedef struct conflib_context_t {
    uint8_t *lib;
    uint32_t lib_len;
} conflib_context_t;

void conflib_init(conflib_context_t *c, uint8_t *buf, uint32_t len);
void conflib_show_entry(const char *s, bool header, const conflib_entry_t *e);
bool conflib_add_entry(conflib_context_t *c, uint16_t id, uint32_t nval);
// returns an entry for the addr if found, otherwise the entry of the last item in the list
conflib_entry_t conflib_find_entry(const conflib_context_t *c, uint16_t search_id);
// returns the value if found, otherwise 0
uint32_t conflib_get_value(const conflib_context_t *c, uint16_t search_id);

void conflib_show_all(const conflib_context_t *c);
bool conflib_remove_id(conflib_context_t *c, uint16_t id);

// a bit-smart memcpy, not optimized but hopefully works! It can copy from
// any bit alignment to any other bit alignment
void _conflib_bitmemcpy(void *to_base, uint32_t to_bit_idx, void *from_base, uint32_t from_bit_idx, uint32_t bits_n);

#ifdef __cplusplus
}
#endif

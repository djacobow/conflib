#include "conflib.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string.h>

int test_bitmemcpy() {
    #define ARR_SIZE (1024)

    for (uint32_t shift_a =0; shift_a < 24; shift_a++) {
        uint32_t shift_b = 24 - shift_a;
         
        uint8_t d_in[ARR_SIZE] = {};
        for (uint32_t i=0; i<ARR_SIZE; i++) { d_in[i] = i; };

        uint8_t d_out[ARR_SIZE];
        uint8_t d_check[ARR_SIZE];
        memcpy(d_check, d_in + 3, ARR_SIZE-3);

        _conflib_bitmemcpy(d_out, 0, d_in,  shift_a, ARR_SIZE * 8);
        _conflib_bitmemcpy(d_out, 0, d_out, shift_b, ARR_SIZE * 8);

        int32_t errors = memcmp(d_out, d_check, ARR_SIZE-3);
        if (errors) {
            printf("memcpy failed\n");
            for (uint32_t i=0;i<ARR_SIZE-3;i++) {
            }
            return errors;
        }
    }

    for (uint32_t shift_a =0; shift_a < 24; shift_a++) {
         
        uint8_t d_in[ARR_SIZE] = {};
        for (uint32_t i=0; i<ARR_SIZE; i++) { d_in[i] = i; };

        uint8_t d_out_1[ARR_SIZE + 1];
        uint8_t d_out_2[ARR_SIZE + 1];
        uint8_t d_check[ARR_SIZE + 1];

        memcpy(d_check, d_in, ARR_SIZE);

        _conflib_bitmemcpy(d_out_1, shift_a, d_in,  0,   ARR_SIZE*8);
        _conflib_bitmemcpy(d_out_2, 0, d_out_1, shift_a, ARR_SIZE*8);

        int32_t errors = memcmp(d_out_2, d_check, ARR_SIZE-3);
        if (errors) {
            printf("memcpy failed\n");
            for (uint32_t i=0;i<ARR_SIZE-3;i++) {
            }
            return errors;
        }
    }
    return 0;
}

int test_basic() {    //
    uint8_t lib[256] = {};

    conflib_context_t c = {};
    conflib_init(&c, lib, 256);
 
    // simple append
    conflib_add_entry(&c, 0xaa, 0xdeadbeef);
    conflib_add_entry(&c, 0xbb, 0xcafebabe);
    conflib_add_entry(&c, 0xcc, 0xf00d);
    conflib_add_entry(&c, 0xdd, 0x1);
    conflib_add_entry(&c, 0xee, 0x1);
    conflib_add_entry(&c, 0xff, 0x1);
    conflib_add_entry(&c, 0x10, 0x0);
    conflib_add_entry(&c, 0x11, 0x0);
    conflib_add_entry(&c, 0x12, 0x11111111);
    conflib_add_entry(&c, 0x13, 0x2);
    conflib_add_entry(&c, 0x14, 0x3333);
    conflib_add_entry(&c, 0x15, 0x4);
    conflib_add_entry(&c, 0x16, 0x555555);
    conflib_add_entry(&c, 0x17, 0x66666666);
    conflib_add_entry(&c, 0x18, 0x777);
    conflib_show_all(&c);
    
    // change in place
    conflib_add_entry(&c, 0xaa, 0xd00db00b);
    conflib_add_entry(&c, 0x12, 0x10101010);
    conflib_show_all(&c);

    // remove a few
    conflib_remove_id(&c, 0xaa);
    conflib_remove_id(&c, 0x14);
    conflib_remove_id(&c, 0xff);
    conflib_show_all(&c);

    // update a few _not_ in place
    conflib_add_entry(&c, 0x16, 0x5);
    conflib_add_entry(&c, 0x14, 0x33333333);
    conflib_add_entry(&c, 0x15, 0x44);
    conflib_show_all(&c);

    FILE *f = fopen("c_out.dat", "wb");
    fwrite(lib, 1, 256, f);
    fclose(f);
    return 0;
}

int test_random() {

    int errors = 0;

    for (uint32_t iters=0; iters<50; iters++) {
        uint8_t lib[1400] = {};
        conflib_context_t c = {};
        conflib_init(&c, lib, 1400) ;
        std::map<uint16_t, uint32_t> ref;
        for (uint32_t inserts=0; inserts<1500; inserts++) {
            uint16_t new_idx = rand() & 0xff;
            if (new_idx == 0) {
                continue;
            }
            uint32_t new_val = (rand() & 0xffff) << 16;
            switch (rand() & 0x3) {
                case 0: new_val &= 0xf; break;
                case 1: new_val &= 0xff; break;
                case 2: new_val &= 0xffff; break;
                default:
                    break;
            }
            new_val |= (rand() & 0xffff);
            ref[new_idx] = new_val;
            if (!conflib_add_entry(&c, new_idx, new_val)) {
                printf("Failed to add id %02x val %08x\n", new_idx, new_val);
            }
        }
        for (auto item: ref) {
            uint32_t val = conflib_get_value(&c, item.first);
            if (val != item.second) {
                printf("Error id %02x, ref %08x, conflib %08x\n", item.first, item.second, val);
                errors++;
            }
        }
        conflib_show_all(&c);
    }

    return errors;
}



int main(int argc, const char *argv[]) {
    int errors = test_bitmemcpy();
    if (errors) {
        printf("ERROR -- bitmemcpy fail\n");
        return errors;
    }
    errors = test_basic();
    if (errors) {
        printf("ERROR -- basic fail\n");
        return errors;
    }

    errors = test_random();
    if (errors) {
        printf("ERROR -- random fail\n");
        return errors;
    }
    printf("PASS\n");
}


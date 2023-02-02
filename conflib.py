#!/usr/bin/env python3

import struct, json, binascii

def bits_to_mask(b):
    return ((1 << b) -1)


class Conflib():
    ADDR_BITS = 8
    SIZE_BITS = 3

    def __init__(self, fname=None):
        self.members = {}
        if fname is not None:
            self.readFromFile(fname)

    def __getValSize(self, v):
        m = 0x80000000
        i = 0;
        while i<32:
            if v & m:
                break
            m = m >> 1
            i += 1
        bits = 32 - i
        if bits == 0:
            return 0
        elif bits == 1:
            return 1
        elif bits == 2:
            return 2
        elif bits <= 4:
            return 3
        elif bits <= 8:
            return 4
        elif bits <= 16:
            return 5
        return 6

    def __size_to_len(self, s):
        return 0x1 << (s-1) if s else 0

    def get(self, id):
        return self.members.get(id,None)

    def getValue(self, id):
        return self.members.get(id,{'value':0})['value']

    def getAll(self):
        return self.members

    def remove(self, id):
        if id in self.members:
            del self.members[id]

    def add(self, id, val=0):
        size = self.__getValSize(val)
        self.members[id] = {
            'value': val,
            'valid': True,
            'size': size,
            'total_len': self.ADDR_BITS + self.SIZE_BITS + self.__size_to_len(size)
        }

    def readFromBytes(self, data):
        data += bytes(8) 

        def getNext(bit_idx, data):
            bytes_idx = bit_idx >> 3
            buf = bytearray(data[bytes_idx: bytes_idx+8])
            (buf_int,) = struct.unpack('<Q', buf)
            buf_int = buf_int >> (bit_idx & 0x7)
            id = buf_int & bits_to_mask(self.ADDR_BITS)
            size = buf_int >> self.ADDR_BITS & bits_to_mask(self.SIZE_BITS)
            bitlen = self.__size_to_len(size)
            total_len = self.ADDR_BITS + self.SIZE_BITS + bitlen;
            value = (buf_int >> (self.ADDR_BITS + self.SIZE_BITS)) &  bits_to_mask(bitlen)
            return {
                'bit_idx': bit_idx,
                'byte_idx': bytes_idx,
                'id': id,
                #'id_hex': hex(id),
                'size': size,
                #'val_len': bitlen,
                'total_len': total_len,
                'value': value,
                #'val_hex': hex(value),
                'valid': id != 0,
            }


        bit_idx = 0;
        while bit_idx < (len(data)*8):
            entry = getNext(bit_idx, data)
            if entry['valid']:
                self.members[entry['id']] = entry
                bit_idx = entry['bit_idx'] + entry['total_len']
            else:
                break

    def readFromFile(self, fname):
        with open(fname, 'rb') as ifh:
            data = ifh.read()
            return self.readFromBytes(data)
       
    def serialize(self):

        def makeOne(m):
            size = self.__getValSize(m['value'])
            buf_int = m['id'] & bits_to_mask(self.ADDR_BITS)
            buf_int = buf_int | size << self.ADDR_BITS
            buf_int = buf_int | m['value'] << (self.ADDR_BITS + self.SIZE_BITS)
            total_len = self.ADDR_BITS + self.SIZE_BITS + self.__size_to_len(size)
            return buf_int, total_len

        bit_pos = 0
        data = bytearray(8)

        for (id, mem) in self.members.items():
            (buf_int, total_len) = makeOne(mem)
            bytes_pos = bit_pos >> 3;
            while len(data) < (bytes_pos + 8):
                data += bytearray(8)
            buf_exist, = struct.unpack('<Q', data[bytes_pos:bytes_pos+8])
            mask = bits_to_mask(total_len)
            mask = mask << (bit_pos & 0x7)
            buf_exist = buf_exist & ~mask
            buf_exist = buf_exist | ((buf_int << (bit_pos & 0x7)) & mask)
            buf_exist_bytes = struct.pack('<Q', buf_exist)
            data[bytes_pos:bytes_pos+8] = buf_exist_bytes
            bit_pos += total_len

        return data

    def show(self):
        tvlen = 0
        ttlen = 0
        print(' ID       Value   Hex Val  Vsize  Tsize')
        print('---  ----------  --------  -----  -----')
        for (id, m) in sorted(self.members.items()):
            val = m.get('value',0)
            vlen = self.__size_to_len(m.get('size',0))
            tlen = self.ADDR_BITS + self.SIZE_BITS + vlen
            print(f'{id:-3d}  {val:-10d}  {val:08x}  {vlen:-5d}  {tlen:-5d}')
            tvlen += vlen
            ttlen += tlen
        print('---  ----------  --------  -----  -----')
        print(f'                  Totals:  {tvlen:-5d}  {ttlen:-5d}')
        tvavg = tvlen / len(self.members.keys())
        ttavg = ttlen / len(self.members.keys())
        print(f'                  Avg:     {tvavg:4.2f}  {ttavg:4.2f}')



def conflib_loads(b: bytes) -> dict:
    c = Conflib()
    c.readFromBytes(b)
    return { x['id']: x['value'] for x in c.getAll().values() }

def conflib_saves(d: dict) -> bytes:
    c = Conflib()
    for (k, v) in d.items():
        c.add(k, v)
    return c.serialize()

if __name__ == '__main__':
    import sys

    c = Conflib('c_out.dat')
    c.show()
    s = c.serialize()
    print(binascii.hexlify(s))

    d = Conflib()
    d.readFromBytes(s)

    errors = 0
    for c_id, c_m in c.getAll().items():
        d_m = d.get(c_m['id'])
        for k in ('id', 'value', 'size', 'valid'):
            if d_m[k] != c_m[k]:
                print(f'Error mismatch {c_m}, {d_m}')
                errors += 1

    d.show()

    print(json.dumps(conflib_loads(s), indent=2))
    sys.exit(errors)


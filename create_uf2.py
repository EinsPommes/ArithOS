#!/usr/bin/env python3
import struct
import sys

def create_uf2(binary_file, output_file):
    with open(binary_file, 'rb') as f:
        data = f.read()
    
    uf2_data = b''
    block_size = 256
    total_blocks = (len(data) + block_size - 1) // block_size
    
    for i in range(total_blocks):
        start = i * block_size
        end = min(start + block_size, len(data))
        block_data = data[start:end]
        
        if len(block_data) < block_size:
            block_data += b'\x00' * (block_size - len(block_data))
        
        header = struct.pack('<IIIIIIII',
            0x0A324655,
            0x9E5D5157,
            0x00002000,
            i,
            total_blocks,
            0x10000000,
            256,
            i
        )
        
        magic_end = struct.pack('<I', 0x0AB16F30)
        uf2_data += header + block_data + magic_end
    
    with open(output_file, 'wb') as f:
        f.write(uf2_data)
    
    print(f'UF2 file created: {output_file}')

if __name__ == '__main__':
    create_uf2('build/arithmos.bin', 'build/arithmos.uf2')

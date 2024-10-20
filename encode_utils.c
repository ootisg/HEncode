#include "encode_utils.h"

void BitStream_init_empty(BitStream* stream, int max_size) {
    stream->data = malloc(max_size * sizeof(uint8_t));
    stream->position = 0;
}

void BitStream_init_filled(BitStream* stream, int data_size, uint8_t* data) {
    stream->data = malloc(data_size * sizeof(uint8_t));
    memcpy(stream->data, data, data_size);
    stream->position = 0;
}

void BitStream_write(BitStream* stream, uint32_t data, int num_bits) {
    uint32_t src_mask = 0x01 << (num_bits - 1);
    while (src_mask != 0x00) {
        int dst_byte = stream->position / 8;
        uint8_t dst_mask = 0x80 >> (stream->position % 8);
        if (data & src_mask) {
            stream->data[dst_byte] |= dst_mask;
        } else {
            stream->data[dst_byte] &= (~dst_mask);
        }
        src_mask >>= 1;
        stream->position++;
    }
}

void BitStream_write_str(BitStream* stream, char* str) {
    int i = 0;
    char curr;
    while (curr = str[i++]) {
        BitStream_write(stream, curr, 8);
    }
    BitStream_write(stream, 0, 8);
}

void BitStream_write_chars(BitStream* stream, char* chars, int num_chars) {
    int i;
    for (i = 0; i < num_chars; i++) {
        BitStream_write(stream, chars[i], 8);
    }
}

uint32_t BitStream_read(BitStream* stream, int num_bits) {
    int result = 0;
    int i;
    for (i = 0; i < num_bits; i++) {
        int src_byte = stream->position / 8;
        int src_mask = 0x80 >> (stream->position % 8);
        result <<= 1;
        if (stream->data[src_byte] & src_mask) {
            result |= 0x01;
        }
        stream->position++;
    }
    return result;
}

void BitStream_read_str(BitStream* stream, char* buf, int buf_len) {
    int i;
    for (i = 0; i < buf_len; i++) {
        uint8_t curr_char = BitStream_read(stream, 8);
        buf[i] = curr_char;
        if (!curr_char) {
            break;
        }
    }
    if (i == buf_len) {
        buf[i - 1] = '\0';
    }
}

void BitStream_read_chars(BitStream* stream, char* buf, int num_chars) {
    int i;
    for (i = 0; i < num_chars; i++) {
        buf[i] = BitStream_read(stream, 8);
    }
}

void BitStream_save(BitStream* stream, char* filepath) {
    FILE* f = fopen(filepath, "wb");
    uint32_t num_bytes = stream->position / 8 + (stream->position % 8 ? 1 : 0);
    uint32_t i;
    fwrite(stream->data, num_bytes, sizeof(char), f);
    fclose(f);
}

void BitStream_load(BitStream* stream, char* filepath) {
    FILE* f = fopen(filepath, "rb");
    int c = fgetc(f);
    while (c != EOF) {
        BitStream_write(stream, c, 8);
        c = fgetc(f);
    }
}

void BitStream_print(BitStream* stream) {
    int i;
    for (i = 0; i < stream->position / 8 + 1; i++) {
        printf("%02x", stream->data[i]);
    }
    printf("\n");
}

void EncodedChar_init(EncodedChar* ec, char symbol, int length, int encoding) {
    ec->raw_symbol = symbol;
    ec->encoded_len = length;
    ec->encoded_symbol = encoding;
}

void EncodedChar_push_bit(EncodedChar* ec, int bit) {
    ec->encoded_symbol <<= 1;
    if (bit) {
        ec->encoded_symbol |= 0x01;
    }
    ec->encoded_len++;
}

int EncodedChar_pop_bit(EncodedChar* ec) {
    int bit = ec->encoded_symbol & 0x01;
    ec->encoded_symbol >>= 1;
    ec->encoded_len--;
    return bit;
}

// int main() {
//     BitStream* s = malloc(sizeof(BitStream));
//     BitStream_init_empty(s, 1024);
//     BitStream_write(s, 0xA, 4); //1010
//     BitStream_write(s, 0xD, 5); //1010 0110 1
//     BitStream_write(s, 0xA, 4); //1010 0110 1101 0
//     BitStream_write(s, 0xD, 5); //1010 0110 1101 0011 01
//     BitStream_write(s, 0xA, 4); //1010 0110 1101 0011 0110 10
//     BitStream_write(s, 0xD, 5); //1010 0110 1101 0011 0110 1001 101
//     BitStream_print(s);         //A    6    D    3    6    9    A
//     BitStream* s2 = malloc(sizeof(BitStream));
//     BitStream_init_filled(s2, 1024, s->data);
//     BitStream_print(s2);
//     printf("%x\n", BitStream_read(s2, 4));
//     printf("%x\n", BitStream_read(s2, 5));
//     printf("%x\n", BitStream_read(s2, 4));
//     printf("%x\n", BitStream_read(s2, 5));
//     printf("%x\n", BitStream_read(s2, 4));
//     printf("%x\n", BitStream_read(s2, 5));
//     return 0;
// }
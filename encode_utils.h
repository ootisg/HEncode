#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

struct EncodedChar {
    char raw_symbol;
    int encoded_len;
    int encoded_symbol;
};

struct BitStream {
    uint8_t* data;
    int position;
};

typedef struct EncodedChar EncodedChar;
typedef struct BitStream BitStream;

void BitStream_init_empty(BitStream* stream, int max_size);
void BitStream_init_filled(BitStream* stream, int data_size, uint8_t* data);
void BitStream_write(BitStream* stream, uint32_t data, int num_bits);
void BitStream_write_str(BitStream* stream, char* str);
void BitStream_write_chars(BitStream* stream, char* chars, int num_chars);
uint32_t BitStream_read(BitStream* stream, int num_bits);
void BitStream_read_str(BitStream* stream, char* buf, int buf_len);
void BitStream_read_chars(BitStream* stream, char* buf, int num_chars);
void BitStream_save(BitStream* stream, char* filepath);
void BitStream_load(BitStream* stream, char* filepath);
void BitStream_print(BitStream* stream);
void EncodedChar_init(EncodedChar* ec, char symbol, int length, int encoding);
void EncodedChar_push_bit(EncodedChar* ec, int bit);
int EncodedChar_pop_bit(EncodedChar* ec);
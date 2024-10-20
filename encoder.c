#include "encoder.h"

#include "stdlib.h"
#include "stdio.h"

char* dbug_serialize_char(char c) {
    char* str = malloc(32 * sizeof(char));
    if (c <= 0x20 || c >= 0x7F) {
        sprintf(str, "[0x%02x]", (uint8_t)c);
    } else {
        sprintf(str, "%c", c);
    }
    return str;
}

char* dbug_serialize(HuffmanNode* node) {
    char* str = malloc(32 * sizeof(char));
    if (!node->left && !node->right) {
        if (node->symbol <= 0x20 || node->symbol >= 0x7F) {
            sprintf(str, "LEAF 0x%02x", (int)node->symbol);
        } else {
            sprintf(str, "LEAF %c", node->symbol);
        }
    } else {
        sprintf(str, "BRANCH %d", node->id);
    }
    return str;
}

void serialize_recursive(BitStream* stream, HuffmanNode* node, char* symbols, int* num_symbols) {
    if (!node->left && !node->right) {
        symbols[(*num_symbols)++] = node->symbol;
    }
    if (node->left) {
        BitStream_write(stream, 1, 1);
        // printf("1");
        serialize_recursive(stream, node->left, symbols, num_symbols);
    }
    if (node->right) {
        BitStream_write(stream, 1, 1);
        // printf("1");
        serialize_recursive(stream, node->right, symbols, num_symbols);
    }
    if (!(node->left && node->right)) {
        BitStream_write(stream, 0, 1);
        // printf("0");
    }
}

void serialize(BitStream* stream, HuffmanNode* head) {
    char* symbols = malloc(256 * sizeof(char));
    int num_symbols = 0;
    serialize_recursive(stream, head, symbols, &num_symbols);
    int i;
    // printf(" ");

    for (i = 0; i < num_symbols; i++) {
        BitStream_write(stream, symbols[i], 8);
        // printf("%s", dbug_serialize_char(symbols[i]));
    }
    // printf("\n");
}

void deserialize_recursive(BitStream* stream, HuffmanNode* node, unsigned char** decoded_chars, int* num_chars) {
    uint32_t next_bit = BitStream_read(stream, 1);
    if (!next_bit) {
        decoded_chars[(*num_chars)++] = &(node->symbol);
    } else {
        node->left = calloc(sizeof(HuffmanNode), 1);
        deserialize_recursive(stream, node->left, decoded_chars, num_chars);
        next_bit = BitStream_read(stream, 1);
        if (next_bit) {
            node->right = calloc(sizeof(HuffmanNode), 1);
            deserialize_recursive(stream, node->right, decoded_chars, num_chars);
        }
    }
}

void deserialize_tree(BitStream* stream, HuffmanNode* head) {
    unsigned char** decoded_chars = malloc(256 * sizeof(unsigned char*));
    int num_chars = 0;
    deserialize_recursive(stream, head, decoded_chars, &num_chars);
    int i;
    for (i = 0; i < num_chars; i++) {
        *(decoded_chars[i]) = BitStream_read(stream, 8);
    }
}

unsigned char get_symbol(BitStream* stream, HuffmanNode* head) {
    HuffmanNode* curr = head;
    while (curr->left || curr->right) {
        curr = BitStream_read(stream, 1) ? curr->right : curr->left;
    }
    return curr->symbol;
}

void populate_symbol_table_recursive(HuffmanNode* node, EncodedChar* symbol_table, EncodedChar* curr) {
    if (!node->left && !node->right) {
        curr->raw_symbol = node->symbol;
        EncodedChar_init(&(symbol_table[node->symbol]), curr->raw_symbol, curr->encoded_len, curr->encoded_symbol);
        // printf("%s: %d, %d\n", dbug_serialize_char(curr->raw_symbol), curr->encoded_symbol, curr->encoded_len);
    }
    if (node->left) {
        EncodedChar_push_bit(curr, 0);
        populate_symbol_table_recursive(node->left, symbol_table, curr);
    }
    if (node->right) {
        EncodedChar_push_bit(curr, 1);
        populate_symbol_table_recursive(node->right, symbol_table, curr);
    }
    EncodedChar_pop_bit(curr);
}

void populate_symbol_table(HuffmanNode* head, EncodedChar* symbol_table) {
    EncodedChar ec;
    EncodedChar_init(&ec, 0, 0, 0);
    populate_symbol_table_recursive(head, symbol_table, &ec);
}

void probe_size_recursive(HuffmanNode* node, int depth, int* max_depth) {
    if (node->left || node->right) {
        // Node is branch
        if (node->left) {
            probe_size_recursive(node->left, depth + 1, max_depth);
        }
        if (node->right) {
            probe_size_recursive(node->right, depth + 1, max_depth);
        }
    } else {
        if (depth > *max_depth) {
            *max_depth = depth;
        }
    }
}

int probe_size(HuffmanNode* head) {
    int max_depth = 1;
    probe_size_recursive(head, 1, &max_depth);
    return max_depth;
}

void print_tree(HuffmanNode* head) {
    if (!head->left && !head->right) {
        // Do nothing for leaf case
    } else {
        
        // if (head->left && !head->right) {
        //     printf("%s -> %s\n", dbug_serialize(head), dbug_serialize(head->left));
        // } else if (head->right && !head->left) {
        //     printf("%s -> %s\n", dbug_serialize(head), dbug_serialize(head->right));
        // } else if (head->left && head->right) {
        //     printf("%s -> %s, %s\n", dbug_serialize(head), dbug_serialize(head->left), dbug_serialize(head->right));
        // }

        if (head->left) {
            print_tree(head->left);
        }
        if (head->right) {
            print_tree(head->right);
        }
    }

}

void encode_file(char* filepath, int encode_levels) {

    int AUTO_ENCODE_MAX_DEPTH = 10;

    char* fname = filepath;
    uint32_t i = 1;
    while (filepath[i]) {
        if (filepath[i - 1] == '/' || filepath[i - 1] == '\\')
        fname = filepath + i;
        i++;
    }

    BitStream in_stream;
    BitStream_init_empty(&in_stream, 1048576);
    BitStream_load(&in_stream, filepath);

    FILE* f = fopen(filepath, "rb");
    fseek(f, 0L, SEEK_END);
    uint32_t num_symbols = ftell(f);
    fclose(f);

    if (num_symbols > 1000000) {
        printf("The file cannot be encoded because it exceeds 1MB in size.\n");
        exit(0);
    }

    BitStream* stream = malloc(sizeof(BitStream));
    BitStream* last_stream = calloc(sizeof(BitStream), 1);

    int curr_encode_level = 1;
    int final_encode_level;
    int keep_encoding = 1;
    while (keep_encoding) {

        unsigned char* buf = in_stream.data;
        
        i = 0;
        int* buckets = calloc(256, sizeof(int));
        while (i < num_symbols) {
            buckets[buf[i]]++;
            i++;
        }

        int num_nodes = 0;
        HuffmanNode* nodes = malloc(256 * sizeof(HuffmanNode) * 2);  // x2 for tree nodes
        int sum = 0;
        for (i = 0x00; i <= 0xFF; i++) {
            if (buckets[i] != 0) {
                nodes[num_nodes].symbol = i;
                nodes[num_nodes].weight = (double)buckets[i] / num_symbols;
                sum += buckets[i];
                // printf("SYMBOL: %s, %d / %d; %d\n", dbug_serialize_char(nodes[num_nodes].symbol), buckets[i], num_symbols, sum);
                num_nodes++;
            }
        }

        // Create list for sorting
        HuffmanNode** sorted = malloc(num_nodes * sizeof(HuffmanNode*));
        int num_sorted = num_nodes;
        for (i = 0; i < num_nodes; i++) {
            sorted[i] = &nodes[i];
        }

        // Sort the list
        int j = 0;
        for (i = 0; i < num_sorted; i++) {
            for (j = i; j < num_sorted; j++) {
                if (sorted[j]->weight > sorted[i]->weight) {
                    HuffmanNode* temp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = temp;
                }
            }
        }

        // // Printout for analysis
        // for (i = 0; i < num_nodes; i++) {
        //     printf("%c: %f\n", sorted[i]->symbol, sorted[i]->weight);
        // }

        int curr_id = 0;
        while (num_sorted > 1) {

            // Create a new tree node using the two least frequent available nodes
            HuffmanNode* left = sorted[num_sorted - 1];
            HuffmanNode* right = sorted[num_sorted - 2];
            HuffmanNode* tree_node = nodes + (num_nodes++);
            tree_node->left = left;
            tree_node->right = right;
            tree_node->weight = left->weight + right->weight;
            tree_node->symbol = 0;
            tree_node->id = curr_id++;

            // Replace the top two nodes with the new node and run one iteration of bubble sort
            num_sorted--;
            sorted[num_sorted - 1] = tree_node;
            for (i = num_sorted - 1; i > 0; i--) {
                HuffmanNode* a = sorted[i - 1];
                HuffmanNode* b = sorted[i];
                if (a->weight < b->weight) {
                    sorted[i - 1] = b;
                    sorted[i] = a;
                }
            }

        }

        // Last remaining node is the head
        HuffmanNode* head = sorted[0];  

        // Compute the symbol table
        EncodedChar* symbol_table = malloc(256 * sizeof(EncodedChar));
        populate_symbol_table(head, symbol_table);

        // Initialize the bitstream with the file identifier and the encoded file's name
        BitStream_init_empty(stream, 1048576);
        BitStream_write_chars(stream, "HENC1\0", 6);
        BitStream_write_str(stream, fname);

        // Append the number of symbols to the bitstream
        for (i = 0; i < 4; i++) {
            BitStream_write(stream, ((uint8_t*)(&num_symbols))[i], 8);
        }

        // Serialize the huffman tree into the bitstream
        serialize(stream, head);

        // Encode the input file symbol by symbol
        for (i = 0; i < num_symbols; i++) {
            unsigned char curr = buf[i];
            BitStream_write(stream, symbol_table[curr].encoded_symbol, symbol_table[curr].encoded_len);
        }

        // printf("ITERATION %d, NUM SYMBOLS %d, OUT SYMBOLS %d\n", curr_encode_level, num_symbols, stream->position / 8 + (stream->position % 8 ? 1 : 0));

        // Check if we should keep encoding
        if (encode_levels == 0) {
            // Auto-detect encoding level
            if (last_stream->position != 0 && last_stream->position < stream->position) {
                // This iteration expanded the data, use the last iteration instead
                stream = last_stream;  //TODO memory leak here
                keep_encoding = 0;
                final_encode_level = curr_encode_level - 1;
            } else if (curr_encode_level >= AUTO_ENCODE_MAX_DEPTH) {
                keep_encoding = 0;
                final_encode_level = curr_encode_level;
            } else {
                BitStream_init_filled(last_stream, num_symbols, stream->data);
                last_stream->position = num_symbols * 8;
                keep_encoding = 1;
            }
        } else {
            if (curr_encode_level >= encode_levels) {
                keep_encoding = 0;
                final_encode_level = curr_encode_level;
            } else {
                keep_encoding = 1;
            }
        }
        curr_encode_level++;

        // Setup for next encoding iteration
        if (keep_encoding) {
            num_symbols = stream->position / 8 + (stream->position % 8 ? 1 : 0);
            BitStream_init_filled(&in_stream, num_symbols, stream->data);
        }

        // Cleanup
        free(sorted);

    }

    // Save the bitstream to an output file
    printf("Successfully encoded the file with encoding depth %d\n", final_encode_level);
    BitStream_save(stream, "encoded.bin");

}

void decode_file(char* filepath, int decode_levels) {

    // Load the input file to a new bitstream
    BitStream* stream = malloc(sizeof(BitStream));
    BitStream_init_empty(stream, 1048576);
    BitStream_load(stream, "encoded.bin");
    stream->position = 0;

    char* save_fname;
    BitStream out_stream;
    char decoded_fname[256];
    char formatted_fname [265];

    int curr_decode_level = 1;
    int keep_decoding = 1;
    while (keep_decoding) {

        // Read the file identifier
        char fcode[6];
        BitStream_read_chars(stream, fcode, 6);
        // printf("%c%c%c%c%c\n", fcode[0], fcode[1], fcode[2], fcode[3], fcode[4]);

        // Read the filename for the decoded file
        BitStream_read_str(stream, decoded_fname, 256);
        FILE* f = fopen(decoded_fname, "rb");
        if (f) {
            sprintf(formatted_fname, "decoded_%s", decoded_fname);
            fclose(f);
            save_fname = formatted_fname;
        } else {
            fclose(f);
            save_fname = decoded_fname;
        }

        // Read the number of symbols in the encoded file
        uint32_t num_chars = 0;
        uint32_t i;
        for (i = 0; i < 4; i++) {
            ((uint8_t*)(&num_chars))[i] = BitStream_read(stream, 8);
        }
        // printf("DECODED LENGTH: %d\n", num_chars);

        // Deserialize the huffman tree from the encoded file's bitstream
        HuffmanNode* head = calloc(sizeof(HuffmanNode), 1);
        deserialize_tree(stream, head);

        // Decode the file symbol by symbol and save to the output file
        BitStream_init_empty(&out_stream, 1048576);
        for (i = 0; i < num_chars; i++) {
            BitStream_write(&out_stream, get_symbol(stream, head), 8);
        }

        // Check if we should keep decoding
        if (decode_levels != 0 && curr_decode_level >= decode_levels) {
            keep_decoding = 0;
        } else {
            if (out_stream.data[0] == 'H' && out_stream.data[1] == 'E' && out_stream.data[2] == 'N' && out_stream.data[3] == 'C' && out_stream.data[5] == 0) {
                keep_decoding = 1;
            } else {
                keep_decoding = 0;
            }
        }

        // Set up for next round of decoding
        if (keep_decoding) {
            BitStream_init_filled(stream, 1048576, out_stream.data);
        }
        curr_decode_level++;


    }

    // Close the file
    printf("Successfully decoded file\n");
    BitStream_save(&out_stream, save_fname);

}

int main(int argc, char** argv) {

    char* fname;
    int mode = 0; // 0 for auto, 1 for encode, 2 for decode, 3 for debug
    int level = 0; // 0 for auto-detect encoding/decoding level
    int i;
    for (i = 1; i < argc; i++) {
        char* curr = argv[i];
        if (curr[0] == '-') {
            if (!strcmp(curr, "-e")) {
                mode = 1;
            } else if (!strcmp(curr, "-d")) {
                mode = 2;
            } else if (!strcmp(curr, "-z")) {
                mode = 3;
            } else if (curr[1] == 'l') {
                level = atoi(curr + 2);
            }
        } else {
            fname = curr;
        }
    }

    // If set to auto, auto-detect header and change mode accordingly
    if (mode == 0) {
        FILE* f = fopen(fname, "rb");
        int c1 = fgetc(f);
        int c2 = fgetc(f);
        int c3 = fgetc(f);
        int c4 = fgetc(f);
        fgetc(f);  // Skip the version specifier
        int c6 = fgetc(f);
        fclose(f);
        if (c1 == 'H' && c2 == 'E' && c3 == 'N' && c4 == 'C' && c6 == '\0') {
            mode = 2;  // Compressed file header found, file needs decoded
        } else {
            mode = 1;  // No header found, file needs encoded
        }
    }

    // Run the encoder/decoder
    if (mode == 1) {
        encode_file(fname, level);
    } else if (mode == 2) {
        decode_file(fname, level);
    } else if (mode == 3) {
        encode_file(fname, level);
        printf("----------------------\n");
        decode_file("encoded.bin", level);
    }

}
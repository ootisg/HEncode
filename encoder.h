#include "encode_utils.h"

struct HuffmanNode {
    struct HuffmanNode* left;
    struct HuffmanNode* right;
    unsigned char symbol;
    int id;
    double weight;
};

typedef struct HuffmanNode HuffmanNode;

void print_tree(HuffmanNode* head);
char* dbug_serialize(HuffmanNode* node);

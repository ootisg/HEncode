target: encoder.c encoder.h encode_utils
	gcc encoder.c encode_utils.o -o HEncode
encode_utils: encode_utils.c encode_utils.h
	gcc -c encode_utils.c
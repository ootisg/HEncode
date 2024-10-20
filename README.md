# HEncode
CLI file compression utility using huffman encoding

Important notes: does not currently support files larger than 1MB. The utility will seg fault for inputs larger than the maximum size. Also, it will currently crash on many inputs for unknown reasons.
Build instructions: Build with GNU make using the provided makefile
Usage: HEncode filename [-d] [-e] [-l#] [-z]
Supported flags:
    \-d forces decode mode
    \-e forces encode mode
    \-l# (e.g. -l2, -l4, etc.) specifies the compression depth. If not specified, this will be auto-detected.
    \-z is a debug flag that runs both the encoder and the decoder
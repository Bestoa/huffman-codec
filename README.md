# huffman-codec

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5a9b713bcb0142e2aeae4de12aebafd6)](https://app.codacy.com/app/Bestoa/huffman-codec?utm_source=github.com&utm_medium=referral&utm_content=Bestoa/huffman-codec&utm_campaign=Badge_Grade_Dashboard)

Very simple 8 bits huffman encoder/decoder.

## Build
```make```

## Usage
Encoder a file:
```shell
./huffman -e input [output]
```

Decode a file:
```shell
./huffman -d input [output]
```

If don't specify the output, output will be the stdout.

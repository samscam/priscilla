#pragma once

const uint16_t MATRIX_FONT_NUMBERS[] PROGMEM = {
0b0111101101101111, // 0
0b0111010010110010, // 1
0b0111100111001111, // 2
0b0111001011001111, // 3
0b0001001111101101, // 4
0b0111001111100111, // 5
0b0111101111100111, // 6
0b0001001001001111, // 7
0b0111101111101111, // 8
0b0111001111101111 // 9
};

// ascii code, width, height, bits (padded to as many bytes as needed)...
// null terminated
const byte MATRIX5_FONT[] PROGMEM = {

33,1,5,0b10111000, // !
34,3,5,0b00000000,0b00001010, // "
35,5,5,0b01010111,0b11010101,0b11110101,0b0, // #
45,2,3,0b00001100, // -
46,1,1,0b10000000, // .

48,3,5,0b11110110,0b11011110, // 0
49,3,5,0b11101001,0b01100100, // 1
50,3,5,0b11110011,0b10011110, // 2
51,3,5,0b11100101,0b10011110, // 3
52,3,5,0b00100111,0b11011010, // 4
53,3,5,0b11100111,0b11001110, // 5
54,3,5,0b11110111,0b11001110, // 6
55,3,5,0b00100100,0b10011110, // 7
56,3,5,0b11110111,0b11011110, // 8
57,3,5,0b11100111,0b11011110, // 9

58,1,5,0b01010000, // :

65,3,5,0b10110111,0b11010100, // A
66,3,5,0b11010111,0b01011100, // B
67,3,5,0b01110010,0b01000110, // C
68,3,5,0b11010110,0b11011100, // D
69,3,5,0b11110011,0b01001110, // E
70,3,5,0b10010011,0b01001110, // F
71,3,5,0b01110110,0b01000110, // G
72,3,5,0b10110111,0b11011010, // H
73,3,5,0b11101001,0b00101110, // I
74,3,5,0b10001001,0b00101110, // J
75,3,5,0b10110111,0b01011010, // K
76,3,5,0b11110010,0b01001000, // L
77,5,5,0b10001100,0b01101011,0b10111000,0b10000000, // M
78,4,5,0b10011001,0b10111101,0b10010000, // N
79,4,5,0b01101001,0b10011001,0b01100000, // O
80,3,5,0b10010011,0b01011100, // P
81,4,5,0b00010110,0b10011001,0b01100000, // Q
82,3,5,0b10111010,0b11011100, // R
83,3,5,0b11000101,0b01000110, // S
84,3,5,0b01001001,0b00101110, // T
85,4,5,0b01101001,0b10011001,0b10010000, // U
86,3,5,0b01001010,0b11011010, // V
87,5,5,0b01010101,0b01100011,0b00011000,0b10000000, // W
88,3,5,0b10110101,0b01011010, // X
89,3,5,0b01001001,0b01011010, // Y
90,3,5,0b11110001,0b00011110, // Z
// Some symbols

// LOWERCASE
97,5,4,0b01101100,0b10100100,0b11000000, // a
98,3,5,0b01010111,0b01001000, // b
99,3,4,0b01110010,0b00110000, // c
100,3,5,0b01010101,0b10010010, // d
101,3,5,0b01110011,0b11010100, // e
102,2,5,0b10101110,0b01000000, // f
103,3,5,0b11000101,0b11010100, // g
104,3,5,0b10110111,0b01001000, // h
105,1,5,0b11101000, // i
106,2,5,0b10010100,0b01000000, // j
107,3,5,0b10111010,0b11001000, // k
108,1,5,0b11111000, // l
109,5,4,0b10101101,0b01101011,0b10100000, // m
110,3,4,0b10110110,0b11100000, // n
111,4,4,0b01101001,0b10010110, // o
112,3,4,0b10011010,0b10100000, // p
113,3,4,0b00101110,0b10100000, // q
114,4,4,0b10001000,0b10010110, // r
115,2,4,0b10011001, // s
116,2,5,0b01101011,0b10000000, // t
117,3,4,0b01110110,0b11010000, // u
118,3,4,0b01001010,0b11010000, // v
119,5,4,0b01010101,0b01100011,0b00010000, // w
120,3,4,0b10101001,0b01010000, // x
121,3,4,0b11000101,0b11010000, // y
122,2,4,0b01100110, // z
0x00 // null terminator
};


// french keyboard layout PS/2 codes

#define KB_F1   0xF1
#define KB_F2   0xF2
#define KB_F3   0xF3
#define KB_F4   0xF4
#define KB_F5   0xF5
#define KB_F6   0xF6
#define KB_F7   0xF7
#define KB_F8   0xF8
#define KB_F9   0xF9
#define KB_F10  0xFA
#define KB_F11  0xFB
#define KB_F12  0xFC

const unsigned char _ps2_codes_tbl[128] = {
	0x0 , //  0 (0) : 
	KB_F9, //  1 (1) : F9
	KB_F7, //  2 (2) : 
	KB_F5, //  3 (3) : F5
	KB_F3, //  4 (4) : F3
	KB_F1, //  5 (5) : F1
	KB_F2, //  6 (6) : F2
	KB_F12, //  7 (7) : F12 => STOP (F12=0x0B)
	0x0 , //  8 (8) : 
	KB_F10, //  9 (9) : F10
	KB_F8, //  A (10): F8
	KB_F6, //  B (11): F6
	KB_F4, //  C (12): F4
	'\t', //  D (13): TAB
	0x0 , //  E (14): `
	0x0 , //  F (15): 
	0x0 , // 10 (16): 
	0x0 , // 11 (17): LALT
	0x0 , // 12 (18): LSHIFT
	0x0 , // 13 (19): 
	0x0 , // 14 (20): LCTRL
	'a' ,  // 15 (21): Q (FR: A)
	'&' , // 16 (22): 1 (FR: &)
	0x0 , // 17 (23): 
	0x0 , // 18 (24): 
	0x0 , // 19 (25): 
	'w' , // 1A (26): Z (FR: W)
	's' , // 1B (27): S
	'q' , // 1C (28): A (FR: Q)
	'z' , // 1D (29): W (FR: Z)
	130 , // 1E (30): 2 (FR: é)
	0x0 , // 1F (31): 
	0x0 , // 20 (): 
	'c' , // 21 (): C
	'x' , // 22 (): X
	'd' , // 23 (): D
	'e' , // 24 (): E
	'\'' , // 25 (): 4 (FR: ')
	'\"' , // 26 (): 3 (FR: ")
	0x0 , // 27 (): 
	0x0 , // 28 (): 
	' ' , // 29 (): SPACE
	'v' , // 2A (): V
	'f' , // 2B (): F
	't' , // 2C (): T
	'r' , // 2D (): R
	'(' , // 2E (): 5 (FR: ()
	0x0 , // 2F (): 
	0x0 , // 30 () : 
	'n' , // 31 (1) : N
	'b' , // 32 (2) : B
	'h' , // 33 (3) : H
	'g' , // 34 (4) : G
	'y' , // 35 (5) : Y
	'-' , // 36 (6) : 6 (FR: -)
	0x0 , // 37 (7) : 
	0x0 , // 38 (8) : 
	0x0 , // 39 (9) : 
	',' , // 3A (10): M  (FR: ,)
	'j' , // 3B (11): J
	'u' , // 3C (12): U
	138 , // 3D (13): 7 (FR: è)
	'_' , // 3E (14): 8 (FR: _)
	0x0 , // 3F (15): 
	0x0 , // 40 (0) : 
	';' , // 41 (1) : ,  (FR: ;)
	'k' , // 42 (2) : K
	'i' , // 43 (3) : I
	'o' , // 44 (4) : O
	133 , // 45 (5) : 0 (zero)  (FR: ;)
	135 , // 46 (6) : 9 (FR: ç)
	0x0 , // 47 (7) : 
	0x0 , // 48 (8) : 
	':' , // 49 (9) : . (FR: :)
	'!' , // 4A (10): SLASH   (FR: !)
	'l' , // 4B (11): L
	'm' , // 4C (12): ; (FR: M)
	'p' , // 4D (13): P
	')' , // 4E (14): -  (FR: ))
	0x0 , // 4F (15): 
	0x0 , // 50 (0) : 
	0x0 , // 51 (1) : 
	151 , // 52 (2) : ;  (FR: ù)
	0x0 , // 53 (3) : 
	'^' , // 54 (4) : [  (FR: ^)
	'=' , // 55 (5) : =
	0x0 , // 56 (6) : 
	0x0 , // 57 (7) : 
	0x0 , // 58 (8) : CAPS
	0x0 , // 59 (9) : RSHIFT
	0x0A, // 5A (10): RETURN
	'$' , // 5B (11): ] (FR: $)
	0x0 , // 5C (12): 
	'*' , // 5D (13): ' (FR: *)
	0x0 , // 5E (14): 
	0x0 , // 5F (15): 
	0x0 , // 60 (0) : 
	'<',  // 61 (1) :    (FR: <)
	0x0 , // 62 (2) : 
	0x0 , // 63 (3) : 
	0x0 , // 64 (4) : 
	0x0 , // 65 (5) : 
	'\b', // 66 (6) : BKSP
	0x0 , // 67 (7) : 
	0x0 , // 68 (8) : 
	0x0 , // 69 (9) : KP 1
	0x0 , // 6A (10): 
	0x0 , // 6B (11): KP 4
	0x0 , // 6C (12): KP 7
	0x0 , // 6D (13): 
	0x0 , // 6E (14): 
	0x0 , // 6F (15): 
	0x0 , // 70 (0) : KP 0
	0x0 , // 71 (1) : KP .
	0x0 , // 72 (2) : KP 2
	0x0 , // 73 (3) : KP 5
	0x0 , // 74 (4) : KP 6
	0x0 , // 75 (5) : KP 8
	27  , // 76 (6) : ESC
	0x0 , // 77 (7) : NUM
	KB_F11, // 78 (8) : F11
	0x0 , // 79 (9) : KP+
	0x0 , // 7A (10): KP 3
	0x0 , // 7B (11): KP -
	0x0 , // 7C (12): KP *
	0x0 , // 7D (13): KP 9
	0x0 , // 7E (14): SCROLL
	0x0 , // 7F (15): 
};

const unsigned char _ps2_codes_shifted_tbl[128] = {
	0x0 , //  0 (0) : 
	0x0 , //  1 (1) : F9
	0x0 , //  2 (2) : 
	0x0 , //  3 (3) : F5
	0x0 , //  4 (4) : F3
	0x0 , //  5 (5) : F1
	0x0 , //  6 (6) : F2
	0x0 , //  7 (7) : F12 => STOP (F12=0x0B)
	0x0 , //  8 (8) : 
	0x0 , //  9 (9) : F10
	0x0 , //  A (10): F8
	0x0 , //  B (11): F6
	0x0 , //  C (12): F4
	0x0 , //  D (13): TAB
	0x0 , //  E (14): `
	0x0 , //  F (15): 
	0x0 , // 10 (16): 
	0x0 , // 11 (17): LALT
	0x0 , // 12 (18): LSHIFT
	0x0 , // 13 (19): 
	0x0 , // 14 (20): LCTRL
	'A' ,  // 15 (21): Q (FR: A)
	'1' , // 16 (22): 1 (FR: &)
	0x0 , // 17 (23): 
	0x0 , // 18 (24): 
	0x0 , // 19 (25): 
	'W' , // 1A (26): Z (FR: W)
	'S' , // 1B (27): S
	'Q' , // 1C (28): A (FR: Q)
	'Z' , // 1D (29): W (FR: Z)
	'2' , // 1E (30): 2 (FR: é)
	0x0 , // 1F (31): 
	0x0 , // 20 (): 
	'C' , // 21 (): C
	'X' , // 22 (): X
	'D' , // 23 (): D
	'E' , // 24 (): E
	'4' , // 25 (): 4 (FR: ')
	'3' , // 26 (): 3 (FR: ")
	0x0 , // 27 (): 
	0x0 , // 28 (): 
	' ' , // 29 (): SPACE
	'V' , // 2A (): V
	'F' , // 2B (): F
	'T' , // 2C (): T
	'R' , // 2D (): R
	'5' , // 2E (): 5 (FR: ()
	0x0 , // 2F (): 
	0x0 , // 30 () : 
	'N' , // 31 (1) : N
	'B' , // 32 (2) : B
	'H' , // 33 (3) : H
	'G' , // 34 (4) : G
	'Y' , // 35 (5) : Y
	'6' , // 36 (6) : 6 (FR: -)
	0x0 , // 37 (7) : 
	0x0 , // 38 (8) : 
	0x0 , // 39 (9) : 
	'?' , // 3A (10): M  (FR: ,)
	'J' , // 3B (11): J
	'U' , // 3C (12): U
	'7' , // 3D (13): 7 (FR: è)
	'8' , // 3E (14): 8 (FR: _)
	0x0 , // 3F (15): 
	0x0 , // 40 (0) : 
	'.' , // 41 (1) : ,  (FR: ;)
	'K' , // 42 (2) : K
	'I' , // 43 (3) : I
	'O' , // 44 (4) : O
	'0' , // 45 (5) : 0 (zero)  (FR: ;)
	'9' , // 46 (6) : 9 (FR: ç)
	0x0 , // 47 (7) : 
	0x0 , // 48 (8) : 
	'/' , // 49 (9) : . (FR: :)
	159 , // 4A (10): SLASH   (FR: !)
	'L' , // 4B (11): L
	'M' , // 4C (12): ; (FR: M)
	'P' , // 4D (13): P
	248 , // 4E (14): -  (FR: ))
	0x0 , // 4F (15): 
	0x0 , // 50 (0) : 
	0x0 , // 51 (1) : 
	'%' , // 52 (2) : ;  (FR: ù)
	0x0 , // 53 (3) : 
	'"' , // 54 (4) : [  (FR: ^)
	'+' , // 55 (5) : =
	0x0 , // 56 (6) : 
	0x0 , // 57 (7) : 
	0x0 , // 58 (8) : CAPS
	0x0 , // 59 (9) : RSHIFT
	0x0A, // 5A (10): RETURN
	156 , // 5B (11): ] (FR: $)
	0x0 , // 5C (12): 
	230 , // 5D (13): ' (FR: *)
	0x0 , // 5E (14): 
	0x0 , // 5F (15): 
	0x0 , // 60 (0) : 
	'>',  // 61 (1) :    (FR: <)
	0x0 , // 62 (2) : 
	0x0 , // 63 (3) : 
	0x0 , // 64 (4) : 
	0x0 , // 65 (5) : 
	'\b', // 66 (6) : BKSP
	0x0 , // 67 (7) : 
	0x0 , // 68 (8) : 
	0x0 , // 69 (9) : KP 1
	0x0 , // 6A (10): 
	0x0 , // 6B (11): KP 4
	0x0 , // 6C (12): KP 7
	0x0 , // 6D (13): 
	0x0 , // 6E (14): 
	0x0 , // 6F (15): 
	0x0 , // 70 (0) : KP 0
	0x0 , // 71 (1) : KP .
	0x0 , // 72 (2) : KP 2
	0x0 , // 73 (3) : KP 5
	0x0 , // 74 (4) : KP 6
	0x0 , // 75 (5) : KP 8
	27  , // 76 (6) : ESC
	0x0 , // 77 (7) : NUM
	0x0 , // 78 (8) : F11
	0x0 , // 79 (9) : KP+
	0x0 , // 7A (10): KP 3
	0x0 , // 7B (11): KP -
	0x0 , // 7C (12): KP *
	0x0 , // 7D (13): KP 9
	0x0 , // 7E (14): SCROLL
	0x0 , // 7F (15): 
};

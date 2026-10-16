#ifndef SIM_GINT_KEYBOARD_H
#define SIM_GINT_KEYBOARD_H
enum { KEY_EXIT = 1, KEY_EXE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1, KEY_F2,
       KEY_DEL, KEY_ACON,
       KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_F3, KEY_F6,
       KEY_DOT, KEY_EXP, KEY_NEG, KEY_PLUS, KEY_MINUS, KEY_MUL, KEY_DIV, KEY_LEFTP, KEY_RIGHTP,
       KEY_SQUARE, KEY_POWER, KEY_LOG, KEY_LN, KEY_SIN, KEY_COS, KEY_TAN };
typedef struct { int key; int mod, shift, alpha; } key_event_t;
key_event_t getkey(void);
#endif

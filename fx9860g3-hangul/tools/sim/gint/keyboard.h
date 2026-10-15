#ifndef SIM_GINT_KEYBOARD_H
#define SIM_GINT_KEYBOARD_H
enum { KEY_EXIT = 1, KEY_EXE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1, KEY_F2,
       KEY_DEL, KEY_ACON,
       KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9 };
typedef struct { int key; } key_event_t;
key_event_t getkey(void);
#endif

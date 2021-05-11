//
//  smartglove.h
//  max-external
//
//  Created by Manolo Müller on 09.05.21.
//
#ifndef smartglove_h
#define smartglove_h

#include "ext.h"
#include "ext_obex.h"

#define M_START 83
#define M_END   69
#define MAX_LEN 26
#define OFFSET 3    //header of each msg is 3 bytes

typedef struct _smartglove {
    t_object p_ob;
    void *outlet;
    uint8_t buffer[MAX_LEN];
    int count;
} t_smartglove;

void *smartglove_new(t_symbol *s, long argc, t_atom *argv);
void smartglove_int(t_smartglove *x, long a);
void parse_information(t_smartglove *x);
void parse_digital(t_smartglove *x);
void parse_analog(t_smartglove *x);
void clear_buffer(t_smartglove *x);
void output(t_smartglove *x, t_symbol *prepend, t_symbol *msg, long val);
void smartglove_assist(t_smartglove *x, void *b, long m, long a, char *s);
t_class *smartglove_class;

enum{
    M_INFORMATION   = 73,
    M_DIGITAL       = 68,
    M_ANALOG        = 65
}messagetype;

enum {
    D_SMARTGLOVE    = 71,
    D_SMARTBALL     = 66
}devicetype;

#define S_INFORMATION 4
t_symbol *information_sym[S_INFORMATION];
t_symbol *i_sym;
const char information_names[S_INFORMATION][100] = {
    "devicetype",
    "SmartGlove",
    "SmartBall",
    "firmware"
};

#define S_DIGITAL 16
t_symbol *digital_sym[S_DIGITAL];
t_symbol *d_sym;
const char digital_names[S_DIGITAL][100] = {
    "thumb1",
    "thumb2",
    "thumb3",
    "thumb4",
    "index1",
    "middle1",
    "ring1",
    "little1",
    "handleft",
    "handright",
    "handup",
    "handdown",
    "index2",
    "middle2",
    "ring2",
    "little2"
};

#define S_ANALOG 11
t_symbol *analog_sym[S_ANALOG];
t_symbol *a_sym;
const char analog_names[S_ANALOG][100] = {
    "distance",
    "accelx",
    "accely",
    "accelz",
    "pitch",
    "roll",
    "yaw",
    "bendindex",
    "bendmiddle",
    "bendring",
    "bendlittle"
};

#endif /* smartglove_h */

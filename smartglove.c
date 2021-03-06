/*
 * Copyright (C) 2021 by Manolo Müller
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "smartglove.h"

void ext_main(void *r){
	t_class *c;
	c = class_new("smartglove", (method)smartglove_new, NULL, sizeof(t_smartglove), 0L, A_GIMME, 0);
    class_addmethod(c, (method)smartglove_assist,	"assist",	A_CANT, 0);
    class_addmethod(c, (method)smartglove_int, "int", A_LONG, 0);
    class_addmethod(c, (method)clear_buffer, "clear", A_CANT, 0);

	CLASS_ATTR_SYM(c, "outputmode", 0, t_smartglove, outputmode);
	CLASS_ATTR_ACCESSORS(c, "outputmode", NULL, smartglove_outputmode_set);
	CLASS_ATTR_LABEL(c, "outputmode", 0, "Output mode");
	CLASS_ATTR_ENUM(c, "outputmode", 0, "sensor midi normalized");
	class_register(CLASS_BOX, c);
	smartglove_class = c;
}

void *smartglove_new(t_symbol *s, long argc, t_atom *argv){
    t_smartglove *x;
    x = (t_smartglove *)object_alloc(smartglove_class);
    x->outlet = outlet_new((t_object *)x, NULL);

	//initialize the buffer
	clear_buffer(x);

	//initialize and assign symbols
    d_sym = gensym("digital");
    a_sym = gensym("analog");
    i_sym = gensym("information");
    for(int i=0;i<S_INFORMATION;i++){
        information_sym[i] = gensym(information_names[i]);
    }
    for(int i=0;i<S_DIGITAL;i++){
        digital_sym[i] = gensym(digital_names[i]);
    }
    for(int i=0;i<S_ANALOG;i++){
        analog_sym[i] = gensym(analog_names[i]);
    }
	for(int i=0;i<S_MODES;i++){
		mode_sym[i] = gensym(mode_names[i]);
	}

	x->outputmode = mode_sym[M_NORMALIZED];
	attr_args_process(x, argc, argv);
    return(x);
}

//COMMENT THIS METHOD !!
void smartglove_int(t_smartglove *x, long a){
	//fill circular buffer
    if(x->count<MAX_LEN){
        x->buffer[x->count++] = a;
    }else{
        uint8_t temp[MAX_LEN];
        memcpy(temp, x->buffer, sizeof(uint8_t) * MAX_LEN);

        for(int i=0;i<MAX_LEN-1;i++){
            x->buffer[i] = temp[i+1];
        }
        x->buffer[MAX_LEN-1] = a;
    }

    int length = 0;
    uint8_t type = 0;
    if(x->buffer[0] == M_START){
        type = x->buffer[1];

        if(!(type==M_INFORMATION || type==M_DIGITAL || type==M_ANALOG)){
            return;
        }

        length = x->buffer[2];

        if(length>MAX_LEN){
            return;
        }else if(x->buffer[length-1] != M_END){
            return;
        }
    }

    switch(type){
        case M_INFORMATION:
            parse_information(x);
            break;
        case M_DIGITAL:
            parse_digital(x);
            break;
        case M_ANALOG:
            parse_analog(x);
            break;
        default:
            ;	//nothing happens
    }
}

void parse_information(t_smartglove *x){
    int device_type = x->buffer[OFFSET];
    t_atom devtype[3];
    atom_setsym(devtype, i_sym);
    atom_setsym(devtype+1, information_sym[0]);

    if(device_type == D_SMARTGLOVE){
        atom_setsym(devtype+2, information_sym[1]);
    }else if(device_type == D_SMARTBALL){
        atom_setsym(devtype+2, information_sym[2]);
    }
    outlet_list(x->outlet, NULL, 3, (t_atom*)&devtype);

    int fw_major    = x->buffer[OFFSET+1];
    int fw_minor    = x->buffer[OFFSET+2];
    t_atom firmware[4];
    atom_setsym(firmware, i_sym);
    atom_setsym(firmware+1, information_sym[3]);
    atom_setlong(firmware+2, fw_major);
    atom_setlong(firmware+3, fw_minor);
    outlet_list(x->outlet, NULL, 4, (t_atom*)&firmware);
}

void parse_digital(t_smartglove *x){
    for(int i=0;i<S_DIGITAL;i++){
		uint8_t ival = x->buffer[i+OFFSET];
		output(x, d_sym, digital_sym[i], ival, 1);
    }
}

void parse_analog(t_smartglove *x){
    for(int i=0;i<S_ANALOG*2;i+=2){
        uint16_t val = (x->buffer[i+3]<<8) + x->buffer[i+1+OFFSET];
        output(x, a_sym, gensym(analog_names[i/2]), val, UINT16_MAX);
    }
}

void clear_buffer(t_smartglove *x){
    memset(x->buffer, 0, sizeof(unsigned char) * MAX_LEN);
    x->count = 0;
}

void output(t_smartglove *x, t_symbol *prepend, t_symbol *msg, uint16_t ival, int max){
	//set our msgtype and sensor/button name
	t_atom a_out[3];
    atom_setsym(a_out, prepend);
    atom_setsym(a_out+1, msg);

	if(x->outputmode == mode_sym[M_NORMALIZED]){
		//dumb hack exclusively for buttons (maybe make them have their own methods?
		if(max==1){
			atom_setlong(a_out+2, ival);
		}else{
			//scale [0..max]->[0..1]
			double fval = (double)ival;
			fval /= max;
			atom_setfloat(a_out+2, fval);
		}
	}else{
		if(x->outputmode == mode_sym[M_MIDI]){
			//squeeze into 7bit-int
			ival = ival >> 9;
		}
		atom_setlong(a_out+2, ival);
	}
	outlet_list(x->outlet, NULL, 3, (t_atom*)&a_out);
}

void smartglove_assist(t_smartglove *x, void *b, long m, long a, char *s){
    if(m==1){
        sprintf(s, "Serial connection in");
    }else if(m==2){
        sprintf(s, "Messages out");
    }
}

t_max_err smartglove_outputmode_set(t_smartglove *x, void *attr, long ac, t_atom *av){
	if(ac && av){
		t_symbol *mode = atom_getsym(av);
		//check if sym is one of our defined modes
		for(int i=0;i<S_MODES;i++){
			if(mode == mode_sym[i]){
				x->outputmode = mode;
				return MAX_ERR_NONE;
			}
		}
		//if it isn't, do nothing
		//default value is set in _new, maybe here to make it more explicit?
		return MAX_ERR_GENERIC;
	}
	return MAX_ERR_NONE;
}

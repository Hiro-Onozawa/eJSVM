/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#include "prefix.h"
#define EXTERN extern
#include "header.h"

/*
 * isNAN
 */
BUILTIN_FUNCTION(builtin_isNaN)
{
  double d;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  d = to_double(context, args[1]);
  set_a(context, true_false(isnan(d)));
  GC_POP_REGBASE(args);
}

/*
 * isFinite
 */
BUILTIN_FUNCTION(builtin_isFinite)
{
  double d;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  d = to_double(context, args[1]);
  set_a(context, true_false(isinf(d)));
  GC_POP_REGBASE(args);
}

/*
 * parseInt str rad
 * converts a string to a number
 */
BUILTIN_FUNCTION(builtin_parse_int)
{
  JSValue str, rad;
  char *cstr;
  char *endPtr;
  int32_t irad;
  long ret;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  str = to_string(context, args[1]);
  GC_PUSH(str);
  rad = to_number(context, args[2]);
  GC_POP(str);
  cstr = string_to_cstr(str);

  if (!is_undefined(rad)) {
    if (is_fixnum(rad)) irad = fixnum_to_cint(rad);
    else if (is_flonum(rad)) irad = flonum_to_cint(rad);
    else irad = 10;
    if (irad < 2 || irad > 36) {
      set_a(context, gconsts.g_flonum_nan);
      GC_POP_REGBASE(args);
      return;
    }
  } else
    irad = 10;

  cstr = space_chomp(cstr);
  ret = strtol(cstr, &endPtr, irad);
  if (cstr == endPtr)
    set_a(context, gconsts.g_flonum_nan);
  else
    set_a(context, int_to_fixnum(ret));
  GC_POP_REGBASE(args);
}

#ifdef need_float
BUILTIN_FUNCTION(builtin_parse_float)
{
  JSValue str;
  char *cstr;
  double x;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  str = to_string(context, args[1]);
  cstr = string_to_cstr(str);
  cstr = space_chomp(cstr);

  x = strtod(cstr, NULL);
  if (is_fixnum_range_double(x))
    set_a(context, double_to_fixnum(x));
  else
    set_a(context, double_to_flonum(x));
  GC_POP_REGBASE(args);
}
#endif /* need_float */

/*
 * throws Error because it is not a constructor
 */
BUILTIN_FUNCTION(builtin_not_a_constructor)
{
  LOG_EXIT("Not a constructor");
}

/*
 * print
 */
BUILTIN_FUNCTION(builtin_print)
{
  int i;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  /* printf("builtin_print: na = %d, fp = %p, args = %p\n", na, fp, args); */

  for (i = 1; i <= na; ++i) {
    /* printf("args[%d] = %016lx\n", i, args[i]); */
    print_value_simple(context, args[i]);
    putchar(i < na ? ' ' : '\n');
  }
  set_a(context, JS_UNDEFINED);
  GC_POP_REGBASE(args);
}

/*
 * printv
 */
BUILTIN_FUNCTION(builtin_printv)
{
  int i;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  /* printf("builtin_print: na = %d, fp = %p, args = %p\n", na, fp, args); */

  for (i = 1; i <= na; ++i) {
    /* printf("args[%d] = %016lx\n", i, args[i]); */
    print_value_verbose(context, args[i]);
    putchar(i < na ? ' ' : '\n');
  }
  set_a(context, JS_UNDEFINED);
  GC_POP_REGBASE(args);
}

/*
 * displays the status
 */
BUILTIN_FUNCTION(builtin_printStatus)
{
  /* int fp; */
  JSValue *regBase;

  /* fp = get_fp(context); */
  regBase = (JSValue*)(&(get_stack(context, fp-1)));
  LOG_ERR("\n-----current spreg-----\ncf = %p\nfp = %d\npc = %d\nlp = %p\n",
          (FunctionCell *)regBase[-CF_POS],
          (int)regBase[-FP_POS],
          (int)regBase[-PC_POS],
          (void *)regBase[-LP_POS]);

  regBase = (JSValue*)(&(get_stack(context, regBase[-FP_POS] - 1)));
  LOG_ERR("\n-----prev spreg-----\ncf = %p\nfp = %d\npc = %d\nlp = %p\n",
          (FunctionCell *)regBase[-CF_POS],
          (int)regBase[-FP_POS],
          (int)regBase[-PC_POS],
          (void *)regBase[-LP_POS]);
}

/*
 * displays the address of an object
 */
BUILTIN_FUNCTION(builtin_address)
{
  JSValue obj;

  builtin_prologue();
  GC_PUSH_REGBASE(args);
  obj = args[1];
  printf("0x%"PRIJSValue"\n", obj);
  set_a(context, JS_UNDEFINED);
  GC_POP_REGBASE(args);
}

/*
 * prints ``hello, world''
 */
BUILTIN_FUNCTION(builtin_hello)
{
  LOG("hello, world\n");
  set_a(context, JS_UNDEFINED);
}

BUILTIN_FUNCTION(builtin_to_string)
{
  builtin_prologue();
  GC_PUSH_REGBASE(args);
  set_a(context, to_string(context, args[1]));
  GC_POP_REGBASE(args);
}

BUILTIN_FUNCTION(builtin_to_number)
{
  builtin_prologue();
  GC_PUSH_REGBASE(args);
  set_a(context, to_number(context, args[1]));
  GC_POP_REGBASE(args);
}


#ifdef USE_PAPI
/*
 * obtains the real usec
 */
BUILTIN_FUNCTION(builtin_papi_get_real)
{
  long long now = PAPI_get_real_usec();
  set_a(context, int_to_fixnum(now));
}
#endif /* USE_PAPI */

#ifdef USE_DUMMY_GPIO
#ifdef USE_DUMMY_SENSOR
/*
 * dummy sensor state
 */
int g_dummy_sensor_state = 0; // 0:sleep, 1:sending response, 2:sending data, 3:finalize
int g_dummy_sensor_signal_cnt = 0;
int g_dummy_sensor_signal[][13] = {
  { 0, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2 }
};
int g_dummy_sensor_current_sample = 0;
unsigned char g_dummy_sensor_sample[][5] = {
  { 50, 0, 24, 0,  74 },
  { 35, 0, 20, 0,  55 },
  { 55, 0, 18, 0,  73 },
  { 45, 0, 10, 0,  55 },
  { 20, 0, 13, 0,  33 },
  { 65, 0, 17, 0,  82 },
  { 80, 0, 21, 0, 101 },
  { 40, 0, 28, 0,  68 }
};
int g_dummy_sensor_current_bits = 0;
#endif

/*
 * dummy gpio functions
 */
BUILTIN_FUNCTION(builtin_read_gpio) {
  builtin_prologue();
  GC_PUSH_REGBASE(args);
  JSValue pin = args[1];
  JSValue ret = JS_UNDEFINED;

#ifdef USE_DUMMY_SENSOR
  int pin_state = 1;
  if (g_dummy_sensor_state == 0) {
    // something wrong
  }
  else if (g_dummy_sensor_state == 1) {
    g_dummy_sensor_signal_cnt++;
    pin_state = (g_dummy_sensor_signal_cnt > 8)? 1 : 0;

    if (g_dummy_sensor_signal_cnt == 16) {
      g_dummy_sensor_signal_cnt = 0;
      g_dummy_sensor_state = 2;
      if (++g_dummy_sensor_current_sample == 8) {
        g_dummy_sensor_current_sample = 0;
      }
    }
  }
  else if (g_dummy_sensor_state == 2) {
    unsigned char value = g_dummy_sensor_sample[g_dummy_sensor_current_sample][g_dummy_sensor_current_bits / 8];
    int bit = (value & (0x80 >> (g_dummy_sensor_current_bits % 8)))?1:0;
    pin_state = g_dummy_sensor_signal[bit][g_dummy_sensor_signal_cnt];
    g_dummy_sensor_signal_cnt++;
    if (g_dummy_sensor_signal[bit][g_dummy_sensor_signal_cnt] == 2) {
      g_dummy_sensor_signal_cnt = 0;
      if (++g_dummy_sensor_current_bits == 40) {
        g_dummy_sensor_current_bits = 0;
        g_dummy_sensor_state = 3;
      }
    }
  }
  else if (g_dummy_sensor_state == 3) {
    if (++g_dummy_sensor_signal_cnt < 4) {
      pin_state = 0;
    }
    else {
      g_dummy_sensor_signal_cnt = 0;
      g_dummy_sensor_state = 0;
    }
  }
  else {
    // something wrong
  }

  ret = cint_to_fixnum(pin_state);
  #endif

  set_a(context, ret);
  GC_POP_REGBASE(args);
}

BUILTIN_FUNCTION(builtin_write_gpio) {
  builtin_prologue();
  GC_PUSH_REGBASE(args);
  JSValue pin = args[1];
  JSValue state = args[2];
  JSValue ret = JS_UNDEFINED;

#ifdef USE_DUMMY_SENSOR
  if (g_dummy_sensor_state == 0) {
    if (fixnum_to_cint(state) == 1) {
      g_dummy_sensor_state = 1;
    }
  }
  else if (g_dummy_sensor_state == 1) {
    // something wrong
  }
  else if (g_dummy_sensor_state == 2) {
    // something wrong
  }
  else if (g_dummy_sensor_state == 3) {
    // something wrong
  }
  else {
    // something wrong
  }
#endif

  set_a(context, ret);
  GC_POP_REGBASE(args);
}
#endif /* USE_DUMMY_GPIO */

ObjBuiltinProp global_funcs[] = {
  { "isNaN",          builtin_isNaN,              1, ATTR_DDDE },
  { "isFinite",       builtin_isFinite,           1, ATTR_DE   },
  /*
  { "parseInt",       builtin_parseInt,           2, ATTR_DE   },
  { "parseFloat",     builtin_parseFloat,         1, ATTR_DE   },
  */
  { "print",          builtin_print,              0, ATTR_ALL  },
  { "printv",         builtin_printv,             0, ATTR_ALL  },
  { "printStatus",    builtin_printStatus,        0, ATTR_ALL  },
  { "address",        builtin_address,            0, ATTR_ALL  },
  { "hello",          builtin_hello,              0, ATTR_ALL  },
  { "to_string",      builtin_to_string,          1, ATTR_ALL  },
  { "to_number",      builtin_to_number,          1, ATTR_ALL  },
#ifdef USE_PAPI
  { "papi_get_real",  builtin_papi_get_real,      0, ATTR_ALL  },
#endif
#ifdef USE_DUMMY_GPIO
  { "read_gpio",      builtin_read_gpio,          1, ATTR_ALL  },
  { "write_gpio",     builtin_write_gpio,         2, ATTR_ALL  },
#endif /* USE_DUMMY_GPIO */
  { NULL,             NULL,                       0, ATTR_DE   }
};

ObjGconstsProp global_gconsts_props[] = {
  { "Object",    &gconsts.g_object,          ATTR_DE   },
  { "Array",     &gconsts.g_array,           ATTR_DE   },
  { "Number",    &gconsts.g_number,          ATTR_DE   },
  { "String",    &gconsts.g_string,          ATTR_DE   },
  { "Boolean",   &gconsts.g_boolean,         ATTR_DE   },
  { "NaN",       &gconsts.g_flonum_nan,      ATTR_DDDE },
  { "Infinity",  &gconsts.g_flonum_infinity, ATTR_DDDE },
  { "Math",      &gconsts.g_math,            ATTR_DE   },
  { NULL,        NULL,                       ATTR_DE   }
};

/*
 * sets the global object's properties
 */
void init_builtin_global(Context *ctx)
{
  JSValue g;

  g = gconsts.g_global;
  GC_PUSH(g);
  set_obj_cstr_prop(ctx, g, "true", JS_TRUE, ATTR_DE);
  set_obj_cstr_prop(ctx, g, "false", JS_FALSE, ATTR_DE);
  set_obj_cstr_prop(ctx, g, "null", JS_NULL, ATTR_DE);
  set_obj_cstr_prop(ctx, g, "undefined", JS_UNDEFINED, ATTR_DE);
  {
    ObjBuiltinProp *p = global_funcs;
    while (p->name != NULL) {
      set_obj_cstr_prop(ctx, g, p->name,
                        new_normal_builtin(ctx, p->fn, p->na), p->attr);
      p++;
    }
  }
  {
    ObjGconstsProp *p = global_gconsts_props;
    while (p->name != NULL) {
      set_obj_cstr_prop(ctx, g, p->name, *(p->addr), p->attr);
      p++;
    }
  }
  GC_POP(g);
}

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */

#include "ruby.h"
#define DECNUMDIGITS 34
#define DECSUBSET 0
#include "decNumber.h"
#include <stdio.h>

/* Backwards compat for old rubies */
#if !defined(RSTRING_LEN)
# define RSTRING_LEN(x) (RSTRING(x)->len)
# define RSTRING_PTR(x) (RSTRING(x)->ptr)
#endif

#if !defined(DEBUGPRINT)
# define WHERESTR  "[file %s, line %d]: "
# define WHEREARG  __FILE__, __LINE__
# define DEBUGPRINT2(...)       fprintf(stderr, __VA_ARGS__)
# define DEBUGPRINT(_fmt, ...)  DEBUGPRINT2(WHERESTR _fmt, WHEREARG, __VA_ARGS__)
#endif

VALUE cDecNumber;
VALUE cDecContext;

#define dec_num_setup( result, self, result_ptr, self_ptr, context_ptr ) \
  Data_Get_Struct( rb_iv_get(self, "@context"), decContext, context_ptr); \
  Data_Get_Struct( self, decNumber, self_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)

#define dec_num_setup_with_new_context( result, self, result_ptr, self_ptr, context, context_ptr ) \
  context = rb_funcall( cDecContext, rb_intern("new"), 0 );		\
  Data_Get_Struct( context, decContext, context_ptr);			\
  Data_Get_Struct( self, decNumber, self_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)

#define dec_num_setup_without_context( result, self, result_ptr, self_ptr ) \
  Data_Get_Struct( self, decNumber, self_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)

#define dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr ) \
  Data_Get_Struct( rb_iv_get(self, "@context"), decContext, context_ptr); \
  rval = rb_funcall( rval, rb_intern("to_dec_number"), 0 );		\
  Data_Get_Struct( self, decNumber, self_ptr);				\
  Data_Get_Struct( rval,  decNumber, rval_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)

#define dec_num_setup_rval_with_new_context( result, self, rval, result_ptr, self_ptr, rval_ptr, context, context_ptr ) \
  context = rb_funcall( cDecContext, rb_intern("new"), 0 );		\
  Data_Get_Struct( context, decContext, context_ptr);			\
  rval = rb_funcall( rval, rb_intern("to_dec_number"), 0 );		\
  Data_Get_Struct( self, decNumber, self_ptr);				\
  Data_Get_Struct( rval,  decNumber, rval_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)

/*****
      DecContext
 *****/
static VALUE con_alloc(VALUE klass) {
  decContext self_struct, *self_ptr;
  VALUE self;
  self_ptr = &self_struct;
  self = Data_Make_Struct(klass, decContext, 0, free, self_ptr);
  return self;
}

static VALUE con_set_digits(VALUE self, VALUE new_value) {
  decContext *self_ptr;
  Data_Get_Struct(self, decContext, self_ptr);
  self_ptr->digits = FIX2INT(new_value);
  return INT2FIX(self_ptr->digits);
}

static VALUE con_initialize(int argc, VALUE *argv, VALUE self) {
  decContext *self_ptr;
  VALUE digits;
  rb_scan_args( argc, argv, "01", &digits );

  Data_Get_Struct(self, decContext, self_ptr);
  decContextDefault(self_ptr, DEC_INIT_BASE);
  self_ptr->traps = 0; /* no traps TODO: error handling */

  if ( NIL_P(digits) ) {
    self_ptr->digits = DECNUMDIGITS;
  } else {
    con_set_digits(self,digits);
  }

  /* TODO: Handle arguments */

  return self;
}

static VALUE con_get_rounding(VALUE self) {
  decContext *self_ptr;
  enum rounding round;
  VALUE name_sym, rounding_names;
  Data_Get_Struct(self, decContext, self_ptr);
  round = decContextGetRounding(self_ptr);
  rounding_names = rb_const_get( cDecContext, rb_intern("ROUNDING_NAMES") );
  name_sym = rb_hash_aref( rounding_names, INT2FIX(round) );
  return name_sym;
}

static VALUE con_set_rounding(VALUE self, VALUE new_rounding) {
  decContext *self_ptr;
  enum rounding round;
  VALUE rounding_numbers;

  Data_Get_Struct(self, decContext, self_ptr);

  if ( SYMBOL_P( new_rounding ) ) {
    rounding_numbers = rb_const_get( cDecContext, rb_intern("ROUNDING_NUMBERS") );
    new_rounding = rb_hash_aref( rounding_numbers, new_rounding );
  }
  if( ! FIXNUM_P( new_rounding ) ) {
    rb_raise(rb_eTypeError, "wrong argument type");
  }

  round = FIX2INT(new_rounding);
  decContextSetRounding(self_ptr,round);
  return new_rounding;
}

static VALUE con_get_digits(VALUE self) {
  decContext *self_ptr;
  int digits;
  VALUE r_fixnum;

  Data_Get_Struct(self, decContext, self_ptr);
  digits = self_ptr->digits;
  r_fixnum = INT2FIX(digits);

  return r_fixnum;
}

static VALUE con_get_emin(VALUE self) {
  decContext *self_ptr;
  Data_Get_Struct(self, decContext, self_ptr);
  return INT2FIX(self_ptr->emin);
}

static VALUE con_set_emin(VALUE self, VALUE new_value) {
  decContext *self_ptr;
  Data_Get_Struct(self, decContext, self_ptr);
  self_ptr->emin = FIX2INT(new_value);
  return INT2FIX(self_ptr->emin);
}

static VALUE con_get_emax(VALUE self) {
  decContext *self_ptr;
  Data_Get_Struct(self, decContext, self_ptr);
  return INT2FIX(self_ptr->emax);
}

static VALUE con_set_emax(VALUE self, VALUE new_value) {
  decContext *self_ptr;
  Data_Get_Struct(self, decContext, self_ptr);
  self_ptr->emax = FIX2INT(new_value);
  return INT2FIX(self_ptr->emax);
}
/* /DecContext */


/*****
      DecNumber
 *****/

static VALUE num_alloc(VALUE klass) {
  decNumber self_struct, *self_ptr;
  decContext context;
  VALUE self;
  self_ptr = &self_struct;
  self = Data_Make_Struct(klass, decNumber, 0, free, self_ptr);
  decContextDefault(&context, DEC_INIT_BASE);

  (*self_ptr).bits = DECNAN;
  return self;
}

static VALUE num_initialize(int argc, VALUE *argv, VALUE self) {
  decNumber *self_ptr;
  decContext *context_ptr;
  VALUE from, r_str, context, digits;

  rb_scan_args( argc, argv, "02", &from, &digits );
  context = rb_funcall( cDecContext, rb_intern("new"), 1, digits );
  rb_iv_set( self, "@context", context );
  Data_Get_Struct(context, decContext, context_ptr);
  Data_Get_Struct(self, decNumber, self_ptr);

  if ( NIL_P(from) ) {
    /*    decNumberFromString(dec_num_ptr, "0", &dec_context); */
  } else {
    r_str = rb_funcall(from, rb_intern("to_s"), 0);
    decNumberFromString(self_ptr, StringValuePtr( r_str ), context_ptr);
  }
  return self;
}

/* TODO: does this work? */
static VALUE dec_number_from_struct(decNumber source_dec_num) {
  VALUE new;
  decNumber *dec_num_ptr;
  
  new = rb_funcall( cDecNumber, rb_intern("new"), 0 );
  Data_Get_Struct(new, decNumber, dec_num_ptr);
  decNumberCopy( dec_num_ptr, &source_dec_num );

  return new;
}

static VALUE dec_number_from_string(VALUE obj, VALUE from) {
  decNumber *dec_num_ptr;
  VALUE new, ret, err, r_str;
  ret = Qnil;
  new = rb_funcall( cDecNumber, rb_intern("new"), 1, from );

  Data_Get_Struct(new, decNumber, dec_num_ptr);
  if ( decNumberIsNaN( dec_num_ptr ) ) {
    /* FIXME: Just use the sprintf from C, silly */
    r_str = rb_funcall( from, rb_intern( "to_s" ), 0 );
    err = rb_funcall( rb_mKernel, rb_intern( "sprintf" ), 2, rb_str_new2("invalid value for DecNumber: %s"), r_str );
    rb_raise(rb_eArgError, StringValuePtr( err ) );
  } else {
    ret = new;
  }

  return ret;
}

/* TODO: Not sure if this should this return as a NaN when not a number, or as a 0 like nil.to_i */
static VALUE to_dec_number(VALUE obj) {
  decNumber *dec_num_ptr;
  VALUE dec_num;
  VALUE ret;
  VALUE err;

  if ( rb_obj_is_kind_of( obj, cDecNumber ) ) {
    return obj;
  }

  ret = Qnil;
  dec_num = rb_funcall( cDecNumber, rb_intern("new"), 1, obj );
  Data_Get_Struct(dec_num, decNumber, dec_num_ptr);

  if ( decNumberIsNaN( dec_num_ptr ) ) {
    decNumberZero( dec_num_ptr );
    ret = dec_num;
  } else {
    ret = dec_num;
  }

  return ret;
}

static VALUE num_coerce( VALUE self, VALUE rhs ) {
  VALUE result_arr;
  if ( rb_obj_classname(rhs) != rb_obj_classname(self) ) {
    rhs = rb_funcall( rhs, rb_intern("to_dec_number"), 0 );
  }
  result_arr = rb_ary_new2(2);
  rb_ary_store( result_arr, 0, rhs);
  rb_ary_store( result_arr, 1, self);
  return result_arr;
}

static VALUE num_to_s(VALUE self) {
  decNumber *dec_num_ptr;
  VALUE str;
  char c_str[DECNUMDIGITS+14];

  Data_Get_Struct(self, decNumber, dec_num_ptr);

  decNumberToString( dec_num_ptr, c_str );
  str = rb_str_new2(c_str);
  return str;
}

static VALUE num_to_i(VALUE self) {
  VALUE result, context;
  decContext *context_ptr;
  decNumber *self_ptr, *result_ptr;
  int32_t c_int;
  dec_num_setup_with_new_context( result, self, result_ptr, self_ptr, context, context_ptr );

  context_ptr->round = DEC_ROUND_DOWN;
  decNumberToIntegralValue( result_ptr, self_ptr, context_ptr);
  c_int = decNumberToInt32( result_ptr, context_ptr );

  return INT2NUM(c_int);
}

static VALUE num_to_f(VALUE self) {
  return rb_funcall(num_to_s(self), rb_intern("to_f"), 0);
}

static VALUE num_zero(VALUE self) {
  decNumber *dec_num_ptr;
  Data_Get_Struct(self, decNumber, dec_num_ptr);
  if ( decNumberIsZero( dec_num_ptr ) ) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

static VALUE num_nonzero(VALUE self) {
  decNumber *dec_num_ptr;
  Data_Get_Struct(self, decNumber, dec_num_ptr);
  if ( decNumberIsZero( dec_num_ptr ) ) {
    return Qfalse;
  } else {
    return Qtrue;
  }
}

static VALUE num_negative(VALUE self) {
  decNumber *dec_num_ptr;
  Data_Get_Struct(self, decNumber, dec_num_ptr);
  if ( decNumberIsNegative( dec_num_ptr ) ) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

static VALUE num_negate(VALUE self) {
  decNumber *dec_num_ptr;
  decNumber *new_dec_num_ptr;
  VALUE new;

  new = rb_funcall( cDecNumber, rb_intern("new"), 0 );
  Data_Get_Struct(self, decNumber, dec_num_ptr);
  if ( decNumberIsNaN( dec_num_ptr ) ) {
    rb_raise(rb_eTypeError, "can't negate a NaN" );
    return Qnil;
  }

  Data_Get_Struct(new, decNumber, new_dec_num_ptr);
  decNumberCopyNegate( new_dec_num_ptr, dec_num_ptr );

  if ( decNumberIsNaN( new_dec_num_ptr ) ) {
    rb_raise(rb_eTypeError, "negate failed" );
    return Qnil;
  }

  return new;
}

/* needs to check for things like nil and false, etc, not convert everything. Probably only convert numbers. */
static VALUE num_compare(VALUE self, VALUE rval) {
  VALUE ret, result, tmp_obj;
  int32_t tmp_n, klass;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;

  ret = Qnil;
  if ( SYMBOL_P( rval ) ) {
    return Qnil;
  }

  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberCompare(result_ptr, self_ptr, rval_ptr, context_ptr);

  if ( decNumberIsNaN( result_ptr ) ) {
    rb_raise(rb_eArgError, "FAIL: TODO: This error (should usually fail earlier)");
    return Qnil;
  } else {
    tmp_n = decNumberToInt32( result_ptr, context_ptr );
    ret = INT2FIX( tmp_n );
  }

  return ret;
}

static VALUE num_divide(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  if ( decNumberIsZero(rval_ptr) ) {
    (*result_ptr).bits = DECNAN;
  } else {
    decNumberDivide( result_ptr, self_ptr, rval_ptr, context_ptr);
  }
  return result;
}

static VALUE num_div(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberDivideInteger( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_multiply(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberMultiply( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_add(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberAdd( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_subtract(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberSubtract( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_abs(VALUE self) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *result_ptr;
  dec_num_setup( result, self, result_ptr, self_ptr, context_ptr );

  decNumberAbs( result_ptr, self_ptr, context_ptr);
  return result;
}

static VALUE num_ceil(VALUE self) {
  VALUE result, context;
  decContext *context_ptr;
  decNumber *self_ptr, *result_ptr;
  dec_num_setup_without_context( result, self, result_ptr, self_ptr );
  context = rb_funcall( cDecContext, rb_intern("new"), 0 );
  Data_Get_Struct( context, decContext, context_ptr); \

  (*context_ptr).round = DEC_ROUND_CEILING;
  
  decNumberToIntegralValue( result_ptr, self_ptr, context_ptr);
  return result;
}

static VALUE num_floor(VALUE self) {
  VALUE result, context;
  decContext *context_ptr;
  decNumber *self_ptr, *result_ptr;
  dec_num_setup_without_context( result, self, result_ptr, self_ptr );
  context = rb_funcall( cDecContext, rb_intern("new"), 0 );
  Data_Get_Struct( context, decContext, context_ptr); \

  (*context_ptr).round = DEC_ROUND_FLOOR;
  
  decNumberToIntegralValue( result_ptr, self_ptr, context_ptr);
  return result;
}

static VALUE num_divmod(VALUE self, VALUE rval) {
  VALUE result_int, result_rem, result_ary;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_int_ptr, *result_rem_ptr;
  dec_num_setup_rval( result_int, self, rval, result_int_ptr, self_ptr, rval_ptr, context_ptr );
  result_rem = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result_rem,  decNumber, result_rem_ptr);

  /* There might be a more efficient way to get these */
  decNumberDivideInteger( result_int_ptr, self_ptr, rval_ptr, context_ptr );
  decNumberRemainder( result_rem_ptr, self_ptr, result_int_ptr, context_ptr);
  result_ary = rb_ary_new3( 2, result_int, result_rem );
  return result_ary;
}

static VALUE num_eql(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;

  if ( rb_obj_is_kind_of( rval, cDecNumber ) && FIX2INT( num_compare( self, rval ) ) == 0 ) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

static VALUE num_modulo(VALUE self, VALUE rval) {
  VALUE result, context;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval_with_new_context( result, self, rval, result_ptr, self_ptr, rval_ptr, context, context_ptr );

  context_ptr->round = DEC_ROUND_DOWN;

  decNumberRemainder( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_round(int argc, VALUE *argv, VALUE self) {
  VALUE result, rval;
  decContext *context_ptr;
  decNumber *self_ptr, *result_ptr, *rval_ptr;
  rb_scan_args( argc, argv, "01", &rval );
  if ( NIL_P( rval ) ) {
    rval = INT2FIX( 0 );
  } else { /* probably not necessary */
    if ( ! rb_obj_is_kind_of( rval, cDecNumber ) ) {
      rval = rb_funcall( rval, rb_intern("to_i"), 0 );
    }
  }

  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberMinus( rval_ptr, rval_ptr, context_ptr);
  decNumberRescale( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

static VALUE num_power(VALUE self, VALUE rval) {
  VALUE result;
  decContext *context_ptr;
  decNumber *self_ptr, *rval_ptr, *result_ptr;
  dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr );

  decNumberPower( result_ptr, self_ptr, rval_ptr, context_ptr);
  return result;
}

void con_setup_rounding_constant( VALUE rounding_names, VALUE rounding_numbers, const char *sym_name, const char *const_name, enum rounding round ) {
  VALUE round_num, round_sym;
  round_num = INT2FIX(round);
  round_sym = ID2SYM(rb_intern(sym_name));
  rb_define_const( cDecContext, const_name, round_num );
  rb_hash_aset( rounding_names, round_num, round_sym );
  rb_hash_aset( rounding_numbers, round_sym, round_num );
  return;
}

void Init_dec_number() {
  /****
       DecContext
   ****/

  /* *** constants and enums */
  enum rounding round;
  VALUE rounding_names, rounding_numbers, round_default_num;
  cDecContext = rb_define_class("DecContext", rb_cObject);
  rounding_names = rb_hash_new();
  rb_define_const( cDecContext, "ROUNDING_NAMES", rounding_names );
  rounding_numbers = rb_hash_new();
  rb_define_const( cDecContext, "ROUNDING_NUMBERS", rounding_numbers );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "ceil",      "ROUND_CEIL",   DEC_ROUND_CEILING );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "down",      "ROUND_DOWN",      DEC_ROUND_DOWN );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "floor",     "ROUND_FLOOR",     DEC_ROUND_FLOOR );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "half_down", "ROUND_HALF_DOWN", DEC_ROUND_HALF_DOWN );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "half_even", "ROUND_HALF_EVEN", DEC_ROUND_HALF_EVEN );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "half_up",   "ROUND_HALF_UP",   DEC_ROUND_HALF_UP );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "up",        "ROUND_UP",        DEC_ROUND_UP );
  con_setup_rounding_constant( rounding_names, rounding_numbers,
			       "up05",      "ROUND_05UP",      DEC_ROUND_05UP );
  /* DEC_ROUND_DEFAULT is a macro with an extra ; so be careful */
  round = DEC_ROUND_DEFAULT;
  round_default_num = INT2FIX(round);
  rb_hash_aset( rounding_numbers, ID2SYM(rb_intern("default")), round_default_num );
  rb_define_const( cDecContext, "ROUND_DEFAULT", round_default_num );

  /* *** methods */
  rb_define_alloc_func(cDecContext, con_alloc);
  rb_define_method(cDecContext, "initialize", con_initialize, -1);
  rb_define_method(cDecContext, "rounding", con_get_rounding, 0);
  rb_define_method(cDecContext, "rounding=", con_set_rounding, 1);
  rb_define_method(cDecContext, "digits", con_get_digits, 0);
  rb_define_method(cDecContext, "digits=", con_set_digits, 1);
  rb_define_method(cDecContext, "emin", con_get_emin, 0);
  rb_define_method(cDecContext, "emin=", con_set_emin, 1);
  rb_define_method(cDecContext, "emax", con_get_emax, 0);
  rb_define_method(cDecContext, "emax=", con_set_emax, 1);

  /****
       DecNumber
   ****/
  cDecNumber = rb_define_class("DecNumber", rb_cNumeric );
  rb_define_alloc_func(cDecNumber, num_alloc);
  rb_define_attr(  cDecNumber, "context", 1, 1);
  rb_define_method(cDecNumber, "initialize", num_initialize, -1);
  rb_define_method(cDecNumber, "to_s", num_to_s, 0);
  rb_define_method(cDecNumber, "to_i", num_to_i, 0);
  rb_define_method(cDecNumber, "to_f", num_to_f, 0);
  rb_define_method(cDecNumber, "-@", num_negate, 0);
  rb_define_method(cDecNumber, "<=>", num_compare, 1);
  rb_define_method(cDecNumber, "/", num_divide, 1);
  rb_define_alias( cDecNumber, "quo", "/");
  rb_define_alias( cDecNumber, "fdiv", "quo");
  rb_define_method(cDecNumber, "coerce", num_coerce, 1);
  rb_define_method(cDecNumber, "div", num_div, 1);
  rb_define_method(cDecNumber, "*", num_multiply, 1);
  rb_define_method(cDecNumber, "+", num_add, 1);
  rb_define_method(cDecNumber, "-", num_subtract, 1);
  rb_define_method(cDecNumber, "abs", num_abs, 0);
  rb_define_method(cDecNumber, "ceil", num_ceil, 0);
  rb_define_method(cDecNumber, "floor", num_floor, 0);
  rb_define_method(cDecNumber, "divmod", num_divmod, 1);
  rb_define_method(cDecNumber, "eql?", num_eql, 1);
  rb_define_method(cDecNumber, "%", num_modulo, 1);
  rb_define_alias( cDecNumber, "modulo", "%");
  rb_define_method(cDecNumber, "zero?", num_zero, 0);
  rb_define_method(cDecNumber, "nonzero?", num_nonzero, 0);
  rb_define_method(cDecNumber, "round", num_round, -1);
  rb_define_method(cDecNumber, "**", num_power, 1);
  rb_define_method(cDecNumber, "negative?", num_negative, 0);
  rb_define_method(rb_cObject, "DecNumber", dec_number_from_string, 1);
  rb_define_method(rb_cObject, "to_dec_number", to_dec_number, 0);
}

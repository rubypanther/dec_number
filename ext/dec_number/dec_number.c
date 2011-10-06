#include "ruby.h"
#include "decNumber.h"
#include <stdio.h>

// Backwards compat for old rubies
#if !defined(RSTRING_LEN)
# define RSTRING_LEN(x) (RSTRING(x)->len)
# define RSTRING_PTR(x) (RSTRING(x)->ptr)
#endif

#if !defined(WHERESTR)
# define WHERESTR  "[file %s, line %d]: "
# define WHEREARG  __FILE__, __LINE__
# define DEBUGPRINT2(...)       fprintf(stderr, __VA_ARGS__)
# define DEBUGPRINT(_fmt, ...)  DEBUGPRINT2(WHERESTR _fmt, WHEREARG, __VA_ARGS__)
#endif

#define dec_num_setup_rval( result, self, rval, result_ptr, self_ptr, rval_ptr, context_ptr ) \
  Data_Get_Struct( rb_iv_get(self, "@context"), decContext, context_ptr);\
  rval = rb_funcall( rval, rb_intern("to_dec_number"), 0 );		\
  Data_Get_Struct( self, decNumber, self_ptr);				\
  Data_Get_Struct( rval,  decNumber, rval_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)


VALUE cDecNumber;
VALUE cDecContext;

static VALUE con_alloc(VALUE klass) {
  decContext *self_ptr;
  VALUE self;

  self = Data_Make_Struct(klass, decContext, 0, free, self_ptr);
  return self;
}

static VALUE con_initialize(int argc, VALUE *argv, VALUE self) {
  decContext *self_ptr;
  VALUE from;

  rb_scan_args( argc, argv, "01", &from );

  Data_Get_Struct(self, decContext, self_ptr);
  decContextDefault(self_ptr, DEC_INIT_BASE);
  (*self_ptr).traps = 0; // no traps TODO: error handling
  (*self_ptr).digits = 34;

  // TODO: Handle arguments

  return self;
}

static VALUE num_alloc(VALUE klass) {
  decNumber *self_ptr;
  decContext context;
  VALUE self;

  self = Data_Make_Struct(klass, decNumber, 0, free, self_ptr);
  decContextDefault(&context, DEC_INIT_BASE);

  (*self_ptr).bits = DECNAN;

  return self;
}

static VALUE num_initialize(int argc, VALUE *argv, VALUE self) {
  decNumber *self_ptr;
  decContext *context_ptr;
  VALUE from, r_str, context;

  rb_scan_args( argc, argv, "01", &from );
  context = rb_funcall( cDecContext, rb_intern("new"), 0 );
  rb_iv_set( self, "@context", context );
  Data_Get_Struct(context, decContext, context_ptr);
  Data_Get_Struct(self, decNumber, self_ptr);

  if ( NIL_P(from) ) {
    //    decNumberFromString(dec_num_ptr, "0", &dec_context);
  } else {
    r_str = rb_funcall(from, rb_intern("to_s"), 0);
    decNumberFromString(self_ptr, StringValuePtr( r_str ), context_ptr);
  }

  return self;
}

// TODO: does this work?
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
  VALUE new, ret, err;

  ret = Qnil;
  new = rb_funcall( cDecNumber, rb_intern("new"), 1, from );
  Data_Get_Struct(new, decNumber, dec_num_ptr);

  if ( decNumberIsNaN( dec_num_ptr ) ) {
    err = rb_funcall( rb_cObject, rb_intern( "sprintf" ), 2, "invalid value for DecNumber: %s", from );
    rb_raise(rb_eArgError, StringValuePtr( err ) );
  } else {
    ret = new;
  }

  return ret;
}

// TODO: Not sure if this should this return as a NaN when not a number, or as a 0 like nil.to_i
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
  if ( TYPE(rhs) != TYPE(self) ) {
    rhs = rb_funcall( rhs, rb_intern("to_dec_number"), 0 );
  }
  result_arr = rb_ary_new2(2);
  rb_ary_store( result_arr, 0, rhs);
  rb_ary_store( result_arr, 1, self);

  return result_arr;
}

static VALUE dec_number_to_string(VALUE self) {
  decNumber *dec_num_ptr;
  VALUE str;
  char c_str[DECNUMDIGITS+14];

  Data_Get_Struct(self, decNumber, dec_num_ptr);

  decNumberToString( dec_num_ptr, c_str );
  str = rb_str_new2(c_str);
  return str;
}

static VALUE num_zero_q(VALUE self) {
  decNumber *dec_num_ptr;
  Data_Get_Struct(self, decNumber, dec_num_ptr);
  if ( decNumberIsZero( dec_num_ptr ) == 1 ) {
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
    rb_raise(rb_eTypeError, "can't negate that" );
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

// needs to check for things like nil and false, etc, not convert everything. Probably only convert numbers.
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

void Init_dec_number() {
  cDecContext = rb_define_class("DecContext", rb_cObject);
  rb_define_alloc_func(cDecContext, con_alloc);
  rb_define_method(cDecContext, "initialize", con_initialize, -1);
  cDecNumber = rb_define_class("DecNumber", rb_cNumeric );
  rb_define_alloc_func(cDecNumber, num_alloc);
  rb_define_method(cDecNumber, "initialize", num_initialize, -1);
  rb_define_method(cDecNumber, "to_s", dec_number_to_string, 0);
  rb_define_method(cDecNumber, "-@", num_negate, 0);
  rb_define_method(cDecNumber, "<=>", num_compare, 1);
  rb_define_method(cDecNumber, "/", num_divide, 1);
  rb_define_method(cDecNumber, "coerce", num_coerce, 1);
  rb_define_method(cDecNumber, "div", num_div, 1);
  rb_define_method(cDecNumber, "*", num_multiply, 1);
  rb_define_method(cDecNumber, "+", num_add, 1);
  
  rb_define_method(rb_cObject, "DecNumber", dec_number_from_string, 1);
  rb_define_method(rb_cObject, "to_dec_number", to_dec_number, 0);
  //  rb_funcall(rb_mKernel,rb_intern("puts"), 1, rb_str_new2("DecNumber loaded"));
}

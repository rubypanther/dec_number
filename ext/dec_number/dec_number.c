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

#define dec_num_setup( result, lval, rval, result_ptr, lval_ptr, rval_ptr, context ) \
  decContextDefault(&(context), DEC_INIT_BASE);				\
  rval = rb_funcall( rval, rb_intern("to_dec_number"), 0 );		\
  Data_Get_Struct( lval, decNumber, lval_ptr);				\
  Data_Get_Struct( rval,  decNumber, rval_ptr);				\
  result = rb_funcall( cDecNumber, rb_intern("new"), 0 );		\
  Data_Get_Struct(result,  decNumber, result_ptr)


VALUE cDecNumber;
VALUE cDecContext;

static VALUE context_alloc(VALUE klass) {
  decContext *self_ptr;
  VALUE self;

  self = Data_Make_Struct(klass, decContext, 0, free, self_ptr);
  return self;
}

static VALUE num_alloc(VALUE klass) {
  decNumber *self_ptr;
  decContext context;
  VALUE self;

  self = Data_Make_Struct(klass, decNumber, 0, free, self_ptr);
  decContextDefault(&context, DEC_INIT_BASE);

  (*self_ptr).bits = DECNAN;

  //  num.digits = 0;
  //  num.bits = DECNAN;
  //  num.exponent = 0;
  //  num.lsu = {0};

  return self;
}

static VALUE num_initialize(int argc, VALUE *argv, VALUE self) {
  decNumber *dec_num_ptr;
  decContext dec_context;
  VALUE from, r_str;

  rb_scan_args( argc, argv, "01", &from );
  decContextDefault(&dec_context, DEC_INIT_BASE); // TODO: wrap context 
  dec_context.traps = 0; // no traps TODO: error handling
  dec_context.digits = 34;

  Data_Get_Struct(self, decNumber, dec_num_ptr);

  if ( NIL_P(from) ) {
    //    decNumberFromString(dec_num_ptr, "0", &dec_context);
  } else {
    r_str = rb_funcall(from, rb_intern("to_s"), 0);
    decNumberFromString(dec_num_ptr, StringValuePtr( r_str ), &dec_context);
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
    //err = rb_funcall( rb_mKernel, rb_intern( "sprintf" ), 2, "invalid value for DecNumber: %s", rb_funcall( obj, rb_intern( "to_s" ), 0 ) );
    //    err = rb_funcall( rb_mKernel, rb_intern( "sprintf" ), 2, rb_str_new2( "invalid value for DecNumber" ), obj );
    //    rb_raise(rb_eArgError, StringValuePtr( err ) );
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
static VALUE num_compare(VALUE self, VALUE rhs) {
  VALUE ret, tmp_obj;
  int32_t tmp_n, klass;
  decContext dec_context;
  decNumber *lhs_dec_num_ptr;
  decNumber *rhs_dec_num_ptr;
  decNumber *eql_dec_num_ptr;
  
  decContextDefault(&dec_context, DEC_INIT_BASE); // TODO: wrap context 
  ret = Qnil;

  Data_Get_Struct(self, decNumber, lhs_dec_num_ptr);

  /* This block needs to make sure rhs_dec_num_ptr is set up! */
  /*
  if ( FIXNUM_P( rhs ) ) {
    Data_Make_Struct(cDecNumber, decNumber, 0, free, rhs_dec_num_ptr);
    decNumberFromInt32(rhs_dec_num_ptr, FIX2INT( rhs ) );
  } else if ( TYPE(rhs) == T_STRING ) {
    Data_Make_Struct(cDecNumber, decNumber, 0, free, rhs_dec_num_ptr);
    decNumberFromString(rhs_dec_num_ptr, StringValuePtr( rhs ), &dec_context);
  } else if ( TYPE(rhs) == T_DATA ) { // TODO: Check for error on T_OBJECT etc
    if ( rb_obj_is_kind_of( rhs, cDecNumber ) != Qtrue ) {
      // convert
      rhs = rb_funcall(rhs, rb_intern("to_dec_number"), 0 );
    }
    Data_Get_Struct(rhs, decNumber, rhs_dec_num_ptr);
  } else if ( SYMBOL_P( rhs ) ) {
    return Qnil; // Haters hate, it's what they do.
  } else { // nil to 0
    rhs = Data_Make_Struct(cDecNumber, decNumber, 0, free, rhs_dec_num_ptr);
    decNumberZero( rhs_dec_num_ptr );
  }
  */

  if ( !rb_obj_is_kind_of( rhs, cDecNumber ) ) {
    rhs = rb_funcall(rhs, rb_intern("to_dec_number"), 0 );
  } else if ( SYMBOL_P( rhs ) ) {
    return Qnil;
  }
  Data_Get_Struct(rhs, decNumber, rhs_dec_num_ptr);
  tmp_obj = rb_funcall( cDecNumber, rb_intern("new"), 1, rb_str_new2("0") );
  Data_Get_Struct( tmp_obj, decNumber, eql_dec_num_ptr );
  decNumberCompare(eql_dec_num_ptr, lhs_dec_num_ptr, rhs_dec_num_ptr, &dec_context);
  if ( decNumberIsNaN( eql_dec_num_ptr ) ) {
    rb_raise(rb_eArgError, "FAIL: TODO: This error (should usually fail earlier)");
  } else {
    tmp_n = decNumberToInt32( eql_dec_num_ptr, &dec_context );
    ret = INT2FIX( tmp_n );
  }

  return ret;
}

/*
static VALUE num_divide(VALUE self, VALUE rhs) {
  VALUE result, arr;
  decContext dec_context;
  decNumber *lhs_dec_num_ptr, *rhs_dec_num_ptr, *result_dec_num_ptr;

  decContextDefault(&dec_context, DEC_INIT_BASE); // TODO: wrap context 

  if ( TYPE(self) == TYPE(rhs) ) {
    Data_Get_Struct(self, decNumber, lhs_dec_num_ptr);
    Data_Get_Struct(rhs,  decNumber, rhs_dec_num_ptr);
    result = rb_funcall( cDecNumber, rb_intern("new"), 0 );
    Data_Get_Struct(result,  decNumber, result_dec_num_ptr);
    decNumberDivide( result_dec_num_ptr, lhs_dec_num_ptr, rhs_dec_num_ptr, &dec_context);
  } else {
    arr = rb_funcall( rhs, rb_intern("coerce"), 1, self );
    result = num_div( rb_ary_entry(arr,0), rb_ary_entry(arr,1) );
  }

  return result;
}
*/

static VALUE num_divide(VALUE self, VALUE rval) {
  VALUE result;
  decContext dec_context;
  decNumber *self_ptr, *rval_ptr, *result_ptr;

  dec_num_setup( result, self, rval, result_ptr, self_ptr, rval_ptr, dec_context );

  if ( decNumberIsZero(rval_ptr) ) {
    (*result_ptr).bits = DECNAN;
  } else {
    decNumberDivide( result_ptr, self_ptr, rval_ptr, &dec_context);
  }

  return result;
}

static VALUE num_div(VALUE self, VALUE rval) {
  VALUE result;
  decContext dec_context;
  decNumber *self_ptr, *rval_ptr, *result_ptr;

  dec_num_setup( result, self, rval, result_ptr, self_ptr, rval_ptr, dec_context );

  decNumberDivideInteger( result_ptr, self_ptr, rval_ptr, &dec_context);
  return result;
}

void Init_dec_number() {
  cDecNumber = rb_define_class("DecNumber", rb_cNumeric );
  rb_define_alloc_func(cDecNumber, num_alloc);
  rb_define_method(cDecNumber, "initialize", num_initialize, -1);
  rb_define_method(cDecNumber, "to_s", dec_number_to_string, 0);
  rb_define_method(cDecNumber, "-@", num_negate, 0);
  rb_define_method(cDecNumber, "<=>", num_compare, 1);
  rb_define_method(cDecNumber, "/", num_divide, 1);
  rb_define_method(cDecNumber, "coerce", num_coerce, 1);
  rb_define_method(cDecNumber, "div", num_div, 1);
  
  rb_define_method(rb_cObject, "DecNumber", dec_number_from_string, 1);
  rb_define_method(rb_cObject, "to_dec_number", to_dec_number, 0);
  //  rb_funcall(rb_mKernel,rb_intern("puts"), 1, rb_str_new2("DecNumber loaded"));
}

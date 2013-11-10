#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mruby/proc.h"
#include "opcode.h"
#include "mruby/irep.h"
#include <stdio.h>

static char *optable[] = {
  "NOP", "MOVE",
  "LOADL", "LOADI", "LOADSYM", "LOADNIL",
  "LOADSELF", "LOADT", "LOADF",
  "GETGLOBAL", "SETGLOBAL", "GETSPECIAL", "SETSPECIAL",
  "GETIV", "SETIV", "GETCV", "SETCV",
  "GETCONST", "SETCONST", "GETMCNST", "SETMCNST",
  "GETUPVAR", "SETUPVAR",
  "JMP", "JMPIF", "JMPNOT",
  "ONERR", "RESCUE", "POPERR", "RAISE", "EPUSH", "EPOP",
  "SEND", "SENDB", "FSEND",
  "CALL", "SUPER", "ARGARY", "ENTER",
  "KARG", "KDICT", "RETURN", "TAILCALL", "BLKPUSH",
  "ADD", "ADDI", "SUB", "SUBI", "MUL", "DIV",
  "EQ", "LT", "LE", "GT", "GE",
  "ARRAY", "ARYCAT", "ARYPUSH", "AREF", "ASET", "APOST",
  "STRING", "STRCAT", "HASH",
  "LAMBDA", "RANGE", "OCLASS",
  "CLASS", "MODULE", "EXEC",
  "METHOD", "SCLASS", "TCLASS",
  "DEBUG", "STOP", "ERR",
};

static mrb_value
mrb_irep_make_optab(mrb_state *mrb)
{
  int i;
  int siz = sizeof(optable) / sizeof(optable[0]);
  mrb_value ary = mrb_ary_new_capa(mrb, siz);

  for (i = 0; i < siz; i++) {
    int ai = mrb_gc_arena_save(mrb);
    mrb_ary_push(mrb, ary, mrb_str_new_cstr(mrb, optable[i]));
    mrb_gc_arena_restore(mrb, ai);
  }

  return ary;
}

static void
mrb_irepobj_free(mrb_state *mrb, void *ptr)
{
  /* Do nothing */
}

static struct mrb_data_type mrb_irep_type = { "Irep", mrb_irepobj_free };

static mrb_value
mrb_irep_wrap(mrb_state *mrb, struct RClass *tc, struct mrb_irep *irep)
{
  return mrb_obj_value(Data_Wrap_Struct(mrb, tc, &mrb_irep_type, irep));
}

static mrb_value
mrb_irep_get_irep_by_no(mrb_state *mrb, mrb_value self)
{
  mrb_value no;
  mrb_get_args(mrb, "i", &no);

  return mrb_irep_wrap(mrb, mrb_class_ptr(self), mrb->irep[mrb_fixnum(no)]);
}

static mrb_value
mrb_irep_get_irep(mrb_state *mrb, mrb_value self)
{
  mrb_value recv;
  mrb_value name;
  struct RProc *m;
  struct RClass *c;

  mrb_get_args(mrb, "oo", &recv, &name);
  c = mrb_class(mrb, recv);
  m = mrb_method_search_vm(mrb, &c, mrb_symbol(name));

  if (m && !MRB_PROC_CFUNC_P(m)) {
    return mrb_irep_wrap(mrb, mrb_class_ptr(self), m->body.irep);
  }
  else {
    return mrb_nil_value();
  }
}

static mrb_value
mrb_irep_new_irep(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_value iseq;
  mrb_value pool;
  mrb_value syms;
  mrb_value nregs;
  mrb_value nlocals;
  mrb_irep *irep;

  mrb_get_args(mrb, "ooooo", &iseq, &pool, &syms, &nregs, &nlocals);
  irep = mrb_add_irep(mrb);

  irep->flags = 0;
  irep->ilen = mrb_ary_ptr(iseq)->len;
  irep->iseq = (mrb_code*)mrb_malloc(mrb, irep->ilen * sizeof(mrb_value));
  for (i = 0; i < irep->ilen; i++) {
    irep->iseq[i] = mrb_fixnum(mrb_ary_entry(iseq, i));
  }

  irep->plen = mrb_ary_ptr(pool)->len;
  irep->pool = (mrb_value *)mrb_malloc(mrb, irep->plen * sizeof(mrb_value));
  for (i = 0; i < irep->plen; i++) {
    irep->pool[i] = mrb_ary_entry(pool, i);
  }

  irep->slen = mrb_ary_ptr(syms)->len;
  irep->syms = (mrb_sym *)mrb_malloc(mrb, irep->slen * sizeof(mrb_value));
  for (i = 0; i < irep->slen; i++) {
    irep->syms[i] = mrb_symbol(mrb_ary_entry(syms, i));
  }

  irep->nregs = mrb_fixnum(nregs);
  irep->nlocals = mrb_fixnum(nlocals);
  return mrb_irep_wrap(mrb, mrb_class_ptr(self), irep);
}

static mrb_value
mrb_irep_iseq(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value ary = mrb_ary_new_capa(mrb, irep->ilen);

  
  for (i = 0; i < irep->ilen; i++) {
    mrb_ary_push(mrb, ary, mrb_fixnum_value((mrb_int) irep->iseq[i]));
  }

  return ary;
}

static mrb_value
mrb_irep_set_iseq(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value src;

  mrb_get_args(mrb, "o", &src);

  if (irep->iseq && !(irep->flags & MRB_ISEQ_NO_FREE)) {
    mrb_free(mrb, irep->iseq);
  }

  irep->flags = 0;
  irep->ilen = mrb_ary_ptr(src)->len;
  irep->iseq = (mrb_code*)mrb_malloc(mrb, irep->ilen * sizeof(mrb_value));
  for (i = 0; i < irep->ilen; i++) {
    irep->iseq[i] = mrb_fixnum(mrb_ary_entry(src, i));
  }

  return src;
}

static mrb_value
mrb_irep_pool(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value ary = mrb_ary_new_capa(mrb, irep->ilen);

  
  for (i = 0; i < irep->plen; i++) {
    mrb_ary_push(mrb, ary, irep->pool[i]);
  }

  return ary;
}

static mrb_value
mrb_irep_set_pool(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value src;

  mrb_get_args(mrb, "o", &src);

  if (irep->pool) {
    mrb_free(mrb, irep->pool);
  }

  irep->plen = mrb_ary_ptr(src)->len;
  irep->pool = (mrb_value *)mrb_malloc(mrb, irep->plen * sizeof(mrb_value));
  for (i = 0; i < irep->plen; i++) {
    irep->pool[i] = mrb_ary_entry(src, i);
  }

  return src;
}

static mrb_value
mrb_irep_syms(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value ary = mrb_ary_new_capa(mrb, irep->ilen);

  
  for (i = 0; i < irep->slen; i++) {
    mrb_ary_push(mrb, ary, mrb_symbol_value(irep->syms[i]));
  }

  return ary;
}

static mrb_value
mrb_irep_set_syms(mrb_state *mrb, mrb_value self)
{
  int i;
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value src;

  mrb_get_args(mrb, "o", &src);

  if (irep->syms) {
    mrb_free(mrb, irep->syms);
  }
    
  irep->slen = mrb_ary_ptr(src)->len;
  irep->syms = (mrb_sym *)mrb_malloc(mrb, irep->slen * sizeof(mrb_value));
  for (i = 0; i < irep->slen; i++) {
    irep->syms[i] = mrb_symbol(mrb_ary_entry(src, i));
  }

  return src;
}

static mrb_value
mrb_irep_nregs(mrb_state *mrb, mrb_value self)
{
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);

  return mrb_fixnum_value((mrb_int) irep->nregs);
}

static mrb_value
mrb_irep_set_nregs(mrb_state *mrb, mrb_value self)
{
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value src;

  mrb_get_args(mrb, "o", &src);
  irep->nregs = mrb_fixnum(src);

  return src;
}

static mrb_value
mrb_irep_nlocals(mrb_state *mrb, mrb_value self)
{
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);

  return mrb_fixnum_value((mrb_int) irep->nlocals);
}

static mrb_value
mrb_irep_set_nlocals(mrb_state *mrb, mrb_value self)
{
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);
  mrb_value src;

  mrb_get_args(mrb, "o", &src);
  irep->nlocals = mrb_fixnum(src);

  return src;
}

static mrb_value
mrb_irep_to_proc(mrb_state *mrb, mrb_value self)
{
  mrb_irep *irep = mrb_get_datatype(mrb, self, &mrb_irep_type);

  return mrb_obj_value(mrb_proc_new(mrb, irep));
}

void
mrb_mruby_meta_circular_gem_init(mrb_state *mrb)
{
  struct RClass *a;

  a = mrb_define_class(mrb, "Irep", mrb->object_class);
  MRB_SET_INSTANCE_TT(a, MRB_TT_DATA);

  mrb_define_const(mrb, a, "OPTABLE", mrb_irep_make_optab(mrb));
  mrb_define_class_method(mrb, a, "get_irep_by_no", mrb_irep_get_irep_by_no, ARGS_REQ(1));
  mrb_define_class_method(mrb, a, "get_irep", mrb_irep_get_irep, ARGS_REQ(2));
  mrb_define_class_method(mrb, a, "new_irep", mrb_irep_new_irep, ARGS_REQ(5));

  mrb_define_method(mrb, a, "iseq", mrb_irep_iseq, ARGS_NONE());
  mrb_define_method(mrb, a, "iseq=", mrb_irep_set_iseq, ARGS_REQ(1));
  mrb_define_method(mrb, a, "pool", mrb_irep_pool, ARGS_NONE());
  mrb_define_method(mrb, a, "pool=", mrb_irep_set_pool, ARGS_NONE());
  mrb_define_method(mrb, a, "syms", mrb_irep_syms, ARGS_NONE());
  mrb_define_method(mrb, a, "syms=", mrb_irep_set_syms, ARGS_NONE());
  mrb_define_method(mrb, a, "nregs", mrb_irep_nregs, ARGS_NONE());
  mrb_define_method(mrb, a, "nregs=", mrb_irep_set_nregs, ARGS_REQ(1));
  mrb_define_method(mrb, a, "nlocals", mrb_irep_nlocals, ARGS_NONE());
  mrb_define_method(mrb, a, "nlocals=", mrb_irep_set_nlocals, ARGS_REQ(1));
  mrb_define_method(mrb, a, "to_proc", mrb_irep_to_proc, ARGS_NONE());
}

void
mrb_mruby_meta_circular_gem_final(mrb_state *mrb)
{
}

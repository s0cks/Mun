#ifndef MUN_LOCATION_H
#define MUN_LOCATION_H

#include "common.h"

HEADER_BEGIN

#include "asm/core.h"

typedef struct _constant_instr constant_instr;

static const uword kInvalidLocation = 0;

typedef enum{
  kAny = 1 << 0,
  kPrefersRegister = 1 << 1,
  kRequiresRegister = 1 << 2,
  kRequiresFpuRegister = 1 << 3,
  kSameAsFirstInput = 1 << 4,
} location_policy;

typedef enum{
  kInvalid = 0,
  kConstant = 1,
  kUnallocated = 3,
  kStackSlot = 4,
  kDoubleStackSlot = 7,
  kRegister = 8,
  kFpuRegister = 12,
} location_kind;

location_kind loc_get_kind(location loc);

location_policy loc_get_policy(location loc);

uword loc_get_payload(location loc);

MUN_INLINE bool
loc_is_fpu_register(location loc){
  return loc_get_kind(loc) == kFpuRegister;
}

MUN_INLINE bool
loc_is_register(location loc){
  return loc_get_kind(loc) == kRegister;
}

MUN_INLINE bool
loc_is_unallocated(location loc){
  return loc_get_kind(loc) == kUnallocated;
}

MUN_INLINE bool
loc_is_stack_slot(location loc){
  return loc_get_kind(loc) == kStackSlot;
}

MUN_INLINE bool
loc_is_double_stack_slot(location loc){
  return loc_get_kind(loc) == kDoubleStackSlot;
}


bool loc_is_invalid(location loc);
bool loc_is_constant(location loc);

MUN_INLINE asm_register
loc_get_register(location loc){
  return ((asm_register) loc_get_payload(loc));
}

MUN_INLINE asm_fpu_register
loc_get_fpu_register(location loc){
  return ((asm_fpu_register) loc_get_payload(loc));
}

word loc_get_stack_slot(location loc);

constant_instr* loc_get_constant(location loc);

void loc_init(location* loc); // none
void loc_init_s(location* loc); // same
void loc_init_a(location* loc); // any
void loc_init_r(location* loc, asm_register reg); // register
void loc_init_x(location* loc, asm_fpu_register reg); // fpu register
void loc_init_c(location* loc, constant_instr* obj); // constant
void loc_init_z(location* loc, word slot);
void loc_init_zr(location* loc, word slot, asm_register base);
void loc_init_d(location* loc, word slot);

void loc_hint_pr(location* loc); // hint prefers register
void loc_hint_rr(location* loc); // hint requires register
void loc_hint_rx(location* loc); // hint requires fpu register

#define NO_CALL 0x0
#define CALL 0x1

typedef struct _location_summary{
  bool contains_call: 1;

  location* inputs;
  word inputs_len;

  location* temps;
  word temps_len;

  location output;
} location_summary;

location_summary* loc_summary_new(word in_count, word temp_count, bool call);
location_summary* loc_summary_make(word in_count, location out, bool call);

MUN_INLINE bool
loc_is_register_beneficial(location loc){
  location any;
  loc_init_a(&any);

  return any != loc;
}

HEADER_END

#endif
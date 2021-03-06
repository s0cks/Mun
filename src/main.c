#include <mun/bitfield.h>
#include <mun/location.h>
#include <mun/object.h>
#include <mun/codegen/ra_ssa_liveness.h>
#include <mun/ast.h>
#include <mun/graph/graph_builder.h>
#include <mun/codegen/ra_allocator.h>
#include <mun/graph/visitors/constant_propagation.h>
#include <mun/codegen/il_entries.h>
#include <mun/codegen/il_core.h>

int
main(int argc, char** argv){
  ast_node* code = sequence_node_new();
  sequence_node_append(code, return_node_new(literal_node_new(number_new(3.141592654))));

  instance* pi = function_new("pi", kMOD_NONE);
  to_function(pi)->ast = code;
  to_function(pi)->scope = local_scope_new(NULL);

  function_allocate_variables(pi);

  graph_builder builder;
  graph_builder_init(&builder, to_function(pi));

  graph* g = graph_build(&builder);
  graph_discover_blocks(g);
  graph_compute_ssa(g, 0);
  graph_select_representations(g);

  printf("'%s' Instructions:\n", to_function(pi)->name);
  int count = 0;
  forward_instr_iter(((block_entry_instr*) g->graph_entry->normal_entry), it){
    printf("\t%d: %s\n", ((count++) + 1), it->ops->name());
  }

  constant_propagator cp;
  cp_init(&cp, g);
  cp_analyze(&cp);
  cp_transform(&cp);

  graph_allocator galloc;
  graph_alloc_init(&galloc, g);
  graph_alloc_regs(&galloc);

  asm_buff func_code;
  asm_buff_init(&func_code);

  forward_instr_iter(((block_entry_instr*) g->graph_entry->normal_entry), it){
    if(!instr_is(it, kParallelMoveTag)){
      if(it->ops->emit_machine_code != NULL){
        printf("Emitting Machine Code For: %s\n", it->ops->name());
        it->ops->emit_machine_code(it, &func_code);
      }
    } else{
      parallel_move_instr* pmove = to_parallel_move(it);
      for(word i = 0; i < pmove->moves.size; i++){
        move_operands* move = pmove->moves.data[i];

        if(loc_is_register(move->src)){
          if(loc_is_register(move->dest)){

          }
        } else if(loc_is_constant(move->src)){
          asm_movq_ri(&func_code, loc_get_register(move->dest), ((asm_imm) loc_get_constant(move->src)->value));
        } else{
          if(loc_get_policy(move->src) == kRequiresRegister){

          } else{

          }
        }
      }
    }
  }

  printf("Result: %s\n", lua_to_string(((instance* (*)(void)) asm_compile(&func_code))()));
  return 0;
}
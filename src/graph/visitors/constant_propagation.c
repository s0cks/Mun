#include <mun/codegen/intermediate_language.h>
#include <mun/graph/visitors/constant_propagation.h>
#include <mun/codegen/il_entries.h>
#include <mun/buffer.h>
#include <mun/codegen/il_core.h>
#include <mun/codegen/il_defs.h>
#include <mun/object.h>

MUN_INLINE bool
set_value(constant_propagator* cp, definition* defn, instance* value){
  defn->constant_value = value;
  if(defn->input_use_list != NULL){
    def_worklist_add(&cp->worklist, defn);
  }
  return TRUE;
}

MUN_INLINE void
set_reachable(constant_propagator* cp, block_entry_instr* block){
  if(!bit_vector_contains(&cp->reachable, block->preorder_num)){
    bit_vector_add(&cp->reachable, block->preorder_num);
    buffer_add(&cp->block_worklist, block);
  }
}

#define get_def(i) \
  container_of(i, definition, instr)

MUN_INLINE bool
is_constant(instance* val){
  return val != NULL && val->type_id != kNilTag;
}

static void
cp_visit_constant(graph_visitor* vis, instruction* instr){
  set_value(get_cp(vis), get_def(instr), to_constant(instr)->value);
}

static void
cp_visit_graph_entry(graph_visitor* vis, instruction* instr){
  for(word i = 0; i < to_graph_entry(instr)->initial_definitions.size; i++){
    instr_accept(((instruction*) to_graph_entry(instr)->initial_definitions.data[i]), vis);
  }

  for(word i = 0; i < instr_successor_count(instr); i++){
    set_reachable(get_cp(vis), instr_successor_at(instr, i));
  }
}

static void
cp_visit_return(graph_visitor* vis, instruction* instr){
  // Fallthrough
}

static void
cp_visit_target_entry(graph_visitor* vis, instruction* instr){
  forward_instr_iter(instr, it){
    instr_accept(it, vis);
  }
}

static void
cp_visit_binary_op(graph_visitor* vis, instruction* instr){
  instance* left = to_binary_op(instr)->inputs[0]->defn->constant_value;
  instance* right = to_binary_op(instr)->inputs[1]->defn->constant_value;

  if(is_constant(left) && is_constant(right)){
    TYPE_GUARD(left, number);
    TYPE_GUARD(right, number);

    double left_val = to_number(left)->value;
    double right_val = to_number(right)->value;
    double result;

    switch(to_binary_op(instr)->operation){
      case 0x0: result = left_val + right_val; break;
      case 0x1: result = left_val - right_val; break;
      case 0x2: result = left_val * right_val; break;
      case 0x3: result = left_val / right_val; break;
      default:{
        fprintf(stderr, "Unreachable\n");
        abort();
      }
    }

    set_value(get_cp(vis), get_def(instr), number_new(result));
  }
}

#define VISITOR \
  cp->vis

void
cp_init(constant_propagator* cp, graph* g){
  cp->flow_graph = g;
  bit_vector_init(&cp->reachable, g->preorder.size);
  buffer_init(&cp->block_worklist, 1);
  def_worklist_init(&cp->worklist, g, 10);
  VISITOR.visit_constant = &cp_visit_constant;
  VISITOR.visit_graph_entry = &cp_visit_graph_entry;
  VISITOR.visit_target_entry = &cp_visit_target_entry;
  VISITOR.visit_return = &cp_visit_return;
  VISITOR.visit_binary_op = &cp_visit_binary_op;
}

void
cp_analyze(constant_propagator* cp){
  graph_entry_instr* entry = cp->flow_graph->graph_entry;
  buffer_add(&cp->block_worklist, ((block_entry_instr*) entry));

  while(TRUE){
    if(buffer_is_empty(&cp->block_worklist)){
      if(def_worklist_is_empty(&cp->worklist)) break;
      definition* defn = def_worklist_del_last(&cp->worklist);
      il_value* use = defn->input_use_list;
      while(use != NULL){
        instr_accept(use->instr, ((graph_visitor*) cp));
        use = use->next;
      }
    } else{
      block_entry_instr* block = buffer_del_last(&cp->block_worklist);
      instr_accept(((instruction*) block), ((graph_visitor*) cp));
    }
  }
}

void
cp_transform(constant_propagator* cp){
  graph_discover_blocks(cp->flow_graph);
}
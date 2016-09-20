#include <mun/codegen/intermediate_language.h>
#include <mun/graph/visitors/constant_propagation.h>
#include <mun/codegen/il_entries.h>
#include <mun/buffer.h>
#include <mun/codegen/il_core.h>

MUN_INLINE bool
set_value(constant_propagator* cp, definition* defn, instance* value){
  defn->constant_value = value;
  if(defn->input_use_list != NULL){
    def_worklist_add(&cp->worklist, defn);
  }
  return TRUE;
}

#define get_cp(v) \
  container_of(v, constant_propagator, vis)
#define get_def(i) \
  container_of(i, definition, instr)

static void
cp_visit_constant(graph_visitor* vis, instruction* instr){
  set_value(get_cp(vis), get_def(instr), to_constant(instr)->value);
}

static void
cp_visit_graph_entry(graph_visitor* vis, instruction* instr){
  object_buffer defs; // definition*
  buffer_clone_into(&defs, &to_graph_entry(instr)->initial_definitions);
  for(word i = 0; i < defs.size; i++){
    ((instruction*) defs.data[i])->ops->accept(defs.data[i], vis);
  }
}

static void
cp_visit_target_entry(graph_visitor* vis, instruction* instr){
  forward_instr_iter(instr, it){
    it->ops->accept(it, vis);
  }
}

#define VISITOR \
  cp->vis

void
cp_init(constant_propagator* cp, graph* g){
  cp->flow_graph = g;
  buffer_init(&cp->block_worklist, 1);
  def_worklist_init(&cp->worklist, g, 10);
  VISITOR.visit_constant = &cp_visit_constant;
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
        use->instr->ops->accept(use->instr, ((graph_visitor*) cp));
        use = use->next;
      }
    } else{
      block_entry_instr* block = buffer_del_last(&cp->block_worklist);
      ((instruction*) block)->ops->accept(((instruction*) block), ((graph_visitor*) cp));
    }
  }
}

void
cp_transform(constant_propagator* cp){
  graph_discover_blocks(cp->flow_graph);
}
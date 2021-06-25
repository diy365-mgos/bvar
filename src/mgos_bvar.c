#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "mgos_bvar.h"

#ifdef MGOS_BVAR_HAVE_JSON
#include "mgos_bvar_json.h"
#endif

#ifdef MGOS_BVAR_HAVE_DIC
#include "mgos_bvar_dic.h"
#endif

#ifdef MGOS_HAVE_MJS
#include "mjs.h"
#endif /* MGOS_HAVE_MJS */

#ifdef MG_BVAR_MEMLEAKS_CHECK
int MG_BVAR_MEMLEACKS = 0;
char *mg_bvar_strdup_hook(const char *str) {
  char *r = strdup(str); 
  if (r) ++MG_BVAR_MEMLEACKS;
  return r;
}
char *mg_bvar_strndup_hook(const char *str, size_t size) {
  char *r = strndup(str, size); 
  if (r) ++MG_BVAR_MEMLEACKS;
  return r;
}
void *mg_bvar_malloc_hook(size_t size) {
  void *r = malloc(size); 
  if (r) ++MG_BVAR_MEMLEACKS;
  return r;
}
void *mg_bvar_calloc_hook(size_t nitems, size_t size) {
  void *r = calloc(nitems, size); 
  if (r) ++MG_BVAR_MEMLEACKS;
  return r;
}
void mg_bvar_free_hook(void *p) {
  if (p) --MG_BVAR_MEMLEACKS;
  free(p); 
}
int mg_bvar_get_memleaks() {
  return MG_BVAR_MEMLEACKS;
}
bool mg_bvar_has_memleaks() {
  return (mg_bvar_get_memleaks() != 0);
}
#endif //MG_BVAR_MEMLEAKS_CHECK

#ifdef MGOS_BVAR_HAVE_DIC
struct mg_bvar_dic_key {
  const char* name;
  size_t name_size;
  mgos_bvar_t var;
  mgos_bvar_t next_var;
  mgos_bvar_t prev_var;
};

struct mg_bvar_dic_key_item_store;

struct mg_bvar_dic_key_item {
  mgos_bvar_t parent_dic;
  struct mg_bvar_dic_key key;
  struct mg_bvar_dic_key_item *next_item;
  struct mg_bvar_dic_key_item *prev_item;
  struct mg_bvar_dic_key_item_store *store;
};

#define MG_BVAR_DIC_KEY_ITEM_STORE_SIZE 6

struct mg_bvar_dic_key_item_store {
 struct mg_bvar_dic_key_item items[MG_BVAR_DIC_KEY_ITEM_STORE_SIZE];
 short int busy_count;
 struct mg_bvar_dic_key_item_store *next_store;
 struct mg_bvar_dic_key_item_store *prev_store;
};

struct mg_bvar_dic_head {
  mgos_bvar_t var;
  int count;
};
#endif

union mgos_bvar_value {
   long l;
   double d;
   bool b;
   char *s;
   #ifdef MGOS_BVAR_HAVE_DIC
   struct mg_bvar_dic_head dic_head;
   #endif
};

struct mg_bvar {
  enum mgos_bvar_type type; 
  union mgos_bvar_value value;
  size_t v_size;
  int changed;
  #ifdef MGOS_BVAR_HAVE_DIC
  struct mg_bvar_dic_key_item *key_items;
  #endif
};

#ifdef MGOS_BVAR_HAVE_JSON
struct mg_bvar_json_walk_cb_arg {
  mgos_bvar_t var;
  char ret;
};
#endif

#ifdef MGOS_BVAR_HAVE_DIC

static struct mg_bvar_dic_key_item_store *s_dic_key_item_store = NULL;

static struct mg_bvar_dic_key_item *mg_bvar_dic_key_item_pop() {
  struct mg_bvar_dic_key_item_store *store = s_dic_key_item_store;
  while(store) {
    if (store->busy_count < MG_BVAR_DIC_KEY_ITEM_STORE_SIZE) {
      for (int i=0; i < MG_BVAR_DIC_KEY_ITEM_STORE_SIZE; ++i) {
        if (!(store->items[i].key.var)) {
          store->busy_count += 1;
          return &store->items[i];
        }
      }
    }
    store = store->next_store;
  }
  
  // init the new store
  store = calloc(1, sizeof(struct mg_bvar_dic_key_item_store));
  for (int i=0; i < MG_BVAR_DIC_KEY_ITEM_STORE_SIZE; ++i) {
    store->items[i].store = store;
  }
  
  // attach new store to the chain
  if (!s_dic_key_item_store) {
    s_dic_key_item_store = store;
  } else {
    // chain the new store as first item in the list
    store->next_store = s_dic_key_item_store->next_store;
    if (store->next_store) store->next_store->prev_store = store;
    s_dic_key_item_store->next_store = store;
    store->prev_store = s_dic_key_item_store;
  }

  store->busy_count = 1;
  return &store->items[0];
}

void mg_bvar_dic_key_item_rel(struct mg_bvar_dic_key_item *item) {
  item->key.var = NULL;
  item->key.next_var = NULL;
  item->key.prev_var = NULL;
  item->next_item = NULL;
  item->prev_item = NULL;
  
  item->store->busy_count -= 1;
  
  // free unused stores
  struct mg_bvar_dic_key_item_store *store = s_dic_key_item_store;
  while(store) {
    if (store != item->store && store->busy_count == 0) {
      for (int i=0; i < MG_BVAR_DIC_KEY_ITEM_STORE_SIZE; ++i) {
        free((char *)store->items[i].key.name);
      }
      // detach store from chain
      if (store == s_dic_key_item_store) {
        // removing the first node
        s_dic_key_item_store = store->next_store;
        s_dic_key_item_store->prev_store = NULL;
      } else {
        store->prev_store->next_store = store->next_store;
        if (store->next_store) store->next_store->prev_store = store->prev_store;
      }
      free(store);
      return;
    }
    store = store->next_store;
  }
}

struct mg_bvar_dic_key_item *mg_bvar_dic_key_item_alloc(const char *key_name, size_t key_len) {
  struct mg_bvar_dic_key_item *item = (key_name ? mg_bvar_dic_key_item_pop() : NULL);
  if (item) {
    if (key_len == -1) key_len = strlen(key_name);
    if (key_len >= item->key.name_size) {
      item->key.name_size = (key_len + 1);
      char *buf = (char *)item->key.name;
      if (buf) buf = realloc(buf, item->key.name_size);
      if (!buf) buf = malloc(item->key.name_size);
      item->key.name = buf;
    }
    strncpy((char *)item->key.name, key_name, key_len);
    ((char *)item->key.name)[key_len] = '\0';
  }
  return item;
}

void mg_bvar_dic_key_item_free(struct mg_bvar_dic_key_item *item) {
  if (item) {
    memset((void *)item->key.name, 0, item->key.name_size);
    mg_bvar_dic_key_item_rel(item);
  }
}

struct mg_bvar_dic_key_item *mg_bvar_dic_get_key_item(mgos_bvarc_t var, mgos_bvarc_t parent_dic) {
  struct mg_bvar_dic_key_item *key_item = (var ? var->key_items : NULL);
  while (key_item && key_item->parent_dic != parent_dic) { key_item = key_item->next_item; }
  return key_item;
}

bool mg_bvar_dic_is_parent(mgos_bvarc_t var, mgos_bvarc_t parent_dic) {
  return (mg_bvar_dic_get_key_item(var, parent_dic) != NULL);
}

mgos_bvar_t mg_bvar_dic_get_root(mgos_bvar_t var) {
  if (var) {
    if (!var->key_items) return var; // root found
    if (var->key_items && !var->key_items->next_item) {
      return mg_bvar_dic_get_root(var->key_items->parent_dic);
    }
  }
  return NULL;
}

mgos_bvar_t mg_bvar_dic_get_parent(mgos_bvar_t var) {
  if (var && var->key_items && !var->key_items->next_item) {
    return var->key_items->parent_dic;
  }
  //the 'var' is NULL, or it has't a parent, or
  // it belongs to two or more dictionaries and so
  // it has't a single/unique parent.
  return NULL; 
}

typedef bool (* mg_bvar_dic_walk_parents_t)(mgos_bvar_t parent, mgos_bvar_t child,
                                            struct mg_bvar_dic_key_item *key_item);

int mg_bvar_dic_walk_parents(mgos_bvar_t var, mg_bvar_dic_walk_parents_t walk_func, mgos_bvar_t filter) {
  int count = 0;
  if (walk_func && var) {
    struct mg_bvar_dic_key_item *key_item = var->key_items;
    while (key_item) {
      if (!filter ||(filter == key_item->parent_dic)) {
        ++count;
        if (walk_func(key_item->parent_dic, var, key_item) == false) { break; }
      }
      key_item = key_item->next_item;
    }
  }
  return count;
}

#endif

void mg_bvar_set_changed(mgos_bvar_t var) {
  if (!var || mgos_bvar_is_changed(var)) return;
  #ifdef MGOS_BVAR_HAVE_DIC
  bool on_parent_found(mgos_bvar_t parent, mgos_bvar_t child,
                       struct mg_bvar_dic_key_item *key_item) {
    mg_bvar_set_changed(parent);
    return true;
	};
  mg_bvar_dic_walk_parents(var, on_parent_found, NULL);
  
  if (mgos_bvar_is_dic(var)) {
    ++var->changed;
    return;
  }
  #endif
  var->changed = 1;
}

void mg_bvar_remove_keys(mgos_bvar_t, bool);
void mg_bvar_close(mgos_bvar_t var, bool free_str) {
  if (var) {
    #ifdef MGOS_BVAR_HAVE_DIC
    if (mgos_bvar_is_dic(var)) {
      mg_bvar_remove_keys(var, true);
      return;
    }
    #endif    

    if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) {
      if (free_str) {
        mg_bvar_set_changed(var);
        MG_BVAR_FREE(var->value.s);
      } else {
        if (strlen(var->value.s) > 0) {
          mg_bvar_set_changed(var);
        }
        var->value.s[0] = '\0'; // set empty string
        return;
      }
    } else {
      // mark as changed (if needed)
      int i = 0;
      for (; i < sizeof(union mgos_bvar_value) && (((char *)&var->value)[i] == 0); ++i);
      if (i < sizeof(union mgos_bvar_value)) {
        mg_bvar_set_changed(var);
      }
    }

    // reset bVarinat value (to 0/NULL)
    memset(&var->value, 0, sizeof(union mgos_bvar_value));
    var->v_size = 0;
  }
}

mgos_bvar_t mg_bvar_set_type(mgos_bvar_t var, enum mgos_bvar_type t) {
  if (var && var->type != t) {
    var->type = t;
    mg_bvar_set_changed(var);
  }
  return var;
}

#ifdef MGOS_BVAR_HAVE_DIC

mgos_bvar_t mg_bvar_dic_ensure(mgos_bvar_t var, bool clear) {
  if (var) {
    if (mgos_bvar_is_dic(var)) {
      if (clear) mg_bvar_remove_keys(var, true);
    } else {
      mg_bvar_close(var, true);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_DIC);
    }
  }
  return var;
}

bool mg_bvar_dic_add(mgos_bvar_t root, const char *key_name, size_t key_len, mgos_bvar_t var) {
  if (!mg_bvar_dic_ensure(root, false) || !var || !key_name) return false;

  mgos_bvar_t last_var = root;
  mgos_bvar_t v = root->value.dic_head.var; 
  struct mg_bvar_dic_key_item *last_key_item = NULL;
  while (v) {
    last_key_item = mg_bvar_dic_get_key_item(v, root);
    last_var = v;
    v = last_key_item->key.next_var;
  };
  
  // init the new key
  struct mg_bvar_dic_key_item *new_key_item = mg_bvar_dic_key_item_alloc(key_name, key_len);
  new_key_item->parent_dic = root;
  if (!var->key_items) {
    var->key_items = new_key_item;
  } else {
    // the var was already added to one or more dictionaries
    var->key_items->prev_item = new_key_item;
    new_key_item->next_item = var->key_items;
    var->key_items = new_key_item;
  }
  
  // attach the var
  new_key_item->key.var = var;
  
  // attach the new var as first var
  new_key_item->key.prev_var = last_var;
  if (last_var == root) {
    last_var->value.dic_head.var = var;
  } else {
    last_key_item->key.next_var = var;
  }

  // increase dic length
  ++root->value.dic_head.count;
  // set dictionary as changed
  mg_bvar_set_changed(root);

  return true;
}

mgos_bvar_t mg_bvar_dic_get(mgos_bvar_t root, const char *key_name, size_t key_len, bool create) {
  if (!mg_bvar_dic_ensure(root, false) || !key_name) return NULL;

  mgos_bvar_t var = root->value.dic_head.var;
  if (key_len == -1) key_len = strlen(key_name);
  while (var) {
    // find the right key to follow in case there are two or more (the var
    // belongs to two or more dictionaries)
    struct mg_bvar_dic_key_item *key_item = mg_bvar_dic_get_key_item(var, root); 
    if (strncmp(key_item->key.name, key_name, key_len) == 0 && key_len == strlen(key_item->key.name)) return var;
    var = key_item->key.next_var;
  }
  
  if (create) {
    // not found...create a new variant
    mgos_bvar_t var = mgos_bvar_new();
    if (mg_bvar_dic_add(root, key_name, key_len, var)) return var;
    mgos_bvar_free(var);
  }
  return NULL;
}

void mg_bvar_dic_rem_key(mgos_bvar_t dic, mgos_bvar_t var) {
  bool on_parent_found(mgos_bvar_t parent, mgos_bvar_t var, struct mg_bvar_dic_key_item *key_item) {
    --parent->value.dic_head.count;  // decrease dic length
    mg_bvar_set_changed(parent); // set dic as changed

    struct mg_bvar_dic_key_item *prev_item = mg_bvar_dic_get_key_item(key_item->key.prev_var, parent);
    struct mg_bvar_dic_key_item *next_item = mg_bvar_dic_get_key_item(key_item->key.next_var, parent);

    // remove the the key form the key list of the var
    if (key_item->prev_item) {
      key_item->prev_item->next_item = key_item->next_item;
      if (key_item->next_item) { key_item->next_item->prev_item = key_item->prev_item; }
    } else {
      var->key_items = key_item->next_item;
      if (var->key_items) { var->key_items->prev_item = NULL; }
    }
    
    if (prev_item && next_item) {
      prev_item->key.next_var = (next_item ? next_item->key.var : NULL);
      next_item->key.prev_var = (prev_item ? prev_item->key.var : NULL);
    } else if (!prev_item && !next_item) {
      // removing the only one key from dictionary
      parent->value.dic_head.var = NULL;
    } else if (!prev_item || !next_item) {
      if (!prev_item) {
        // removing the first key from dictionary
        parent->value.dic_head.var = next_item->key.var;
        next_item->key.prev_var = parent;
      } else {
        // removing the last key from dictionary
        prev_item->key.next_var = NULL;
      }
    }
    
    mg_bvar_dic_key_item_free(key_item);
    return false;
  };

  mg_bvar_dic_walk_parents(var, on_parent_found, dic);
}

void mg_bvar_remove_keys(mgos_bvar_t var, bool dispose) {
  if (mgos_bvar_is_dic(var)) {
    while (var->value.dic_head.var) { 
      mgos_bvar_t v = var->value.dic_head.var;
      mg_bvar_dic_rem_key(var, v);
      if (dispose) mgos_bvar_free(v);
    }
  }
}

/* mg_bvar_dic_cmp - compares 2 dictinaries 
 * Returns:
     - MGOS_BVAR_CMP_RES_EQUAL: the two dictionaries are equal
     - MGOS_BVAR_CMP_RES_NOT_EQUAL: the two dictionaries are not equal
     - (MGOS_BVAR_CMP_RES_MINOR | MGOS_BVAR_CMP_RES_EQUAL): var1 is contained, as exact copy, into var2 (var1 < var2)
     - (MGOS_BVAR_CMP_RES_MAJOR | MGOS_BVAR_CMP_RES_EQUAL): var1 contains an exact copy of var2 (var1 > var2)
*/
enum mgos_bvar_cmp_res mg_bvar_dic_cmp(mgos_bvarc_t var1, mgos_bvarc_t var2) {
  if (!mgos_bvar_is_dic(var1) || !mgos_bvar_is_dic(var2)) return MGOS_BVAR_CMP_RES_NOT_EQUAL;

  int var1_len = mgos_bvar_length(var1);
  int var2_len = mgos_bvar_length(var2);

  mgos_bvarc_t dsmall = (var1_len > var2_len ? var2 : var1);
  mgos_bvarc_t dbig = (dsmall ==  var1 ? var2 : var1);
 
  mgos_bvar_t var = dsmall->value.dic_head.var;
  while(var) {
    struct mg_bvar_dic_key_item *my_key_items = mg_bvar_dic_get_key_item(var, dsmall);
    mgos_bvar_t value = mg_bvar_dic_get((mgos_bvar_t)dbig, my_key_items->key.name, strlen(my_key_items->key.name), false);
    if (!value)
      return MGOS_BVAR_CMP_RES_NOT_EQUAL; // key not found, the twp dictionaries are not equal
    if (mgos_bvar_cmp(value, var) != MGOS_BVAR_CMP_RES_EQUAL)
      return MGOS_BVAR_CMP_RES_NOT_EQUAL; // the two key_items are not equal, so the two dictionaries are not equal
    var = my_key_items->key.next_var;
  }

  // all key_items of the smaller dictionary are euqal to the ones in the
  // bigger dictionary

  if (var1_len == var2_len)
    return MGOS_BVAR_CMP_RES_EQUAL; // all key_items are euqal, so the two dictionaries are equal
  else if (var1_len < var2_len)
    return (MGOS_BVAR_CMP_RES_MINOR | MGOS_BVAR_CMP_RES_EQUAL); // an exact copy of var1 is contained into the bigger var2 (var1 < var2)
  else
    return (MGOS_BVAR_CMP_RES_MAJOR | MGOS_BVAR_CMP_RES_EQUAL); // an exact copy of var2 is contained into the bigger var1 (var1 > var2)
}

bool mg_bvar_copy(mgos_bvarc_t, mgos_bvar_t, bool);
bool mg_bvar_dic_copy(mgos_bvarc_t src, mgos_bvar_t dest, bool del_unmatch) {
  if (!mgos_bvar_is_dic(src)) return false;
  if (!mg_bvar_dic_ensure(dest, false)) return false;

  // Sync existing key_items and add new key_items
  struct mg_bvar_dic_key_item *key_item;
  mgos_bvar_t value;
  mgos_bvar_t var = src->value.dic_head.var;
  while(var) {
    key_item = mg_bvar_dic_get_key_item(var, src);
    value = mg_bvar_dic_get(dest, key_item->key.name, -1, false);
    if (value) {
      mg_bvar_copy(var, value, del_unmatch);
    } else {
      mg_bvar_dic_add(dest, key_item->key.name, -1, var); 
    }
    var = key_item->key.next_var;
  }
  
  if (del_unmatch) {
    // Remove not existing key_items 
    var = dest->value.dic_head.var;
    while(var) {
      key_item = mg_bvar_dic_get_key_item(var, dest);
      if (!mg_bvar_dic_get((mgos_bvar_t)src, key_item->key.name, strlen(key_item->key.name), false)) {
        var = key_item->key.next_var; 
        value = key_item->key.var;
        mg_bvar_dic_rem_key(dest, value);
        mgos_bvar_free(value);
      } else {
        var = key_item->key.next_var;
      }
    }
  }
  
  return true;
}

#endif //MGOS_BVAR_HAVE_DIC

#ifdef MGOS_BVAR_HAVE_JSON

void mg_bvar_json_walk_cb(void *callback_data,
                          const char *name, size_t name_len,
                          const char *path,
                          const struct json_token *token) {
  struct mg_bvar_json_walk_cb_arg *arg = (struct mg_bvar_json_walk_cb_arg *)callback_data;
  if (!arg || (arg->ret != 0)) return;
  
  mgos_bvar_t new_item = NULL;
  if (token->type == JSON_TYPE_STRING || token->type == JSON_TYPE_NUMBER ||
      token->type == JSON_TYPE_FALSE  || token->type == JSON_TYPE_TRUE ||
      token->type == JSON_TYPE_NULL   || token->type == JSON_TYPE_OBJECT_START) {
    #ifdef MGOS_BVAR_HAVE_DIC
    new_item = (token->type == JSON_TYPE_OBJECT_START ? mgos_bvar_new_dic() : mgos_bvar_new()); 
    if (arg->var && name) {
      mg_bvar_dic_add(arg->var, name, name_len, new_item);
    }
    if (token->type == JSON_TYPE_OBJECT_START || !arg->var) arg->var = new_item;
    #else
    arg->var = new_item = mgos_bvar_new();
    #endif
  }
      
  switch(token->type) {
    case JSON_TYPE_NULL:
      break;
    case JSON_TYPE_NUMBER:
      if (memchr(token->ptr, '.', token->len) != NULL) {
        double value;
        json_scanf(token->ptr, token->len, "%lf", &value);
        mgos_bvar_set_decimal(new_item, value);
      } else {
        long value;
        json_scanf(token->ptr, token->len, "%ld", &value);
        mgos_bvar_set_integer(new_item, value);
      }
      break;
    case JSON_TYPE_TRUE:
      mgos_bvar_set_bool(new_item, true);
      break;
    case JSON_TYPE_FALSE:
      mgos_bvar_set_bool(new_item, false);
      break;
    case JSON_TYPE_STRING:
      mgos_bvar_set_nstr(new_item, token->ptr, token->len);
      break;
    #ifdef MGOS_BVAR_HAVE_DIC
    case JSON_TYPE_OBJECT_START:
      // nothing to do
      break;
    case JSON_TYPE_OBJECT_END: {
        mgos_bvar_t parent = mg_bvar_dic_get_parent(arg->var);
        if (parent) arg->var = parent;
        break;
      }
    #else
    case JSON_TYPE_OBJECT_START:
      arg->ret = 1; // stop walking
      break;
    #endif
    default:
      arg->ret = -1; // stop walking + error
      break;
  };
}

#endif // MGOS_BVAR_HAVE_JSON

bool mgos_bvar_is_null(mgos_bvarc_t var) {
  return (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_NULL);
}

mgos_bvar_t mgos_bvar_new() {
  return (mgos_bvar_t)MG_BVAR_CALLOC(1, sizeof(struct mg_bvar));
}

mgos_bvar_t mgos_bvar_new_integer(long value) {
  mgos_bvar_t var = mgos_bvar_new();
  mgos_bvar_set_integer(var, value);
  mgos_bvar_set_unchanged(var);
  return var;
}

mgos_bvar_t mgos_bvar_new_bool(bool value) {
  mgos_bvar_t var = mgos_bvar_new();
  mgos_bvar_set_bool(var, value);
  mgos_bvar_set_unchanged(var);
  return var;
}

mgos_bvar_t mgos_bvar_new_decimal(double value) {
  mgos_bvar_t var = mgos_bvar_new();
  mgos_bvar_set_decimal(var, value);
  mgos_bvar_set_unchanged(var);
  return var;
}

mgos_bvar_t mgos_bvar_new_str(const char *value) {
  mgos_bvar_t var = mgos_bvar_new();
  mgos_bvar_set_str(var, value);
  mgos_bvar_set_unchanged(var);
  return var;
}

mgos_bvar_t mgos_bvar_new_nstr(const char *value, int value_len) {
  mgos_bvar_t var = mgos_bvar_new();
  mgos_bvar_set_nstr(var, value, value_len);
  mgos_bvar_set_unchanged(var);
  return var;
}

enum mgos_bvar_type mgos_bvar_get_type(mgos_bvarc_t var) {
  return (var ? var->type : MGOS_BVAR_TYPE_NULL);
}

void mgos_bvar_set_null(mgos_bvar_t var) {
  if (var && (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_NULL)) {
    mg_bvar_close(var, true);
    mg_bvar_set_type(var, MGOS_BVAR_TYPE_NULL);
  }
}

void mgos_bvar_clear(mgos_bvar_t var) {
  if (var && (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_NULL)) {
    mg_bvar_close(var, false);
  }
}

enum mgos_bvar_cmp_res mgos_bvar_cmp(mgos_bvarc_t var1, mgos_bvarc_t var2) {
  if (var1 == var2) return MGOS_BVAR_CMP_RES_EQUAL; // comparing the same instance (or both NULL)
  if ((var1 == NULL && var2 != NULL) || (var1 != NULL && var2 == NULL)) return MGOS_BVAR_CMP_RES_NOT_EQUAL;
    
  #ifdef MGOS_BVAR_HAVE_DIC
  if(mgos_bvar_is_dic(var1) && mgos_bvar_is_dic(var2)) {
    return mg_bvar_dic_cmp(var1, var2);
  }
  #endif

  enum mgos_bvar_type t1 = mgos_bvar_get_type(var1);
  enum mgos_bvar_type t2 = mgos_bvar_get_type(var2);
  switch(t1) {
    case MGOS_BVAR_TYPE_INTEGER:
      if (t2 == MGOS_BVAR_TYPE_INTEGER)
        return (var1->value.l < var2->value.l ? MGOS_BVAR_CMP_RES_MINOR :
          (var1->value.l > var2->value.l ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      else if (t2 == MGOS_BVAR_TYPE_DECIMAL)
        return (var1->value.l < var2->value.d ? MGOS_BVAR_CMP_RES_MINOR :
          (var1->value.l > var2->value.d ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      break;
    case MGOS_BVAR_TYPE_DECIMAL:
      if (t2 == MGOS_BVAR_TYPE_DECIMAL)
        return (var1->value.d < var2->value.d ? MGOS_BVAR_CMP_RES_MINOR :
          (var1->value.d > var2->value.d ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      else if (t2 == MGOS_BVAR_TYPE_INTEGER)
        return (var1->value.d < var2->value.l ? MGOS_BVAR_CMP_RES_MINOR :
          (var1->value.d > var2->value.l ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      break;
    case MGOS_BVAR_TYPE_BOOL:
      if (t2 == MGOS_BVAR_TYPE_BOOL)
        return (var1->value.b < var2->value.b ? MGOS_BVAR_CMP_RES_MINOR :
          (var1->value.b > var2->value.b ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      break;
    case MGOS_BVAR_TYPE_STR:
      if (t2 == MGOS_BVAR_TYPE_STR) {
        int cmp = strcmp(var1->value.s, var2->value.s);
        return (cmp < 0 ? MGOS_BVAR_CMP_RES_MINOR : (cmp > 0 ? MGOS_BVAR_CMP_RES_MAJOR : MGOS_BVAR_CMP_RES_EQUAL));
      }
      break;
    case MGOS_BVAR_TYPE_NULL:
      if (t2 == MGOS_BVAR_TYPE_NULL)
        return MGOS_BVAR_CMP_RES_EQUAL;
      break;
  };

  return MGOS_BVAR_CMP_RES_NOT_EQUAL;
}

bool mg_bvar_copy(mgos_bvarc_t src_var, mgos_bvar_t dest_var, bool del_unmatch) {
  if (!src_var || !dest_var) return false;
  if (src_var == dest_var) return true; // coping the same instance
  
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(src_var)) {
    return mg_bvar_dic_copy(src_var, dest_var, del_unmatch);
  }
  #endif

  switch(mgos_bvar_get_type(src_var)) {
    case MGOS_BVAR_TYPE_INTEGER:
      mgos_bvar_set_integer(dest_var, src_var->value.l);
      break;
    case MGOS_BVAR_TYPE_DECIMAL:
      mgos_bvar_set_decimal(dest_var, src_var->value.d);
      break;
    case MGOS_BVAR_TYPE_BOOL:
      mgos_bvar_set_bool(dest_var, src_var->value.b);
      break;
    case MGOS_BVAR_TYPE_STR:
      mgos_bvar_set_str(dest_var, (const char *)src_var->value.s);
      break;
    case MGOS_BVAR_TYPE_NULL:
      mgos_bvar_set_null(dest_var);
      break;
    default:
      return false;
  };

  return true;
}

bool mgos_bvar_copy(mgos_bvarc_t src_var, mgos_bvar_t dest_var) {
  return mg_bvar_copy(src_var, dest_var, true);
}

bool mgos_bvar_merge(mgos_bvarc_t src_var, mgos_bvar_t dest_var) {
  if (src_var == dest_var) return true; // merging the same instance
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(src_var)) {
    return  mg_bvar_dic_copy(src_var, dest_var, false);
  }
  #endif
  return mgos_bvar_copy(src_var, dest_var);
}

void mgos_bvar_set_integer(mgos_bvar_t var, long value) {
  if (var) {
    if (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_INTEGER) {
      mg_bvar_close(var, true);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_INTEGER);
      var->v_size = sizeof(long);
    } else if (value != var->value.l) {
      mg_bvar_set_changed(var);
    }
    var->value.l = value;
  }
}

void mgos_bvar_set_bool(mgos_bvar_t var, bool value) {
  if (var) {
    if (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_BOOL) {
      mg_bvar_close(var, true);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_BOOL);
      var->v_size = sizeof(bool);
    } else if (var->value.b != value) {
      mg_bvar_set_changed(var);
    }
    var->value.b = value;
  }
}

void mgos_bvar_set_decimal(mgos_bvar_t var, double value) {
  if (var) {
    if (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_DECIMAL) {
      mg_bvar_close(var, true);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_DECIMAL);
      var->v_size = sizeof(double);
    } else if (var->value.d != value) {
      mg_bvar_set_changed(var);
    }
    var->value.d = value;
  }
}

void mgos_bvar_set_nstr(mgos_bvar_t var, const char *value, size_t value_len) {
  if (!var) return;
  
  if (!value) {
    mgos_bvar_set_null(var);
    return;
  }
  
  if (value_len == -1) value_len = strlen(value);
  
  if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) {
    // re-use previously allocated buffer
    char *ext_buff = NULL;
    if (value_len >= var->v_size) {
      // try to extend previously allocated buffer
      ext_buff = realloc(var->value.s, (value_len + 1));
      if (ext_buff) {
        var->value.s = ext_buff;
        var->v_size = (value_len + 1);
      }
    }
    if (value_len < var->v_size) {
      if (ext_buff || (strncmp(var->value.s, value, value_len) != 0 || value_len != strlen(var->value.s))) {
        strncpy(var->value.s, value, value_len);
        var->value.s[value_len] = '\0';
        mg_bvar_set_changed(var);
      }
      return;
    }
  }

  mg_bvar_close(var, true);
  mg_bvar_set_type(var, MGOS_BVAR_TYPE_STR);
  if (value) {
    var->value.s = MG_BVAR_STRNDUP(value, value_len);
    var->v_size = (value_len + 1);
  }
  mg_bvar_set_changed(var);
}

void mgos_bvar_set_str(mgos_bvar_t var, const char *value) {
  mgos_bvar_set_nstr(var, value, -1);
}

#ifdef MGOS_BVAR_HAVE_DIC
void mg_bvar_set_asc_unchanged(mgos_bvar_t var, char flag) {
  bool on_parent_found(mgos_bvar_t parent, mgos_bvar_t child,
                        struct mg_bvar_dic_key_item *key_item) {
    mg_bvar_set_asc_unchanged(parent, 1);
    return true;
  };
  mg_bvar_dic_walk_parents(var, on_parent_found, NULL);

  if (flag == 1) --var->changed;
}
#endif

void mg_bvar_set_unchanged(mgos_bvar_t var) {
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(var)) {
    mgos_bvar_t v = var->value.dic_head.var;
    while(v) {
      if (mgos_bvar_is_changed(var)) {
        mg_bvar_set_unchanged(v);
      }
      v = mg_bvar_dic_get_key_item(v, var)->key.next_var;
    }
  }
  #endif
  var->changed = 0;
}

void mgos_bvar_set_unchanged(mgos_bvar_t var) {
  if (mgos_bvar_is_changed(var)) {
    #ifdef MGOS_BVAR_HAVE_DIC
    mg_bvar_set_asc_unchanged(var, 0);
    #endif
    mg_bvar_set_unchanged(var);
    var->changed = 0;
  }
}

bool mgos_bvar_is_changed(mgos_bvarc_t var) {
  return (var ? (var->changed > 0) : false);
}

long mgos_bvar_get_integer(mgos_bvarc_t var) {
  switch(mgos_bvar_get_type(var)) {
    case MGOS_BVAR_TYPE_INTEGER:
      return var->value.l;
    case MGOS_BVAR_TYPE_DECIMAL:
      return (long)var->value.d;
    case MGOS_BVAR_TYPE_BOOL:
      return (long)var->value.b;
    default:
      return 0;
  }
}

bool mgos_bvar_get_bool(mgos_bvarc_t var) {
  switch(mgos_bvar_get_type(var)) {
    case MGOS_BVAR_TYPE_BOOL:
      return var->value.b;
    case MGOS_BVAR_TYPE_INTEGER:
      return (var->value.l != 0);
    case MGOS_BVAR_TYPE_DECIMAL:
      return (var->value.d != 0);
    case MGOS_BVAR_TYPE_STR:
      return (var->value.s == NULL ? false : (strlen(var->value.s) > 0));
    default:
      return false;
  }
}

double mgos_bvar_get_decimal(mgos_bvarc_t var) {
  switch(mgos_bvar_get_type(var)) {
    case MGOS_BVAR_TYPE_DECIMAL:
      return var->value.d;
    case MGOS_BVAR_TYPE_INTEGER:
      return (double)var->value.l;
    default:
      return 0.0;
  }
}

const char *mgos_bvar_get_str(mgos_bvarc_t var) {
  return ((mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) ? var->value.s : NULL);
}

#ifdef MGOS_BVAR_HAVE_JSON

#ifdef MGOS_BVAR_HAVE_DIC
int json_printf_bvar_dic_key_item(struct json_out *out, va_list *ap) {
  struct mg_bvar_dic_key_item *key_item = va_arg(*ap, void *);
  return (json_printf(out, "%Q:", key_item->key.name) +
    json_printf(out, "%M", json_printf_bvar, key_item->key.var));
}
#endif

int json_printf_bvar(struct json_out *out, va_list *ap) {
  mgos_bvarc_t var = va_arg(*ap, void *);
  
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(var)) {
    int len = json_printf(out, "{");
    mgos_bvar_t v = var->value.dic_head.var;
    while (v) {
      struct mg_bvar_dic_key_item *key_item = mg_bvar_dic_get_key_item(v, var);
      len += json_printf(out, "%M", json_printf_bvar_dic_key_item, key_item);
      v = key_item->key.next_var;
      if (v) len += json_printf(out, ",");
    }
    len += json_printf(out, "}");
    return len;
  }
  #endif

  switch(mgos_bvar_get_type(var)) {
    case MGOS_BVAR_TYPE_INTEGER:
      return json_printf(out, "%ld", var->value.l);
    case MGOS_BVAR_TYPE_DECIMAL:
      return json_printf(out, "%lf", var->value.d);
    case MGOS_BVAR_TYPE_BOOL:
      return json_printf(out, "%B", var->value.b);
    case MGOS_BVAR_TYPE_STR:
      return json_printf(out, "%Q", var->value.s);
    case MGOS_BVAR_TYPE_NULL:
      return json_printf(out, "%Q", NULL);
  };

  return 0;
}

mgos_bvar_t mgos_bvar_json_bscanf(const char *json, int json_len) {
  mgos_bvar_t var = NULL;
  if (json) {
    struct mg_bvar_json_walk_cb_arg arg = { .var = NULL, .ret = 0 };
    if ((json_walk(json, json_len, mg_bvar_json_walk_cb, &arg) > 0) && (arg.ret != -1)) {
      #ifdef MGOS_BVAR_HAVE_DIC
      var = mg_bvar_dic_get_root(arg.var);
      #else
      var = arg.var;
      #endif
    } else {
      if (mgos_bvar_free(arg.var)) arg.var = NULL;
    }
    mgos_bvar_set_unchanged(var);
  }
  return var;
}

mgos_bvar_t mgos_bvar_json_scanf(const char *json) {
  return mgos_bvar_json_bscanf(json, (json ? strlen(json): 0));
} 

#endif // MGOS_BVAR_HAVE_JSON

bool mgos_bvar_free(mgos_bvar_t var) {
  if (!var) return false;
  
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(var)) {
    mg_bvar_remove_keys(var, true);
  }
  #endif

  #ifdef MGOS_BVAR_HAVE_DIC
  if (var->key_items) {
    // the var belongs to a dictionary, I cannot
    // dispose it
    return false;
  }
  
  #endif
  
  mg_bvar_close(var, true);
  MG_BVAR_FREE(var);
  return true;
}

int mgos_bvar_length(mgos_bvarc_t var) {
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(var)) {
    return var->value.dic_head.count;
  }
  #endif
  if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) {
    return var->value.s ? strlen(var->value.s) : 0;
  }
  return 0;
}

#ifdef MGOS_BVAR_HAVE_DIC

mgos_bvar_t mgos_bvar_new_dic() {
  mgos_bvar_t dic = mg_bvar_dic_ensure(mgos_bvar_new(), false);
  mgos_bvar_set_unchanged(dic);
  return dic;
}

bool mgos_bvar_is_dic(mgos_bvarc_t var) {
  return (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_DIC);
}

void mgos_bvar_remove_keys(mgos_bvar_t var) {
  mg_bvar_remove_keys(var, false);
}

mgos_bvar_t mgos_bvar_remove_key(mgos_bvar_t var, const char *key) {
  mgos_bvar_t v = mg_bvar_dic_get(var, key, (key ? strlen(key) : 0), false);
  mg_bvar_dic_rem_key(var, v);
  return v;
}

void mgos_bvar_delete_key(mgos_bvar_t var, const char *key) {
  mgos_bvar_free(mgos_bvar_remove_key(var, key));
}

bool mgos_bvar_add_key(mgos_bvar_t dic, const char *key_name, mgos_bvar_t key_value) {
  if (!mg_bvar_dic_is_parent(key_value, dic) && !mg_bvar_dic_get(dic, key_name, -1, false)) {
    if (mg_bvar_dic_add(dic, key_name, -1, key_value)) { return true; }
  } 
  return false;
}

mgos_bvar_t mgos_bvar_get_key(mgos_bvar_t var, const char *key) {
  return (mgos_bvar_t)mgos_bvarc_get_key(var, key);
}

bool mgos_bvar_try_get_key(mgos_bvar_t var, const char *key, mgos_bvar_t *out) {
  return mgos_bvarc_try_get_key(var, key, (mgos_bvarc_t *)out);
}

bool mgos_bvar_get_next_key(mgos_bvar_enum_t *key_enum, mgos_bvar_t *out, const char **key_name) {
  return mgos_bvarc_get_next_key((mgos_bvarc_enum_t *)key_enum, (mgos_bvarc_t *)out, key_name);
}

mgos_bvarc_t mgos_bvarc_get_key(mgos_bvarc_t var, const char *key) {
  return mg_bvar_dic_get((mgos_bvar_t)var, key, (key ? strlen(key) : 0), false);
}

bool mgos_bvarc_try_get_key(mgos_bvarc_t var, const char *key, mgos_bvarc_t *out) {
  mgos_bvarc_t ret = mgos_bvarc_get_key(var, key);
  if (out) *out = ret;
  return (ret != NULL);
}

bool mgos_bvarc_get_next_key(mgos_bvarc_enum_t *key_enum, mgos_bvarc_t *out, const char **key_name) {
  if (key_name) *key_name = NULL;
  if (out) *out = NULL;
  if (!key_enum || (*key_enum == NULL)) return false; // no next

  struct mg_bvar_dic_key_item *key_item = (struct mg_bvar_dic_key_item *)*key_enum;
  
  if (out) *out  = key_item->key.var;
  if (key_name) *key_name = key_item->key.name;
  *key_enum = (mgos_bvar_enum_t)mg_bvar_dic_get_key_item(key_item->key.next_var, key_item->parent_dic);
  return true;
}

bool mgos_bvar_has_key(mgos_bvarc_t var, const char *key) {
  return (mg_bvar_dic_get((mgos_bvar_t)var, key, (key ? strlen(key) : 0), false) != NULL);
}

mgos_bvar_enum_t mgos_bvar_get_keys(mgos_bvar_t var) {
  return (mgos_bvar_enum_t)mgos_bvarc_get_keys((mgos_bvarc_t)var);
}

mgos_bvarc_enum_t mgos_bvarc_get_keys(mgos_bvarc_t var) {
  if (!mgos_bvar_is_dic(var)) return NULL;
  return (mgos_bvarc_enum_t)mg_bvar_dic_get_key_item(var->value.dic_head.var, var);
}

#endif //MGOS_BVAR_HAVE_DIC

#ifdef MGOS_HAVE_MJS

#endif /* MGOS_HAVE_MJS */

bool mgos_bvar_init() {
  return true;
}
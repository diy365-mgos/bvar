#include "mg_bvar_sdk.h"
#include "mgos_bvar.h"

#ifdef MGOS_BVAR_HAVE_JSON
#include "mgos_bvar_json.h"
#endif

#ifdef MGOS_BVAR_HAVE_DIC
#include "mgos_bvar_dic.h"
#endif

#ifdef MGOS_BVAR_MEMLEAKS_CHECK
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
#endif //MGOS_BVAR_MEMLEAKS_CHECK

typedef struct mg_bvar_store_base *(* mg_bvar_store_create_t)();

typedef void *(* mg_bvar_store_get_item_t)(struct mg_bvar_store_base *store, short int index);

typedef void (* mg_bvar_store_release_item_t)(void *item);

bool mg_bvar_store_is_item_busy(struct mg_bvar_store_base *store, short int item_index) {
  short int item = (1 << item_index);
  return ((store->busy_items & item) != 0);
}

void mg_bvar_store_set_item_busy(struct mg_bvar_store_base *store, short int item_index, bool busy) {
  short int item = (1 << item_index);
  if (busy) {
    store->busy_items |= item;
    #ifdef MGOS_BVAR_MEMLEAKS_CHECK
    ++MG_BVAR_MEMLEACKS;
    #endif
  } else {
    store->busy_items &= ~item;
    #ifdef MGOS_BVAR_MEMLEAKS_CHECK
    --MG_BVAR_MEMLEACKS;
    #endif
  }
}

short int mg_bvar_store_get_free_index(struct mg_bvar_store_base *store) {
  for (short int i = 0; i < MG_BVAR_STORE_SIZE; ++i) {
    if (!mg_bvar_store_is_item_busy(store, i)) { return i; }
  }
  return -1;
}

void *mg_bvar_store_pick_item(struct mg_bvar_store_base **store,
                              mg_bvar_store_get_item_t get_item,
                              mg_bvar_store_create_t create_new_store) {
  if (*store == NULL) {
    *store = create_new_store();
  }
  short int i = mg_bvar_store_get_free_index(*store);
  void *item;
  if (i != -1) { 
    mg_bvar_store_set_item_busy(*store, i, true);
    item = get_item(*store, i); 
  } else {
    item = mg_bvar_store_pick_item(&((*store)->next_store), get_item, create_new_store);
    if (!(*store)->next_store->prev_store) {
      (*store)->next_store->prev_store = *store;
    }
  }
  return item;
}

struct mg_bvar_store_base *mg_bvar_store_get_by_item(struct mg_bvar_store_base *store, 
                                                     void *item,
                                                     mg_bvar_store_get_item_t get_item,
                                                     short int *item_index) {
  for (short int i = 0; i < MG_BVAR_STORE_SIZE; ++i) {
    if (get_item(store, i) == item) {
      if (item_index) *item_index = i;
      return store; 
    }
  }
  return (store->next_store ? mg_bvar_store_get_by_item(store->next_store, item, get_item, item_index) : NULL);
}

void mg_bvar_store_release_item(struct mg_bvar_store_base **store,
                                void *item,
                                mg_bvar_store_get_item_t get_item,
                                mg_bvar_store_release_item_t release_item) {
  short int item_index;  
  struct mg_bvar_store_base *item_store = mg_bvar_store_get_by_item(*store, item, get_item, &item_index);
  if (!item_store) return;
  release_item(item);
  mg_bvar_store_set_item_busy(item_store, item_index, false);
  
  if (item_store->busy_items != 0) return; // the store is not empty, so exit
  if (!item_store->prev_store && !item_store->next_store) return; // the store is empty, but it is the only allocated, so exit
  struct mg_bvar_store_base *st = *store;
  while (st && (st->busy_items < MG_BVAR_STORE_ALL_BUSY && st->busy_items > 0)) { st = st->next_store; }
  if (!st) return; // there are no partially empty stores, so exit
  if (item_store->next_store) item_store->next_store->prev_store = item_store->prev_store;
  if (!item_store->prev_store)
    *store = item_store->next_store;
  else
    item_store->prev_store->next_store = item_store->next_store;
  free(item_store);
}

static struct mg_bvar_store *s_bvar_store = NULL;

mgos_bvar_t mg_bvar_pick() {
  struct mg_bvar_store_base *create_new_store() {
    return calloc(1, sizeof(struct mg_bvar_store));
  };

  void *get_item(struct mg_bvar_store_base *store, short int index) {
    return &((struct mg_bvar_store *)store)->items[index];
  };
  
  return (mgos_bvar_t)mg_bvar_store_pick_item((struct mg_bvar_store_base **)&s_bvar_store, get_item, create_new_store);
}

void mg_bvar_release(mgos_bvar_t item) {
  void *get_item(struct mg_bvar_store_base *store, short int index) {
    return &((struct mg_bvar_store *)store)->items[index];
  };
  
  void release_item(void *item) {
    mg_bvar_close((mgos_bvar_t)item, true);
    memset(item, 0, sizeof(struct mg_bvar));
  };
  
  mg_bvar_store_release_item((struct mg_bvar_store_base **)&s_bvar_store, item, get_item, release_item);
}

#ifdef MGOS_BVAR_HAVE_DIC

static struct mg_bvar_dic_key_item_store *s_dic_key_item_store = NULL;

struct mg_bvar_dic_key_item *mg_bvar_dic_key_item_pick(const char *key_name, size_t key_len) {
  struct mg_bvar_store_base *create_new_store() {
    return calloc(1, sizeof(struct mg_bvar_dic_key_item_store));
  };

  void *get_item(struct mg_bvar_store_base *store, short int index) {
    return &((struct mg_bvar_dic_key_item_store *)store)->items[index];
  };

  struct mg_bvar_dic_key_item *item = (struct mg_bvar_dic_key_item *)mg_bvar_store_pick_item((struct mg_bvar_store_base **)&s_dic_key_item_store,
    get_item, create_new_store);

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

void mg_bvar_dic_key_item_release(struct mg_bvar_dic_key_item *item) {
  void *get_item(struct mg_bvar_store_base *store, short int index) {
    return &((struct mg_bvar_dic_key_item_store *)store)->items[index];
  };
  
  void release_item(void *item) {
    char *kn = (char *)((struct mg_bvar_dic_key_item *)item)->key.name;
    memset(item, 0, sizeof(struct mg_bvar_dic_key_item));
    ((struct mg_bvar_dic_key_item *)item)->key.name = kn;
  };
  
  mg_bvar_store_release_item((struct mg_bvar_store_base **)&s_dic_key_item_store, item, get_item, release_item);
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

void mg_bvar_close(mgos_bvar_t var, bool free_str) {
  if (var) {
    #ifdef MGOS_BVAR_HAVE_DIC
    if (mgos_bvar_is_dic(var)) {
      mg_bvar_dic_remove_keys(var, true);
      return;
    }
    #endif    
    
    if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR && free_str == false) {
      if (var->value.s != NULL && strlen(var->value.s) > 0) {
        var->value.s[0] = '\0'; // set empty string
        mg_bvar_set_changed(var);
      }
      return;
    }
      
    if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) {
      if (var->value.s != NULL) {
        MG_BVAR_FREE(var->value.s);
        var->value.s = NULL;
        var->v_size = 0;
        mg_bvar_set_changed(var);
      }
    } else {
      union mg_bvar_value *p = (&var->value + (sizeof(union mg_bvar_value)-1));
      while(p >= &var->value && *((char *)p) == '\0') { --p; };
      if (p >= &var->value) {
        // the value is not 0(zero), so I set it to 0(zero) and mark it as changed
        memset(&var->value, 0, sizeof(union mg_bvar_value));
        mg_bvar_set_changed(var);
      }
    }
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
      if (clear) mg_bvar_dic_remove_keys(var, true);
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
  struct mg_bvar_dic_key_item *new_key_item = mg_bvar_dic_key_item_pick(key_name, key_len);
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
    
    mg_bvar_dic_key_item_release(key_item);
    return false;
  };

  mg_bvar_dic_walk_parents(var, on_parent_found, dic);
}

void mg_bvar_dic_remove_keys(mgos_bvar_t var, bool dispose) {
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
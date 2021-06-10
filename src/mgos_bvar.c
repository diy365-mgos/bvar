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

#ifdef MGOS_BVAR_HAVE_DIC
struct mg_bvar_dic_key {
  const char* name;
  mgos_bvar_t var;
  mgos_bvar_t next_var;
  mgos_bvar_t prev_var;
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
  char changed;
  #ifdef MGOS_BVAR_HAVE_DIC
  struct mg_bvar_dic_key *key;
  #endif
};

#ifdef MGOS_BVAR_HAVE_JSON
struct mg_bvar_json_walk_cb_arg {
  mgos_bvar_t var;
  char ret;
};
#endif

#ifdef MGOS_BVAR_HAVE_DIC

mgos_bvar_t mg_bvar_dic_get_root(mgos_bvar_t var) {
  if (!var) return NULL;
  if (var->key) return mg_bvar_dic_get_root(var->key->prev_var);
  return var;
}

mgos_bvar_t mg_bvar_dic_get_parent(mgos_bvar_t var, bool root) {
  if (!var) return NULL;
  mgos_bvar_t p = (var->key ? var->key->prev_var : NULL);
  while (p && !mgos_bvar_is_dic(p)) {
    p = (p->key ? p->key->prev_var : NULL);
  }
  return p;
}

#endif

void mg_bvar_close(mgos_bvar_t var) {
  if (var) {
    #ifdef MGOS_BVAR_HAVE_DIC
    if (mgos_bvar_is_dic(var)) mgos_bvar_remove_keys(var);
    #endif
    if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR) {
      free(var->value.s);
      var->value.s = NULL;
    }
    memset(&var->value, 0, sizeof(union mgos_bvar_value));
    var->v_size = 0;
  }
}

mgos_bvar_t mg_bvar_set_type(mgos_bvar_t var, enum mgos_bvar_type t) {
  if (var) var->type = t;
  return var;
}

void mg_bvar_set_changed(mgos_bvar_t var) {
  if (!var) return;
  #ifdef MGOS_BVAR_HAVE_DIC
  mgos_bvar_t parent = mg_bvar_dic_get_parent(var, false);
  if (parent != var) { 
    mg_bvar_set_changed(parent);
  }
  #endif
  var->changed = 1;
}

#ifdef MGOS_BVAR_HAVE_DIC

mgos_bvar_t mg_bvar_dic_ensure(mgos_bvar_t var, bool clear) {
  if (var) {
    if (mgos_bvar_is_dic(var)) {
      if (clear) mgos_bvar_remove_keys(var);
    } else {
      mg_bvar_close(var);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_DIC);
    }
  }
  return var;
}

bool mg_bvar_dic_add(mgos_bvar_t root, const char *key_name, size_t key_len, mgos_bvar_t var) {
  if (!mg_bvar_dic_ensure(root, false) || !var || var->key || !key_name) return false;

  mgos_bvar_t last_var = root;
  mgos_bvar_t v = root->value.dic_head.var; 
  while (v && v->key) {
    last_var = v;
    v = v->key->next_var;
  };
 
  // init the new key
  var->key = calloc(1, sizeof(struct mg_bvar_dic_key));
  if (key_len == -1) {
    var->key->name = strdup(key_name);
  } else {
    var->key->name = malloc(key_len + 1);
    strncpy((char *)var->key->name, key_name, key_len);
    ((char *)var->key->name)[key_len] = '\0';
  }
  var->key->var = var;
  
  // attach the new var as first var
  var->key->prev_var = last_var;
  if (last_var == root) {
    last_var->value.dic_head.var = var;
  } else {
    last_var->key->next_var = var;
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
    if (strncmp(var->key->name, key_name, key_len) == 0 && key_len == strlen(var->key->name)) return var;
    var = var->key->next_var;
  }

  if (create) {
    // not found...create a new variant
    mgos_bvar_t var = mgos_bvar_new();
    if (mg_bvar_dic_add(root, key_name, key_len, var)) return var;
    mgos_bvar_free(var);
  }

  return NULL;
}

void mg_bvar_dic_rem_key(mgos_bvar_t var) {
  struct mg_bvar_dic_key *key = (var ? var->key : NULL);
  if (key) {
    mgos_bvar_t parent = mg_bvar_dic_get_parent(var, false);
    if (parent) {
      --parent->value.dic_head.count;  // decrease dic length
      mg_bvar_set_changed(parent); // set dic as changed
    }
  
    // start removing key...
    var->key = NULL;
    
    if (key->prev_var) {
      if (mgos_bvar_is_dic(key->prev_var)) {
        key->prev_var->value.dic_head.var = key->next_var;
      } else {
        key->prev_var->key->next_var = key->next_var;
      }
    }
    
    if (key->next_var) {
      key->next_var->key->prev_var = key->prev_var;
    }
    
    free((char *)key->name);
    free(key);
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
    mgos_bvar_t value = mg_bvar_dic_get((mgos_bvar_t)dbig, var->key->name, strlen(var->key->name), false);
    if (!value)
      return MGOS_BVAR_CMP_RES_NOT_EQUAL; // key not found, the twp dictionaries are not equal
    if (mgos_bvar_cmp(value, var) != MGOS_BVAR_CMP_RES_EQUAL)
      return MGOS_BVAR_CMP_RES_NOT_EQUAL; // the two keys are not equal, so the two dictionaries are not equal
    var = var->key->next_var;
  }

  // all keys of the smaller dictionary are euqal to the ones in the
  // bigger dictionary

  if (var1_len == var2_len)
    return MGOS_BVAR_CMP_RES_EQUAL; // all keys are euqal, so the two dictionaries are equal
  else if (var1_len < var2_len)
    return (MGOS_BVAR_CMP_RES_MINOR | MGOS_BVAR_CMP_RES_EQUAL); // an exact copy of var1 is contained into the bigger var2 (var1 < var2)
  else
    return (MGOS_BVAR_CMP_RES_MAJOR | MGOS_BVAR_CMP_RES_EQUAL); // an exact copy of var2 is contained into the bigger var1 (var1 > var2)
}

bool mg_bvar_copy(mgos_bvarc_t, mgos_bvar_t, bool);
bool mg_bvar_dic_copy(mgos_bvarc_t src, mgos_bvar_t dest, bool del_unmatch) {
  if (!mgos_bvar_is_dic(src)) return false;
  if (!mg_bvar_dic_ensure(dest, false)) return false;

  // Sync existing keys and add new keys
  mgos_bvar_t value;
  mgos_bvar_t var = src->value.dic_head.var;
  while(var) {
    value = mg_bvar_dic_get(dest, var->key->name, strlen(var->key->name), true);
    if (mgos_bvar_cmp(value, var) != MGOS_BVAR_CMP_RES_EQUAL) {
      mg_bvar_copy(var, value, del_unmatch);
    }
    var = var->key->next_var;
  }
  
  if (del_unmatch) {
    // Remove not existing keys 
    var = dest->value.dic_head.var;
    while(var) {
      if (!mg_bvar_dic_get((mgos_bvar_t)src, var->key->name, strlen(var->key->name), false)) {
        value = var;
        var = var->key->next_var; 
        mgos_bvar_free(value);
      } else {
        var = var->key->next_var;
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
        mgos_bvar_t parent = mg_bvar_dic_get_parent(arg->var, false);
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
  return (mgos_bvar_t)calloc(1, sizeof(struct mg_bvar));
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
    #ifdef MGOS_BVAR_HAVE_DIC
    if (mgos_bvar_is_dic(var)) {
      mgos_bvar_remove_keys(var);
    }
    #endif
    mg_bvar_close(var);
    mg_bvar_set_type(var, MGOS_BVAR_TYPE_NULL);
    mg_bvar_set_changed(var);
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
      mg_bvar_close(var);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_INTEGER);
      var->v_size = sizeof(long);
      mg_bvar_set_changed(var);
    } else if (value != var->value.l) {
      mg_bvar_set_changed(var);
    }
    var->value.l = value;
  }
}

void mgos_bvar_set_bool(mgos_bvar_t var, bool value) {
  if (var) {
    if (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_BOOL) {
      mg_bvar_close(var);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_BOOL);
      var->v_size = sizeof(bool);
      mg_bvar_set_changed(var);
    } else if (var->value.b != value) {
      mg_bvar_set_changed(var);
    }
    var->value.b = value;
  }
}

void mgos_bvar_set_decimal(mgos_bvar_t var, double value) {
  if (var) {
    if (mgos_bvar_get_type(var) != MGOS_BVAR_TYPE_DECIMAL) {
      mg_bvar_close(var);
      mg_bvar_set_type(var, MGOS_BVAR_TYPE_DECIMAL);
      var->v_size = sizeof(double);
      mg_bvar_set_changed(var);
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
  
  if (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_STR && value_len < var->v_size) {
    // re-use previously allocated buffer
    if (strncmp(var->value.s, value, value_len) != 0 || value_len != strlen(var->value.s)) {
      strncpy(var->value.s, value, value_len);
      var->value.s[value_len] = '\0';
      mg_bvar_set_changed(var);
    }
    return;
  }

  mg_bvar_close(var);
  mg_bvar_set_type(var, MGOS_BVAR_TYPE_STR);
  if (value) {
    var->value.s = strndup(value, value_len);
    var->v_size = (value_len + 1);
  }
  mg_bvar_set_changed(var);
}

void mgos_bvar_set_str(mgos_bvar_t var, const char *value) {
  mgos_bvar_set_nstr(var, value, -1);
}

void mgos_bvar_set_unchanged(mgos_bvar_t var) {
  if (!var) return;
  #ifdef MGOS_BVAR_HAVE_DIC
  if (mgos_bvar_is_dic(var)) {
    mgos_bvar_t v = var->value.dic_head.var;
    while(v) {
      mgos_bvar_set_unchanged(v);
      v = v->key->next_var;
    }
  }
  #endif
  var->changed = 0;
}

bool mgos_bvar_is_changed(mgos_bvarc_t var) {
  return (var ? (var->changed == 1) : false);
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

int json_printf_bvar(struct json_out *out, va_list *ap) {
  mgos_bvarc_t var = va_arg(*ap, void *);
  
  int len = 0;
  #ifdef MGOS_BVAR_HAVE_DIC
  if (var->key) len += json_printf(out, "%Q:", var->key->name);
  if (mgos_bvar_is_dic(var)) {
    len += json_printf(out, "{");
    mgos_bvar_t v = var->value.dic_head.var;
    while (v) {
      len += json_printf(out, "%M", json_printf_bvar, v);
      v = v->key->next_var;
      if (v) len += json_printf(out, ",");
    }
    len += json_printf(out, "}");
  }
  #endif

  switch(mgos_bvar_get_type(var)) {
    case MGOS_BVAR_TYPE_INTEGER:
      len += json_printf(out, "%ld", var->value.l);
      break;
    case MGOS_BVAR_TYPE_DECIMAL:
      len += json_printf(out, "%lf", var->value.d);
      break;
    case MGOS_BVAR_TYPE_BOOL:
      len += json_printf(out, "%B", var->value.b);
      break;
    case MGOS_BVAR_TYPE_STR:
      len += json_printf(out, "%Q", var->value.s);
      break;
    case MGOS_BVAR_TYPE_NULL:
      len += json_printf(out, "%Q", NULL);
      break;
  };

  return len;
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
      mgos_bvar_free(arg.var);
    }
    mgos_bvar_set_unchanged(var);
  }
  return var;
}

mgos_bvar_t mgos_bvar_json_scanf(const char *json) {
  return mgos_bvar_json_bscanf(json, (json ? strlen(json): 0));
} 

#endif // MGOS_BVAR_HAVE_JSON

void mgos_bvar_free(mgos_bvar_t var) {
  if (!var) return;
  
  #ifdef MGOS_BVAR_HAVE_DIC
  bool is_dic = mgos_bvar_is_dic(var);
  if (is_dic) {
    while (var->value.dic_head.var) {
      mgos_bvar_free(var->value.dic_head.var);
    }
  }
  #endif

  #ifdef MGOS_BVAR_HAVE_DIC
  mg_bvar_dic_rem_key(var);
  #endif
  
  mg_bvar_close(var);
  free(var);
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
  return mg_bvar_dic_ensure(mgos_bvar_new(), false);
}

bool mgos_bvar_is_dic(mgos_bvarc_t var) {
  return (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_DIC);
}

void mgos_bvar_remove_keys(mgos_bvar_t var, bool dispose) {
  if (mgos_bvar_is_dic(var)) {
    void (*del_func)(mgos_bvar_t);
    del_func = (dispose ? mg_bvar_dic_rem_key : mgos_bvar_free);
    while (var->value.dic_head.var) {
      del_func(var->value.dic_head.var);
    }
  }
}

mgos_bvar_t mgos_bvar_remove_key(mgos_bvar_t var, const char *key, bool dispose) {
  void (*del_func)(mgos_bvar_t);
  del_func = (dispose ? mg_bvar_dic_rem_key : mgos_bvar_free);
  mgos_bvar_t v = mg_bvar_dic_get(var, key, (key ? strlen(key) : 0), false);
  if (v) del_func(v);
  return (dispose ? NULL : v);
}

bool mgos_bvar_add_key(mgos_bvar_t var, const char *key, mgos_bvar_t val) {
  if (mg_bvar_dic_get(var, key, -1, false) == NULL) {
    if (mg_bvar_dic_add(var, key, -1, val)) { return true; }
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

  mgos_bvar_t var = (mgos_bvar_t)*key_enum;

  if (mgos_bvar_is_dic(var)) {
    var = var->value.dic_head.var;
  }
  
  if (!var || !var->key) return false;
  
  if (out) *out  = var;
  if (key_name) *key_name = var->key->name;
  *key_enum = (mgos_bvar_enum_t)var->key->next_var;
  return true;
}

bool mgos_bvar_has_key(mgos_bvarc_t var, const char *key) {
  return (mg_bvar_dic_get((mgos_bvar_t)var, key, (key ? strlen(key) : 0), false) != NULL);
}

mgos_bvar_enum_t mgos_bvar_get_keys(mgos_bvar_t var) {
  return (mgos_bvar_enum_t)var;
}

mgos_bvarc_enum_t mgos_bvarc_get_keys(mgos_bvarc_t var) {
  return (mgos_bvarc_enum_t)var;
}

#endif //MGOS_BVAR_HAVE_DIC

#ifdef MGOS_HAVE_MJS

#endif /* MGOS_HAVE_MJS */

bool mgos_bvar_init() {
  return true;
}

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "mgos_bvar.h"
#include "mg_bvar_sdk.h"

#ifdef MGOS_BVAR_HAVE_JSON
#include "mgos_bvar_json.h"
#endif

#ifdef MGOS_BVAR_HAVE_DIC
#include "mgos_bvar_dic.h"
#endif

#ifdef MGOS_HAVE_MJS
#include "mjs.h"
#endif /* MGOS_HAVE_MJS */

bool mgos_bvar_is_null(mgos_bvarc_t var) {
  return (mgos_bvar_get_type(var) == MGOS_BVAR_TYPE_NULL);
}

mgos_bvar_t mgos_bvar_new() {
  return mg_bvar_pick();
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
    mg_bvar_dic_remove_keys(var, true);
  }
  #endif

  #ifdef MGOS_BVAR_HAVE_DIC
  if (var->key_items) {
    // the var belongs to a dictionary, I cannot
    // dispose it
    return false;
  }
  
  #endif

  mg_bvar_release(var);
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
  mg_bvar_dic_remove_keys(var, false);
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
/*
 * Copyright (c) 2021 DIY365
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MG_BVAR_SDK_H_
#define MG_BVAR_SDK_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mgos_bvar.h"

#ifdef MGOS_BVAR_HAVE_JSON
#include "frozen.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MGOS_BVAR_HAVE_DIC
struct mg_bvar_dic_head {
  mgos_bvar_t var;
  int count;
};
#endif

union mg_bvar_value {
   long l;
   double d;
   bool b;
   char *s;
   #ifdef MGOS_BVAR_HAVE_DIC
   struct mg_bvar_dic_head dic_head;
   #endif
};

#ifdef MGOS_BVAR_MEMLEAKS_CHECK
char *mg_bvar_strdup_hook(const char *str);
char *mg_bvar_strndup_hook(const char *str, size_t size);
void mg_bvar_free_hook(void *p);
int mg_bvar_get_memleaks();
bool mg_bvar_has_memleaks();

#define MG_BVAR_STRDUP(arg) mg_bvar_strdup_hook(arg)
#define MG_BVAR_STRNDUP(arg1, arg2) mg_bvar_strndup_hook(arg1, arg2)
#define MG_BVAR_FREE(arg) mg_bvar_free_hook(arg)
#else
#define MG_BVAR_STRDUP(arg) strdup(arg)
#define MG_BVAR_STRNDUP(arg1, arg2) strndup(arg1, arg2)
#define MG_BVAR_FREE(arg) free(arg)
#endif

#define MG_BVAR_STORE_SIZE 8       // 8 bits
#define MG_BVAR_STORE_ALL_BUSY 255 //(1111-1111) all 8 bits are ON

struct mg_bvar_store_base;

struct mg_bvar_store_base {
  short int busy_items;
  struct mg_bvar_store_base *next_store;
  struct mg_bvar_store_base *prev_store;
};

#ifdef MGOS_BVAR_HAVE_DIC

struct mg_bvar_dic_key {
  const char* name;
  size_t name_size;
  mgos_bvar_t var;
  mgos_bvar_t next_var;
  mgos_bvar_t prev_var;
};

struct mg_bvar_dic_key_item {
  mgos_bvar_t parent_dic;
  struct mg_bvar_dic_key key;
  struct mg_bvar_dic_key_item *next_item;
  struct mg_bvar_dic_key_item *prev_item;
};

struct mg_bvar_dic_key_item_store {
  struct mg_bvar_store_base base;
  struct mg_bvar_dic_key_item items[MG_BVAR_STORE_SIZE];
};

#endif

struct mg_bvar {
  enum mgos_bvar_type type; 
  union mg_bvar_value value;
  size_t v_size;
  int changed;
  #ifdef MGOS_BVAR_HAVE_DIC
  struct mg_bvar_dic_key_item *key_items;
  #endif
};

struct mg_bvar_store {
  struct mg_bvar_store_base base;
  struct mg_bvar items[MG_BVAR_STORE_SIZE];
};

#ifdef MGOS_BVAR_HAVE_JSON
struct mg_bvar_json_walk_cb_arg {
  mgos_bvar_t var;
  char ret;
};
#endif

#ifdef MGOS_BVAR_HAVE_DIC

static struct mg_bvar_dic_key_item *mg_bvar_dic_key_item_pop();

void mg_bvar_dic_key_item_rel(struct mg_bvar_dic_key_item *item);

struct mg_bvar_dic_key_item *mg_bvar_dic_key_item_alloc(const char *key_name, size_t key_len);

void mg_bvar_dic_key_item_free(struct mg_bvar_dic_key_item *item);

struct mg_bvar_dic_key_item *mg_bvar_dic_get_key_item(mgos_bvarc_t var, mgos_bvarc_t parent_dic);

bool mg_bvar_dic_is_parent(mgos_bvarc_t var, mgos_bvarc_t parent_dic);

mgos_bvar_t mg_bvar_dic_get_root(mgos_bvar_t var);

mgos_bvar_t mg_bvar_dic_get_parent(mgos_bvar_t var);

typedef bool (* mg_bvar_dic_walk_parents_t)(mgos_bvar_t parent, mgos_bvar_t child,
                                            struct mg_bvar_dic_key_item *key_item);

int mg_bvar_dic_walk_parents(mgos_bvar_t var, mg_bvar_dic_walk_parents_t walk_func, mgos_bvar_t filter);
#endif

mgos_bvar_t mg_bvar_pick();
void mg_bvar_release(mgos_bvar_t item);

void mg_bvar_set_changed(mgos_bvar_t var);

void mg_bvar_close(mgos_bvar_t var, bool free_str);

mgos_bvar_t mg_bvar_set_type(mgos_bvar_t var, enum mgos_bvar_type t);

#ifdef MGOS_BVAR_HAVE_DIC

mgos_bvar_t mg_bvar_dic_ensure(mgos_bvar_t var, bool clear);

bool mg_bvar_dic_add(mgos_bvar_t root, const char *key_name, size_t key_len, mgos_bvar_t var);

mgos_bvar_t mg_bvar_dic_get(mgos_bvar_t root, const char *key_name, size_t key_len, bool create);

void mg_bvar_dic_rem_key(mgos_bvar_t dic, mgos_bvar_t var);

void mg_bvar_dic_remove_keys(mgos_bvar_t var, bool dispose);

/* mg_bvar_dic_cmp - compares 2 dictinaries 
 * Returns:
     - MGOS_BVAR_CMP_RES_EQUAL: the two dictionaries are equal
     - MGOS_BVAR_CMP_RES_NOT_EQUAL: the two dictionaries are not equal
     - (MGOS_BVAR_CMP_RES_MINOR | MGOS_BVAR_CMP_RES_EQUAL): var1 is contained, as exact copy, into var2 (var1 < var2)
     - (MGOS_BVAR_CMP_RES_MAJOR | MGOS_BVAR_CMP_RES_EQUAL): var1 contains an exact copy of var2 (var1 > var2)
*/
enum mgos_bvar_cmp_res mg_bvar_dic_cmp(mgos_bvarc_t var1, mgos_bvarc_t var2);

bool mg_bvar_dic_copy(mgos_bvarc_t src, mgos_bvar_t dest, bool del_unmatch);

#endif //MGOS_BVAR_HAVE_DIC

bool mg_bvar_copy(mgos_bvarc_t, mgos_bvar_t, bool);

#ifdef MGOS_BVAR_HAVE_JSON

void mg_bvar_json_walk_cb(void *callback_data,
                          const char *name, size_t name_len,
                          const char *path,
                          const struct json_token *token);

#endif // MGOS_BVAR_HAVE_JSON

#ifdef MGOS_BVAR_HAVE_DIC
void mg_bvar_set_asc_unchanged(mgos_bvar_t var, char flag);
#endif

void mg_bvar_set_unchanged(mgos_bvar_t var);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_BVAR_SDK_H_ */
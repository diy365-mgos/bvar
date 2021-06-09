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

#ifndef MGOS_BVAR_H_
#define MGOS_BVAR_H_

/* Uncomment lines below for running unit tests
 * (run unit-tests.c)
 */
//#define MGOS_BVAR_HAVE_DIC 1
//#define MGOS_BVAR_HAVE_JSON 1

#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#ifdef MGOS_HAVE_MJS
#include "mjs.h"
#endif /* MGOS_HAVE_MJS */ 

#ifdef __cplusplus
extern "C" {
#endif

enum mgos_bvar_type {
  MGOS_BVAR_TYPE_NULL = 0,
  MGOS_BVAR_TYPE_BOOL = 2,
  MGOS_BVAR_TYPE_INTEGER = 3,
  MGOS_BVAR_TYPE_DECIMAL = 4,
  MGOS_BVAR_TYPE_STR = 5
};

struct mg_bvar;
typedef struct mg_bvar *mgos_bvar_t;
typedef const struct mg_bvar *mgos_bvarc_t;

#define MGOS_BVAR_CONST(v) ((mgos_bvarc_t)v)

mgos_bvar_t mgos_bvar_new();            
mgos_bvar_t mgos_bvar_new_integer(long value);
mgos_bvar_t mgos_bvar_new_bool(bool value);
mgos_bvar_t mgos_bvar_new_decimal(double value);
mgos_bvar_t mgos_bvar_new_str(const char *value);
mgos_bvar_t mgos_bvar_new_nstr(const char *value, int value_len);

void mgos_bvar_set_null(mgos_bvar_t var);                                 
void mgos_bvar_set_integer(mgos_bvar_t var, long value);
void mgos_bvar_set_bool(mgos_bvar_t var, bool value);
void mgos_bvar_set_decimal(mgos_bvar_t var, double value);
void mgos_bvar_set_str(mgos_bvar_t var, const char *value);
void mgos_bvar_set_nstr(mgos_bvar_t var, const char *value, size_t value_len);

long mgos_bvar_get_integer(mgos_bvarc_t var);
bool mgos_bvar_get_bool(mgos_bvarc_t var);
double mgos_bvar_get_decimal(mgos_bvarc_t var);
const char *mgos_bvar_get_str(mgos_bvarc_t var);

enum mgos_bvar_type mgos_bvar_get_type(mgos_bvarc_t var);

bool mgos_bvar_is_null(mgos_bvarc_t var);

bool mgos_bvar_copy(mgos_bvarc_t src_var, mgos_bvar_t dest_var);
bool mgos_bvar_merge(mgos_bvarc_t src_var, mgos_bvar_t dest_var);

int mgos_bvar_cmp(mgos_bvarc_t var1, mgos_bvarc_t var2);

int mgos_bvar_length(mgos_bvarc_t var);

void mgos_bvar_set_unchanged(mgos_bvar_t var);
bool mgos_bvar_is_changed(mgos_bvarc_t var);

void mgos_bvar_free(mgos_bvar_t var);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_BVAR_H_ */

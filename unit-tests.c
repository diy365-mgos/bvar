#include <stdio.h>
#include <string.h>
#include "mgos_bvar.h"

#ifdef MGOS_BVAR_HAVE_JSON
#include "mgos_bvar_json.h"
#endif

#ifdef MGOS_BVAR_HAVE_DIC
#include "mgos_bvar_dic.h"
#endif

#define FAIL(str, line)                                        \
  do {                                                         \
    fprintf(stderr, "Fail on line %d: [%s]\n", line, str);     \
    fprintf(stderr, "Tests failed. See above for details.\n"); \
    return 0;                                                  \
  } while (0)

#define ASSERT(expr)                    \
  do {                                  \
    if (!(expr)) FAIL(#expr, __LINE__); \
  } while (0)

int main()
{
  printf("Tests in progress...\n");
  #ifdef MGOS_BVAR_HAVE_JSON
  printf("  - Testing JSON          [X]\n");
  #else
  printf("  - Testing JSON          [ ]\n");
  printf("    (uncomment line 23 in 'mgos_bvar.h' to enable JSON tests)\n");
  #endif
  #ifdef MGOS_BVAR_HAVE_DIC
  printf("  - Testing dictionary    [X]\n");
  #else
  printf("  - Testing dictionary    [ ]\n");
  printf("    (uncomment line 22 in 'mgos_bvar.h' to enable dictionary tests)\n");
  #endif
  printf("\n");
  
  mgos_bvar_t v1 = NULL, v2 = NULL;
  mgos_bvarc_t cv1 = NULL;
  #ifdef MGOS_BVAR_HAVE_DIC
  mgos_bvarc_enum_t e1 = NULL;
  #endif
  const char *key1, key2;
  char *json;
  
  ASSERT(mgos_bvar_is_null(v1));
  ASSERT(mgos_bvar_is_null(v2));
  
  v1 = mgos_bvar_new();
  v2 = mgos_bvar_new();
  ASSERT(mgos_bvar_is_null(v1));
  ASSERT(!mgos_bvar_is_changed(v1));
  ASSERT(mgos_bvar_is_null(v2));
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_str("Mark");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Mark") == 0);
  ASSERT(mgos_bvar_length(v1) == 4);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_set_null(v1);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(mgos_bvar_is_null(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new_integer(101);
  ASSERT(mgos_bvar_get_integer(v1) == 101);
  ASSERT(mgos_bvar_get_decimal(v1) == 101.00);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new_decimal(101.10);
  ASSERT(mgos_bvar_get_decimal(v1) == 101.10);
  ASSERT(mgos_bvar_get_integer(v1) == 101);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_bool(true);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_set_decimal(v1, 101.10);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_set_decimal(v1, 101.10);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_set_integer(v1, 101);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_set_integer(v1, 101);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_set_bool(v1, false);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_set_bool(v1, false);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_set_str(v1, NULL);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_set_str(v1, "Pippo");
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_set_str(v1, "Pippo");
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_set_str(v1, NULL);
  ASSERT(mgos_bvar_is_changed(v1));
  ASSERT(mgos_bvar_is_null(v1));
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_NULL);
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_set_decimal(v1, 101.10);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  mgos_bvar_set_decimal(v1, -101.10);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  mgos_bvar_set_decimal(v1, 0.0);
  ASSERT(mgos_bvar_get_bool(v1) == false);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_set_integer(v1, 101);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  mgos_bvar_set_integer(v1, -101);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  mgos_bvar_set_integer(v1, 0);
  ASSERT(mgos_bvar_get_bool(v1) == false);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_set_str(v1, "Mark");
  ASSERT(mgos_bvar_get_bool(v1) == true);
  mgos_bvar_set_str(v1, "");
  ASSERT(mgos_bvar_get_bool(v1) == false);
  mgos_bvar_set_str(v1, NULL);
  ASSERT(mgos_bvar_get_bool(v1) == false);
  mgos_bvar_free(v1);
   
  v1 = mgos_bvar_new();
  ASSERT(mgos_bvar_get_bool(v1) == false);
  mgos_bvar_free(v1);
   
  v1 = mgos_bvar_new_str("same");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "same") == 0);
  mgos_bvar_set_str(v1, "same_name_and_surname");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "same_name_and_surname") == 0);
  mgos_bvar_set_str(v1, "same_name");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "same_name") == 0);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_set_str(v1, "Mark1");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Mark1") == 0);
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Mark2") != 0);
  mgos_bvar_set_str(v1, "Mark2");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Mark1") != 0);
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Mark2") == 0);
  mgos_bvar_set_null(v1);
  ASSERT(mgos_bvar_is_null(v1));
  mgos_bvar_free(v1);
  
  ASSERT(mgos_bvar_cmp(NULL, NULL) == 0);
  
  v1 = mgos_bvar_new_bool(false);
  v2 = mgos_bvar_new_integer(10);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_NOT_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_decimal(10.00);
  v2 = mgos_bvar_new_integer(10);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_set_decimal(v2, 10.1);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_MINOR);
  ASSERT(mgos_bvar_cmp(v2, v1) == MGOS_BVAR_CMP_RES_MAJOR);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  v2 = mgos_bvar_new();
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  ASSERT(mgos_bvar_cmp(v1, NULL) == MGOS_BVAR_CMP_RES_NOT_EQUAL);
  ASSERT(mgos_bvar_cmp(NULL, v2) == MGOS_BVAR_CMP_RES_NOT_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_str("A");
  v2 = mgos_bvar_new_str("B");
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_MINOR);
  ASSERT(mgos_bvar_cmp(v2, v1) == MGOS_BVAR_CMP_RES_MAJOR);
  mgos_bvar_set_str(v2, "A");
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_integer(10);
  v2 = mgos_bvar_new_integer(20);
  ASSERT(mgos_bvar_cmp(v1, v2) < MGOS_BVAR_CMP_RES_MINOR);
  ASSERT(mgos_bvar_cmp(v2, v1) > MGOS_BVAR_CMP_RES_MAJOR);
  mgos_bvar_set_integer(v2, 10);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_decimal(10.22);
  v2 = mgos_bvar_new_decimal(20.22);
  ASSERT(mgos_bvar_cmp(v1, v2) < MGOS_BVAR_CMP_RES_MINOR);
  ASSERT(mgos_bvar_cmp(v2, v1) > MGOS_BVAR_CMP_RES_MAJOR);
  mgos_bvar_set_decimal(v2, 10.22);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_bool(false);
  v2 = mgos_bvar_new_bool(true);
  ASSERT(mgos_bvar_cmp(v1, v2) < MGOS_BVAR_CMP_RES_MINOR);
  ASSERT(mgos_bvar_cmp(v2, v1) > MGOS_BVAR_CMP_RES_MAJOR);
  mgos_bvar_set_bool(v2, false);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_str("Marco");
  v2 = mgos_bvar_new();
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_integer(124);
  v2 = mgos_bvar_new();
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_decimal(123.88);
  v2 = mgos_bvar_new();
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_bool(true);
  v2 = mgos_bvar_new();
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_integer(1001);
  v2 = mgos_bvar_new_integer(1001);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_decimal(1001);
  v2 = mgos_bvar_new_decimal(1001.00);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_EQUAL);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
 
  v1 = mgos_bvar_new_bool(true);
  v2 = mgos_bvar_new_bool(false);
  ASSERT(mgos_bvar_cmp(v1, v2) == MGOS_BVAR_CMP_RES_NOT_EQUAL);
  ASSERT(mgos_bvar_get_bool(v1) == true);
  ASSERT(mgos_bvar_get_bool(v2) == false);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_bool(true);
  v2 = mgos_bvar_new_bool(true);
  mgos_bvar_copy(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_str("Mark");
  v2 = mgos_bvar_new_str("Mark");
  mgos_bvar_copy(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_integer(101);
  v2 = mgos_bvar_new_integer(101);
  mgos_bvar_copy(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_decimal(12.33);
  v2 = mgos_bvar_new_decimal(12.33);
  mgos_bvar_copy(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
 
  #ifdef MGOS_BVAR_HAVE_JSON

  v1 = mgos_bvar_new_str("Mark");
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "\"Mark\"") == 0);
  mgos_bvar_free(v1);
  free(json);
  
  v1 = mgos_bvar_new_str(NULL);
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "null") == 0);
  mgos_bvar_free(v1);
  free(json);
 
  v1 = mgos_bvar_new_integer(234);
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "234") == 0);
  mgos_bvar_free(v1);
  free(json);
   
  v1 = mgos_bvar_new_bool(true);
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "true") == 0);
  mgos_bvar_free(v1);
  free(json);
  
  v1 = mgos_bvar_new_decimal(122.20);
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "122.200000") == 0);
  mgos_bvar_free(v1);
  free(json);
  
  v1 = mgos_bvar_json_scanf("234");
  ASSERT(mgos_bvar_get_integer(v1) == 234);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("378.340");
  ASSERT(mgos_bvar_get_decimal(v1) == 378.34);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("true");
  ASSERT(mgos_bvar_get_bool(v1) == true);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_json_scanf("false");
  ASSERT(mgos_bvar_get_bool(v1) == false);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("null");
  ASSERT(mgos_bvar_is_null(v1));
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("\"AAA\"");
  ASSERT(strcmp(mgos_bvar_get_str(v1), "AAA") == 0);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("AAA");
  ASSERT(v1 == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("");
  ASSERT(v1 == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf(NULL);
  ASSERT(v1 == NULL);
  mgos_bvar_free(v1);
  
  #if !MGOS_BVAR_HAVE_DIC
  
  v1 = mgos_bvar_json_scanf("{}");
  ASSERT(v1 != NULL);
  ASSERT(mgos_bvar_is_null(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\", \"Age\":46}");
  ASSERT(v1 != NULL);
  ASSERT(mgos_bvar_is_null(v1));
  mgos_bvar_free(v1);
  
  #endif // !MGOS_BVAR_HAVE_DIC
  
  #endif // MGOS_BVAR_HAVE_JSON

  #ifdef MGOS_BVAR_HAVE_DIC
  
  v1 = mgos_bvar_new_dic();
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(!mgos_bvar_is_null(v1));
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
   
  v1 = mgos_bvar_new_dic();
  mgos_bvar_set_null(v1);
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(mgos_bvar_is_null(v1));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvarc_try_get_key(v1, "Name", &cv1) == true);
  ASSERT(mgos_bvarc_try_get_key(v1, "Age", &cv1) == false);
  ASSERT(mgos_bvar_try_get_key(v1, "Age", &v2) == false);
  ASSERT(mgos_bvar_try_get_key(v1, "Name", &v2) == true);
  mgos_bvar_set_str(v2, "Greg");
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Greg") == 0);
  mgos_bvar_free(v1);
   
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Surname", mgos_bvar_new_str("Smith"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(mgos_bvar_length(v1) == 3);
  mgos_bvar_set_null(v1);
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(!mgos_bvar_is_dic(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 2);
  mgos_bvar_set_str(v1, "Is not a dictionary");
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == strlen(mgos_bvar_get_str(v1)));
  ASSERT(strcmp(mgos_bvar_get_str(v1), "Is not a dictionary") == 0);
  mgos_bvar_free(v1);
     
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  ASSERT(mgos_bvar_length(v1) == 2);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_dic();
  v2 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_copy(v2, v1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  ASSERT(mgos_bvar_length(v1) == 2);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
 
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(mgos_bvar_length(v1) == 2);
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Surname", mgos_bvar_new_str("Smith"));
  mgos_bvar_copy(v2, v1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Surname")), "Smith") == 0);
  ASSERT(mgos_bvar_length(v1) == 1);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Surname", mgos_bvar_new_str("Smith"));
  v2 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_length(v2) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v2, "Surname")), "Smith") == 0);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  v2 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_merge(v2, v1);
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 2);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);

  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new_decimal(124.67);
  mgos_bvar_merge(v2, v1);
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(mgos_bvar_get_decimal(v1) == 124.67);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);

  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Surname", mgos_bvar_new_str("Smith"));
  v2 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_merge(v2, v1);
  ASSERT(mgos_bvar_length(v1) == 3);
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  ASSERT(mgos_bvar_length(v1) == 1);
  mgos_bvar_free(v1);
   
  v1 = mgos_bvar_new_dic();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  ASSERT(mgos_bvar_length(v1) == 3);
  mgos_bvar_remove_keys(v1, true);
  ASSERT(mgos_bvar_length(v1) == 0);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_NULL);
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_new_bool(false);
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_BOOL);
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_str("Mark");
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_STR);
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_integer(1001);
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_INTEGER);
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_decimal(12.44);
  ASSERT(!mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_get_type(v1) == MGOS_BVAR_TYPE_DECIMAL);
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_str(v1) == NULL);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(mgos_bvar_has_key(v1, "Age"));
  ASSERT(!mgos_bvar_has_key(v1, "Surname"));
  mgos_bvar_remove_key(v1, "Age", true);    
  ASSERT(mgos_bvar_length(v1) == 2);
  ASSERT(!mgos_bvar_has_key(v1, "Age"));
  mgos_bvar_remove_key(v1, "Weigth", true);
  ASSERT(mgos_bvar_length(v1) == 1);
  ASSERT(!mgos_bvar_has_key(v1, "Weigth"));
  mgos_bvar_remove_key(v1, "Name", true);
  ASSERT(mgos_bvar_length(v1) == 0);
  ASSERT(!mgos_bvar_has_key(v1, "Name"));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  ASSERT(!mgos_bvar_is_dic(v1));
  e1 = mgos_bvar_get_keys(v1);
  ASSERT(e1 != NULL);
  ASSERT(mgos_bvarc_get_next_key(&e1, NULL, NULL) == false);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new_dic();
  ASSERT(mgos_bvar_is_dic(v1));
  e1 = mgos_bvar_get_keys(v1);
  ASSERT(e1 != NULL);
  ASSERT(mgos_bvarc_get_next_key(&e1, NULL, NULL) == false);
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  ASSERT(mgos_bvar_is_dic(v1));
  e1 = mgos_bvarc_get_keys(v1);
  ASSERT(e1 != NULL);
  {
    mgos_bvarc_t v = NULL; const char *k = NULL; int i = 0;
    for (i = 0; mgos_bvarc_get_next_key(&e1, &v, &k); ++i) {
      ASSERT(mgos_bvar_cmp(mgos_bvarc_get_key(v1, k), v) == MGOS_BVAR_CMP_RES_EQUAL);
    }
    ASSERT(i == mgos_bvar_length(v1));
  }
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  ASSERT(mgos_bvar_is_dic(v1));
  e1 = mgos_bvar_get_keys(v1);
  ASSERT(e1 != NULL);
  {
    mgos_bvarc_t v = NULL; const char *k = NULL; int i = 0;
    for (i = 0; mgos_bvarc_get_next_key(&e1, &v, &k); ++i) {
      ASSERT(mgos_bvar_cmp(mgos_bvarc_get_key(v1, k), v) == MGOS_BVAR_CMP_RES_EQUAL);
    }
    ASSERT(i == mgos_bvar_length(v1));
  }
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Surname", mgos_bvar_new_str("Smith"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  ASSERT(mgos_bvar_length(v1) == 4);
  mgos_bvar_free(mgos_bvar_get_key(v1, "Age")); // remove middle item
  ASSERT(mgos_bvar_length(v1) == 3);
  mgos_bvar_free(mgos_bvar_get_key(v1, "Weigth")); // remove tail item
  ASSERT(mgos_bvar_length(v1) == 2);
  mgos_bvar_free(mgos_bvar_get_key(v1, "Name")); // remove head item
  ASSERT(mgos_bvar_length(v1) == 1);
  mgos_bvar_free(mgos_bvar_get_key(v1, "Surname")); // remove single item
  ASSERT(mgos_bvar_length(v1) == 0);
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(90.2));
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Employee", v1);
  mgos_bvar_add_key(v2, "Type", mgos_bvar_new_str("EMPLOYEE"));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(mgos_bvar_length(v2) == 2);
  ASSERT(mgos_bvar_length(mgos_bvarc_get_key(v2, "Employee")) == 3);
  mgos_bvar_free(v1);
  ASSERT(mgos_bvar_length(v2) == 1);
  ASSERT(!mgos_bvar_has_key(v2, "Employee"));
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);

  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Code", mgos_bvar_new_integer(999));
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Type", mgos_bvar_new_str("TYPE_01"));
  mgos_bvar_add_key(v2, "Employee", v1);
  mgos_bvar_set_unchanged(v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_set_integer(mgos_bvar_get_key(mgos_bvar_get_key(v2, "Employee"), "Code"), 888);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Code")) == 888);
  ASSERT(mgos_bvar_is_changed(v2));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new_dic();
  mgos_bvar_remove_keys(v1, true);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v1);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_remove_keys(v1, true);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v1);
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_remove_key(v1, "Name", true);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v1);
  mgos_bvar_free(mgos_bvar_get_key(v1, "Name"));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v1);
  v2 = mgos_bvar_get_key(v1, "Name");
  mgos_bvar_set_str(v2, "Rosy");
  ASSERT(mgos_bvar_is_changed(v2));
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_set_unchanged(v2);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new();
  mgos_bvar_copy(v1, v2);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v2);
  mgos_bvar_copy(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
   v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new();
  mgos_bvar_merge(v1, v2);
  ASSERT(mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_set_unchanged(v2);
  mgos_bvar_merge(v1, v2);
  ASSERT(!mgos_bvar_is_changed(v2));
  mgos_bvar_free(v1);
  mgos_bvar_free(v2);
  
  #ifdef MGOS_BVAR_HAVE_JSON

  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Surname", mgos_bvar_new_str(NULL));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  mgos_bvar_add_key(v1, "Weigth", mgos_bvar_new_decimal(102.44));
  mgos_bvar_add_key(v1, "Enable", mgos_bvar_new_bool(true));
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "{\"Name\":\"Mark\",\"Surname\":null,\"Age\":46,\"Weigth\":102.440000,\"Enable\":true}") == 0);
  mgos_bvar_free(v1);
  free(json);

  v1 = mgos_bvar_new();
  mgos_bvar_add_key(v1, "Name", mgos_bvar_new_str("Mark"));
  mgos_bvar_add_key(v1, "Age", mgos_bvar_new_integer(46));
  v2 = mgos_bvar_new();
  mgos_bvar_add_key(v2, "Name", mgos_bvar_new_str("Gregory"));
  mgos_bvar_add_key(v2, "Age", mgos_bvar_new_integer(80));
  mgos_bvar_add_key(v1, "Fhater", v2);
  json = json_asprintf("%M", json_printf_bvar, v1);
  ASSERT(strcmp(json, "{\"Name\":\"Mark\",\"Age\":46,\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80}}") == 0);
  mgos_bvar_free(v1);
  free(json);

  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\",\"Surname\":null,\"Age\":46,\"Weigth\":102.440000,\"Enable\":true}");
  ASSERT(!mgos_bvar_is_changed(v1));
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\",\"Surname\":null,\"Age\":46,\"Weigth\":102.440000,\"Enable\":true}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 5);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_is_null(mgos_bvarc_get_key(v1, "Surname")));
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  ASSERT(mgos_bvar_get_decimal(mgos_bvarc_get_key(v1, "Weigth")) == 102.44);
  ASSERT(mgos_bvar_get_bool(mgos_bvarc_get_key(v1, "Enable")) == true);
  mgos_bvar_free(v1);

  v1 = mgos_bvar_json_scanf("{\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80}}");
  ASSERT(!mgos_bvar_is_changed(v1));
  ASSERT(!mgos_bvar_is_changed(mgos_bvarc_get_key(v1, "Fhater")));
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("{\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80}}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 1);
  cv1 = mgos_bvarc_get_key(v1, "Fhater");
  ASSERT(mgos_bvar_is_dic(cv1));
  ASSERT(mgos_bvar_length(cv1) == 2);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Gregory") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 80);
  mgos_bvar_free(v1);

  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\",\"Age\":46,\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80}}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  cv1 = mgos_bvarc_get_key(v1, "Fhater");
  ASSERT(mgos_bvar_is_dic(cv1));
  ASSERT(mgos_bvar_length(cv1) == 2);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Gregory") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 80);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\",\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80},\"Age\":46}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  cv1 = mgos_bvarc_get_key(v1, "Fhater");
  ASSERT(mgos_bvar_is_dic(cv1));
  ASSERT(mgos_bvar_length(cv1) == 2);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Gregory") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 80);
  mgos_bvar_free(v1);
 
  v1 = mgos_bvar_json_scanf("{\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80},\"Age\":46,\"Name\":\"Mark\"}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  cv1 = mgos_bvarc_get_key(v1, "Fhater");
  ASSERT(mgos_bvar_is_dic(cv1));
  ASSERT(mgos_bvar_length(cv1) == 2);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Gregory") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 80);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("{}");
  ASSERT(v1 != NULL);
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 0);
  mgos_bvar_free(v1);
  
  v1 = mgos_bvar_json_scanf("{\"Name\":\"Mark\",\"Fhater\":{\"Name\":\"Gregory\",\"Age\":80, \"Mother\":{\"Name\":\"Mary\",\"Age\":79}},\"Age\":46}");
  ASSERT(mgos_bvar_is_dic(v1));
  ASSERT(mgos_bvar_length(v1) == 3);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(v1, "Name")), "Mark") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(v1, "Age")) == 46);
  cv1 = mgos_bvarc_get_key(v1, "Fhater");
  ASSERT(mgos_bvar_is_dic(cv1));
  ASSERT(mgos_bvar_length(cv1) == 3);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Gregory") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 80);
  cv1 = mgos_bvarc_get_key(cv1, "Mother");
  ASSERT(mgos_bvar_length(cv1) == 2);
  ASSERT(strcmp(mgos_bvar_get_str(mgos_bvarc_get_key(cv1, "Name")), "Mary") == 0);
  ASSERT(mgos_bvar_get_integer(mgos_bvarc_get_key(cv1, "Age")) == 79);
  mgos_bvar_free(v1);
  
  #endif // MGOS_BVAR_HAVE_JSON

  #endif // MGOS_BVAR_HAVE_DIC
  
  printf("Tests successfully completed!");
  
  return 0;
}
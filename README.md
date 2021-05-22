# bVariant Library
## Overview
This Mongoose OS library allows you to create variant bVariants which haven't data type declared explicitly (like using `var` statement in javascript). You can create variables that can easily and dynamically assume any data-type within the followings:

- Boolean (`bool`)
- Integer (`long`)
- Decimal (`double`)
- String (`char *`)
- Dictionary (key/value pair dictionary) - This requires you to include the [bVariant Dictionary library](https://github.com/diy365-mgos/bvar-dic) in your porject.
## Features
- **Observable value** - You can check it the value of a bVariant is changed or not.
- **JSON support** - You can dynamically create a variant varibale from a JSON string or you can save it as JSON in a very easy way. Just include the [bVariant JSON library](https://github.com/diy365-mgos/bvar-json) into your project. 
## Get Started in C/C++
### Example 1 - Create bVariants
Include the library into your `mos.yml` file.
```yaml
libs:
  - origin: https://github.com/diy365-mgos/bvar
```
```c
#include "mgos_bvar.h"

mgos_bvar_t n = mgos_bvar_new();                  // void *n = NULL; (type-less var)
mgos_bvar_t i = mgos_bvar_new_integer(101);       // long i = 101;
mgos_bvar_t b = mgos_bvar_new_bool(true);         // bool b = true;
mgos_bvar_t d = mgos_bvar_new_decimal(101.99);    // double d = 101.99;
mgos_bvar_t s = mgos_bvar_new_str("Lorem Ipsum"); // char *s = "Lorem Ipsum";
```
## C/C++ APIs Reference
### MGOS_BVAR_CONST
```c
#define MGOS_BVAR_CONST(v) ((mgos_bvarc_t)v)
```
Macro to cast a bVariant to *const bVariant*. A constant bVariant can't be modified, it's readonly.
### enum mgos_bvar_type
```c
enum mgos_bvar_type {
  MGOS_BVAR_TYPE_NULL,
  MGOS_BVAR_TYPE_BOOL,
  MGOS_BVAR_TYPE_INTEGER,
  MGOS_BVAR_TYPE_DECIMAL,
  MGOS_BVAR_TYPE_STR
};
```
bVariant data-types.
### mgos_bvar_get_type
```c
enum mgos_bvar_type mgos_bvar_get_type(mgos_bvarc_t var);
```
Returns the bVariant [data-type](#enum-mgos_bvar_type). Returns `MGOS_BVAR_TYPE_DIC` if it is a dictionary.

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_new
```c
mgos_bvar_t mgos_bvar_new();
```
Creates a type-less bVariant (with no data-type defined). Returns `NULL` if error. The returned instance must be deallocated using `mgos_bvar_free()`.
### mgos_bvar_new_integer | mgos_bvar_new_bool | mgos_bvar_new_decimal | mgos_bvar_new_str
```c       
mgos_bvar_t mgos_bvar_new_integer(long value);
mgos_bvar_t mgos_bvar_new_bool(bool value);
mgos_bvar_t mgos_bvar_new_decimal(double value);
mgos_bvar_t mgos_bvar_new_str(const char *value);
```
Creates and initializes a bVariant. Returns `NULL` if error. Invoking `mgos_bvar_new_str(NULL)` is equivalent to `mgos_bvar_new()`. The returned instance must be deallocated using `mgos_bvar_free`.

|Parameter||
|--|--|
|value|Value to be set.|
### mgos_bvar_set_null
```c 
void mgos_bvar_set_null(mgos_bvar_t var);
```
Sets a bVariant as type-less (with no data-type defined).

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_is_null
```c
bool mgos_bvar_is_null(mgos_bvarc_t var);
```
Returns `true` if the bVariant is type-less (with no data-type defined), or `false` otherwise.

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_set_integer | mgos_bvar_set_bool | mgos_bvar_set_decimal | mgos_bvar_set_str
```c                                 
void mgos_bvar_set_integer(mgos_bvar_t var, long value);
void mgos_bvar_set_bool(mgos_bvar_t var, bool value);
void mgos_bvar_set_decimal(mgos_bvar_t var, double value);
void mgos_bvar_set_str(mgos_bvar_t var, const char *value);
```
Sets the value of a bVariant. Invoking `mgos_bvar_set_str(var, NULL)` is equivalent to `mgos_bvar_set_null(var)`.

|Parameter||
|--|--|
|var|A bVariant.|
|value|Value to set.|
### mgos_bvar_set_nstr
```c 
void mgos_bvar_set_nstr(mgos_bvar_t var, const char *value, size_t value_len);
```
Sets a bVariant value to the provided string. This is a specialized version of `mgos_bvar_set_str()` Invoking `mgos_bvar_set_nstr(var, NULL, <any_value>)` is equivalent to `mgos_bvar_set_null(var)`.

|Parameter||
|--|--|
|var|A bVariant.|
|value|String value to set.|
|value_len|Maximum number of characters to set. Ignored if `value` parameter is `NULL`.|
### mgos_bvar_get_integer | mgos_bvar_get_bool | mgos_bvar_get_decimal | mgos_bvar_get_str
```c 
long mgos_bvar_get_integer(mgos_bvarc_t var);
bool mgos_bvar_get_bool(mgos_bvarc_t var);
double mgos_bvar_get_decimal(mgos_bvarc_t var);
const char *mgos_bvar_get_str(mgos_bvarc_t var);
```
Returns the value of a bVariant.

|Parameter||
|--|--|
|var|A bVariant.|

**Remarks**

The returned value depends on the input bVariant data-type. Please refer to details below.
|Function / data-type|INTEGER|BOOL|DECIMAL|STRING|Any other|
|--|--|--|--|--|--|
|**mgos_bvar_get_integer()**|Returns the integer value|Returns `0` if input value is `false`|Returns the integer part of the decimal|Returns `0`|Returns `0`|
|**mgos_bvar_get_bool()**|Returns `false` if input value is `0`|Returns the boolean value|Returns `false` if input value is `0.0`|Returns `false` if input string is empty|Returns `false`|
|**mgos_bvar_get_decimal()**|Returns the input value as decimal|Returns `0.0`|Returns the decimal value|Returns `0.0`|Returns `0.0`|
|**mgos_bvar_get_str()**|Returns `NULL`|Returns `NULL`|Returns `NULL`|Returns the string value|Returns `NULL`|
### mgos_bvar_cmp
```c
int mgos_bvar_cmp(mgos_bvarc_t var1, mgos_bvarc_t var2);
```
Compares two bVariants. Returns `INT_MAX` if error.

|Parameter||
|--|--|
|var1|A bVariant.|
|var2|A bVariant.| 

**Remarks**

Returns an integer value indicating the relationship between the compared bVariants:
|Return value||
|--|--|
|<0|The value of *var1* is minor than the value of *var2*. If one or both of the bVariants are dictionaries, they are not equal.|
|0|The two bVariants are equal. If both of them are dictionaries, they contain the same keys, regardless the order.|
|>0|The value of *var1* is minor than the value of *var2*.|
### mgos_bvar_copy
```c
bool mgos_bvar_copy(mgos_bvarc_t src_var, mgos_bvar_t dest_var); 
```
Copies a source bVariant into the destination one. Returns `true` on success, or `false` otherwise.

|Parameter||
|--|--|
|src_var|Source bVariant.|
|dest_var|Destination bVariant.|
### mgos_bvar_merge
```c
bool mgos_bvar_merge(mgos_bvarc_t src_var, mgos_bvar_t dest_var);
```
Merges a source bVariant into the destination one. Returns `true` on success, or `false` otherwise. If the source is not a dictionary it is just copied into the destination (see [mgos_bvar_copy()](#mgos_bvar_copy) above), otherwise source dictionary keys are added to the destination one. 

|Parameter||
|--|--|
|src_var|A source bVariant.|
|dest_var|A destination bVariant.|
### mgos_bvar_length
```c
int mgos_bvar_length(mgos_bvarc_t var); 
```
Returns the number of items in a dictionary or the string length. Returns `0` in all other cases.

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_set_unchanged
```c
void mgos_bvar_set_unchanged(mgos_bvar_t var);
```
Marks the bVariant as unchanged. This function could be used in combination with `mgos_bvar_is_changed()`.

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_is_changed
```c
bool mgos_bvar_is_changed(mgos_bvarc_t var);
```
Returns `true` if the bVariant is changed since its creation or since the last call of `mgos_bvar_set_unchanged()`, or `false` otherwise.

|Parameter||
|--|--|
|var|A bVariant.|
### mgos_bvar_free
```c
void mgos_bvar_free(mgos_bvar_t var);
```
Deallocates the bVariant. If it is an element of a dictionary, it is also removed from the collection. If it is a dictionary, all its items are deallocated as well.

|Parameter||
|--|--|
|var|A bVariant.|
## To Do
- Implement variant array.
- Implement javascript APIs for [Mongoose OS MJS](https://github.com/mongoose-os-libs/mjs).

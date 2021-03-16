#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "access/hash.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

typedef struct intSet
{
	/*
	   Fixed write style
	   The first element is the length of this set
	   From the second element it is the content of this intset
	*/
	int length;
	int intset[1];
}intSet;


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_in);
// Aim to use qsort
int cmpfunc (const void * a, const void * b);
int cmpfunc (const void * a, const void * b) {
	return ( *(int*)a - *(int*)b );
}

Datum
intset_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	intSet    *result;

	//check whether the input is right:using loop or regrex

	// Split character
	char *delim = "{, }";

	char *token;
	int *array,length = 2, element_count = 0, found = 0, element = 0;

	array = malloc(sizeof(int) * length);

	// Get the first char
	token = strtok(str, delim);

	// Continue to read other elements
	while( token != NULL ) {
		element = atoi(token);
		if(element_count == length){
			length = length * 2;
			// Enlarge the memory
			array = realloc(array, sizeof(int) * length);
		}

		// check whether existing the same elements
		found = 0;
		for(int i =0; i< element_count; i++){
			if(array[i] == element){
				found = 1;
				break;
			}
		}

		// If not, add new distinct element to the array
		if(found == 0){
			// Chnage string to int
			array[element_count] = atoi(token);
			element_count++;
		}

		token = strtok(NULL, delim);
	}

	// Sort the array and qsort is the internal method in C
	qsort(array, element_count, sizeof(int), cmpfunc);

	result= (intSet *) palloc(VARHDRSZ + (sizeof(int) * (element_count + 1)));
	// SET to enable it
	SET_VARSIZE(result, VARHDRSZ + (sizeof(int) * (element_count + 1)));

	result->intset[0] = element_count;
	for(int i = 0; i < element_count; i++){
		elog(NOTICE, "index: %d, value: %d", i, array[i]);
		// Mock the head method of record
		result->intset[i + 1] = array[i];
	}

	free(array);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
	intSet *paramSet = (intSet *) PG_GETARG_POINTER(0);
	// todo:change to dynamic
	char result[1024], strNum[32];
	result[0] = '\0';

	strcat(result, "{");
	// The first element is the length of set
	if (paramSet->intset[0] > 0){
		// Change the first element to number string and return strNum
		// This is the method of postegresql
		pg_itoa(paramSet->intset[1],strNum);
		// concat result and strNum
		strcat(result, strNum);
		for(int i = 1; i< paramSet->intset[0];i++){
			strcat(result, ",");
			pg_itoa(paramSet->intset[i + 1],strNum);
			strcat(result, strNum);
		}
	}

	strcat(result, "}");
	// Fixed gramma to print
	PG_RETURN_CSTRING(psprintf("%s", result));
}

/* Include */
PG_FUNCTION_INFO_V1(intset_include);

Datum
intset_include(PG_FUNCTION_ARGS)
{
	int *a = (int *) PG_GETARG_POINTER(0);
	intSet *b = (intSet *) PG_GETARG_POINTER(1);

	int result = 0;
	for (int i=0;i< b->intset[0];i++){
		if(a == b->intset[i+1]){
			result = 1;
			break;
		}
	}
	PG_RETURN_BOOL(result);
}

/* LeftInclude */
PG_FUNCTION_INFO_V1(intset_leftInclude);

Datum
intset_leftINclude(PG_FUNCTION_ARGS)
{
	intSet *a = (intSet *) PG_GETARG_POINTER(0);
	intSet *b = (intSet *) PG_GETARG_POINTER(1);

	int result = 1;
	int count = 0;
	if(a->intset[0] < b->intset[0]) {
		result = 0;
	} else {
		for (int i=0;i< b->intset[0];i++){
			for (int j=0;j< a->intset[0];j++){
				if(b->intset[i+1] == a->intset[j+1]){
					count++;
					break;
				}
			}
		}
	}
	if(count != b->intset[0]){
		result = 0;
	}

	PG_RETURN_BOOL(result);
}

/* Equal */
PG_FUNCTION_INFO_V1(intset_eq);

Datum
intset_eq(PG_FUNCTION_ARGS)
{
	intSet *a = (intSet *) PG_GETARG_POINTER(0);
	intSet *b = (intSet *) PG_GETARG_POINTER(1);

	int result = 1;
	if(a->intset[0] != b->intset[0]) {
		result = 0;
	} else {
		for (int i=0;i< a->intset[0];i++){
			if(a->intset[i+1] != b->intset[i+1]){
				result = 0;
				break;
			}
		}
	}

	PG_RETURN_BOOL(result);
}


/* Cardinality */
PG_FUNCTION_INFO_V1(intset_cardinality);

Datum
intset_cardinality(PG_FUNCTION_ARGS)
{
	intSet *a = (intSet *) PG_GETARG_POINTER(0);
	PG_RETURN_INT32(a->intset[0]);
}


/* Difference */
PG_FUNCTION_INFO_V1(intset_difference);

Datum
intset_difference(PG_FUNCTION_ARGS)
{
	intSet *a = (intSet *) PG_GETARG_POINTER(0);
	intSet *b = (intSet *) PG_GETARG_POINTER(1);
	intSet *result;

	int *array,length = 2, element_count = 0, found = 0;

	array = malloc(sizeof(int) * length);

	for(int i=0;i < a->intset[0];i++){
		found = 0;
		for(int j=0;j<b->intset[0];j++){
			if(a->intset[i+1] == b->intset[j+1]){
				found=1;
				break;
			}
		}
		// element is in a also in b
		if(found==1){
			continue;
		}else{
			// In a but not in b
			if(element_count == length){
				length = length * 2;
				// 扩容内存
				array = realloc(array, sizeof(int) * length);
			}
			array[element_count] = a->intset[i+1];
			element_count++;
		}

	}

	result= (intSet *) palloc(VARHDRSZ + (sizeof(int) * (element_count + 1)));
	// 设置一下
	SET_VARSIZE(result, VARHDRSZ + (sizeof(int) * (element_count + 1)));

	result->intset[0] = element_count;
	for(int i = 0; i < element_count; i++){
		elog(NOTICE, "index: %d, value: %d", i, array[i]);
		// 模拟record的header方式
		result->intset[i + 1] = array[i];
	}

	free(array);
	PG_RETURN_POINTER(result);

}

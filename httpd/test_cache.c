/*
 * test_cache.cc
 *
 * Here's a tester for the server cache. It eats and throws up as commanded.
 */

#ifdef CACHE_TEST

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "CUnit/Basic.h"
#include "cache.h"
#include "palloc.h"

static palloc_env env = NULL;

static int init_suite(void)
{
        env = palloc_init("cache test env");
        cache_init(env);
        return 0;
}

static int clean_suite(void)
{
        if (env != NULL) {
                pfree(env);
                env = NULL;
        }
        return 0;
}

// Check if cache_add, cache_get, and cache_remove correctly works together.
static void test_cache_simple_1(void)
{
        int status;
        char *fetched_response;
        char *input_request = "random request 0001";
        char *input_response = "random response 0001";

        CU_ASSERT_FATAL(CACHE_SIZE >= 1);
        CU_ASSERT_PTR_NULL(cache_get(input_request));

        status = cache_add(input_request, input_response);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response);

        status = cache_remove(input_request);
        CU_ASSERT_FATAL(status == 0);

        CU_ASSERT_PTR_NULL(cache_get(input_request));
}

// Check if cache_get is deterministic and does not modify entries.
static void test_cache_simple_2(void)
{
        int status;
        char *fetched_response;
        char *input_request_1 = "random request 0002";
        char *input_response_1 = "random response 0002";
        char *input_request_2 = "random request 0003";
        char *input_response_2 = "random response 0003";
        char *input_request_3 = "random request 0004";
        char *input_response_3 = "random response 0004";

        CU_ASSERT_FATAL(CACHE_SIZE >= 3);
        CU_ASSERT_PTR_NULL(cache_get(input_request_1));
        CU_ASSERT_PTR_NULL(cache_get(input_request_2));
        CU_ASSERT_PTR_NULL(cache_get(input_request_3));

        status = cache_add(input_request_1, input_response_1);
        CU_ASSERT_FATAL(status == 0);
        status = cache_add(input_request_2, input_response_2);
        CU_ASSERT_FATAL(status == 0);
        status = cache_add(input_request_3, input_response_3);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request_1);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_1);
        fetched_response = cache_get(input_request_2);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_2);
        fetched_response = cache_get(input_request_3);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_3);

        fetched_response = cache_get(input_request_2);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_2);
        fetched_response = cache_get(input_request_1);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_1);
        fetched_response = cache_get(input_request_3);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_3);
}

// Check if cache_add overwrites for same request.
static void test_cache_simple_3(void)
{
        int status;
        char *fetched_response;
        char *input_request = "random request 0005";
        char *input_response_1 = "random response 0005";
        char *input_response_2 = "random response 0006";

        CU_ASSERT_FATAL(CACHE_SIZE >= 1);
        CU_ASSERT_PTR_NULL(cache_get(input_request));

        status = cache_add(input_request, input_response_1);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_1);

        status = cache_add(input_request, input_response_2);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_2);

        status = cache_remove(input_request);
        CU_ASSERT_FATAL(status == 0);

        CU_ASSERT_PTR_NULL(cache_get(input_request));
}

// Check if cache_remove removes only intended entry.
static void test_cache_simple_4(void)
{
        int status;
        char *fetched_response;
        char *input_request_1 = "random request 0007";
        char *input_response_1 = "random response 0007";
        char *input_request_2 = "random request 0008";
        char *input_response_2 = "random response 0008";
        char *input_request_3 = "random request 0009";
        char *input_response_3 = "random response 0009";

        CU_ASSERT_FATAL(CACHE_SIZE >= 3);
        CU_ASSERT_PTR_NULL(cache_get(input_request_1));
        CU_ASSERT_PTR_NULL(cache_get(input_request_2));
        CU_ASSERT_PTR_NULL(cache_get(input_request_3));

        status = cache_add(input_request_1, input_response_1);
        CU_ASSERT_FATAL(status == 0);
        status = cache_add(input_request_2, input_response_2);
        CU_ASSERT_FATAL(status == 0);
        status = cache_add(input_request_3, input_response_3);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request_1);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_1);
        fetched_response = cache_get(input_request_2);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_2);
        fetched_response = cache_get(input_request_3);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_3);

        status = cache_remove(input_request_2);
        CU_ASSERT_FATAL(status == 0);
        CU_ASSERT_PTR_NULL(cache_get(input_request_2));

        fetched_response = cache_get(input_request_1);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_1);
        fetched_response = cache_get(input_request_3);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response_3);
}

// Check if adding many entries will push out the earlier ones.
static void test_cache_simple_5(void)
{
        int status;
        int i;
        char *fetched_response;
        char input_request[21];
        char input_response[22];
        int j;
        for (j = 0; j < 20; j++) {
                if ((j % 2) == 0) {
                        input_request[j] = 'o';
                        input_response[j] = 'q';
                } else {
                        input_request[j] = 'e';
                        input_response[j] = 'h';
                }
        }
        input_response[20] = 'h';
        input_request[20] = 0;
        input_response[21] = 0;
        int request_length = strlen(input_request);
        int response_length = strlen(input_response);

        CU_ASSERT_FATAL(CACHE_SIZE >= 1);
        CU_ASSERT_PTR_NULL(cache_get(input_request));

        // Add enough entries to cause at least 1 replacement in cache.
        for (i = 1; i <= CACHE_SIZE+1; i++)
        {
                *(input_request + request_length - 5) = (char)('0' + ((char)(i/1000)));
                *(input_request + request_length - 4) = (char)('0' + ((char)((i % 1000)/100)));
                *(input_request + request_length - 3) = (char)('0' + ((char)((i % 100)/10)));
                *(input_request + request_length - 2) = (char)('0' + ((char)((i % 10)/1)));
                *(input_request + request_length - 1) = (char)('L');
                *(input_response + response_length - 5) = (char)('0' + ((char)(i/1000)));
                *(input_response + response_length - 4) = (char)('0' + ((char)((i % 1000)/100)));
                *(input_response + response_length - 3) = (char)('0' + ((char)((i % 100)/10)));
                *(input_response + response_length - 2) = (char)('0' + ((char)((i % 10)/1)));
                *(input_response + response_length - 1) = (char)('L');

                status = cache_add(input_request, input_response);
                CU_ASSERT_FATAL(status == 0);
                fetched_response = cache_get(input_request);
                CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
                CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response);
        }

        // Find possible conflict in cache from replacement.
        for (i = CACHE_SIZE+1; i >= 1; i--)
        {
                *(input_request + request_length - 5) = (char)('0' + ((char)(i/1000)));
                *(input_request + request_length - 4) = (char)('0' + ((char)((i % 1000)/100)));
                *(input_request + request_length - 3) = (char)('0' + ((char)((i % 100)/10)));
                *(input_request + request_length - 2) = (char)('0' + ((char)((i % 10)/1)));
                *(input_request + request_length - 1) = (char)('L');
                *(input_response + response_length - 5) = (char)('0' + ((char)(i/1000)));
                *(input_response + response_length - 4) = (char)('0' + ((char)((i % 1000)/100)));
                *(input_response + response_length - 3) = (char)('0' + ((char)((i % 100)/10)));
                *(input_response + response_length - 2) = (char)('0' + ((char)((i % 10)/1)));
                *(input_response + response_length - 1) = (char)('L');

                fetched_response = cache_get(input_request);
                if (fetched_response == NULL)
                        break;
        }

        // Check if not last item updated (which should be in cache).
        CU_ASSERT_FATAL(i <= CACHE_SIZE);
        // Check if actually found conflict.
        CU_ASSERT_FATAL(i > 0);
}

// Check if methods respond correctly to invalid input.
static void test_cache_simple_6(void)
{
        int status;
        char *fetched_response;
        char *input_request = "random request 00016";
        char *input_response = "random response 00016";

        CU_ASSERT_FATAL(CACHE_SIZE >= 1);
        CU_ASSERT_PTR_NULL(cache_get(input_request));

        // Invalid cache_add input.
        status = cache_add(NULL, input_response);
        CU_ASSERT_FATAL(status != 0);
        CU_ASSERT_PTR_NULL(cache_get(input_request));
        status = cache_add(input_request, NULL);
        CU_ASSERT_FATAL(status != 0);
        CU_ASSERT_PTR_NULL(cache_get(input_request));
        status = cache_add(input_request, input_response);
        CU_ASSERT_FATAL(status == 0);

        // Invalid cache_get input.
        CU_ASSERT_PTR_NULL(cache_get(NULL));
        CU_ASSERT_PTR_NULL(cache_get("wrong request"));
        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response);

        // Invalid cache_remove input.
        status = cache_remove(NULL);
        CU_ASSERT_FATAL(status != 0);
        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response);
        CU_ASSERT_PTR_NULL(cache_get(NULL));

        status = cache_remove("wrong request");
        CU_ASSERT_FATAL(status != 0);
        fetched_response = cache_get(input_request);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL(fetched_response, input_response);
        CU_ASSERT_PTR_NULL(cache_get("wrong request"));

        status = cache_remove(input_request);
        CU_ASSERT_FATAL(status == 0);
        CU_ASSERT_PTR_NULL(cache_get(input_request));
}

// Check if cache status is indepedent of calls to palloc and pfree.
static void test_cache_simple_7(void)
{
        int status;
        char *fetched_response;
        char *input_request_stack = "random request 00017";
        char *input_response_stack = "random response 00017";
        char *input_request_heap = palloc_strdup(env, input_request_stack);
        char *input_response_heap = palloc_strdup(env, input_response_stack);

        CU_ASSERT_PTR_NOT_NULL_FATAL(input_request_heap);
        CU_ASSERT_PTR_NOT_NULL_FATAL(input_response_heap);
        CU_ASSERT_FATAL(CACHE_SIZE >= 1);
        CU_ASSERT_PTR_NULL(cache_get(input_request_heap));

        status = cache_add(input_request_heap, input_response_heap);
        CU_ASSERT_FATAL(status == 0);

        fetched_response = cache_get(input_request_stack);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_stack);

        pfree(input_request_heap);

        fetched_response = cache_get(input_request_stack);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_stack);

        pfree(input_response_heap);

        fetched_response = cache_get(input_request_stack);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_stack);

        pfree(fetched_response);

        fetched_response = cache_get(input_request_stack);
        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_stack);

        status = cache_remove(input_request_stack);
        CU_ASSERT_FATAL(status == 0);

        CU_ASSERT_PTR_NOT_NULL_FATAL(fetched_response);
        CU_ASSERT_STRING_EQUAL_FATAL(fetched_response, input_response_stack);

        pfree(fetched_response);
}

static void test_cache_fuzz_1(void)
{

}

static void test_cache_fuzz_2(void)
{

}

static void test_cache_fuzz_3(void)
{

}

int main(int argc, char** argv)
{
        if (CUE_SUCCESS != CU_initialize_registry())
                return CU_get_error();

        CU_pSuite pSuite = CU_add_suite("cache tests", init_suite, clean_suite);
        if (NULL == pSuite) {
                CU_cleanup_registry();
                return CU_get_error();
        }

        if (NULL == CU_add_test(pSuite, "Simple Test #1", test_cache_simple_1)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #2", test_cache_simple_2)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #3", test_cache_simple_3)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #4", test_cache_simple_4)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #5", test_cache_simple_5)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #6", test_cache_simple_6)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Simple Test #7", test_cache_simple_7)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Fuzz Test #1", test_cache_fuzz_1)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Fuzz Test #2", test_cache_fuzz_2)) {
                CU_cleanup_registry();
                return CU_get_error();
        }
        if (NULL == CU_add_test(pSuite, "Fuzz Test #3", test_cache_fuzz_3)) {
                CU_cleanup_registry();
                return CU_get_error();
        }

        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        CU_cleanup_registry();
        return CU_get_error();
}

#endif /* CACHE_TEST */

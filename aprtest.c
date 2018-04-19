//
// Created by Stefan Schwarz on 28.12.17.
//

#include <stdlib.h>
#include <apr-1/apr.h>
#include <apr-1/apr_pools.h>
#include <apr-1/apr_file_io.h>

int main()
{
    apr_pool_t *pool;
    apr_file_t *out;

    apr_initialize();
//    atexit(apr_terminate());

    apr_pool_create(&pool, NULL);
    apr_file_open_stdout(&out, pool);
    apr_file_printf(out, "Hello World\n");
    apr_pool_destroy(pool);

    return EXIT_SUCCESS;
}
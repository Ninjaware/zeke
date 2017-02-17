/**
 * @file test_atomic.c
 * @brief Test atomic.
 */

#include <kunit.h>
#include <libkern.h>
#include <machine/atomic.h>

static atomic_t avar;

static void setup(void)
{
    avar = ATOMIC_INIT(5);
}

static void teardown(void)
{
}

static char * test_atomic_read(void)
{
    int val = 0;

    ku_test_description("Test that atomic_read() works.");

    val = atomic_read(&avar);
    ku_assert_equal("avar is read correctly", val, 5);

    return NULL;
}

static char * test_atomic_set(void)
{
    ku_test_description("Test that atomic_set() works.");

    ku_assert_equal("Old value of avar is returned on set",
                    atomic_set(&avar, -2), 5);
    ku_assert_equal("New value was set correctly", atomic_read(&avar), -2);

    return NULL;
}

static char * test_atomic_add(void)
{
    ku_test_description("Test that atomic_add() works.");

    ku_assert_equal("Old value of avar is returned on set",
                    atomic_add(&avar, 1), 5);
    ku_assert_equal("New value was set correctly", atomic_read(&avar), 6);

    return NULL;
}

static char * test_atomic_sub(void)
{
    ku_test_description("Test that atomic_add() works.");

    ku_assert_equal("Old value of avar is returned on write",
                    atomic_sub(&avar, 1), 5);
    ku_assert_equal("New value was set correctly", atomic_read(&avar), 4);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_atomic_read, KU_RUN);
    ku_def_test(test_atomic_set, KU_RUN);
    ku_def_test(test_atomic_add, KU_RUN);
    ku_def_test(test_atomic_sub, KU_RUN);
}

TEST_MODULE(hal, atomic);

/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/param.h>
#include <sys/types.h>

#define _used_ __attribute__((__used__))
#define _unused_ __attribute__((__unused__))
#define _public_ __attribute__((__visibility__("default")))
#define _hidden_ __attribute__((__visibility__("hidden")))
/* Define C11 noreturn without <stdnoreturn.h> and even on older gcc
 * compiler versions */
#ifndef _noreturn_
#        if __STDC_VERSION__ >= 201112L
#                define _noreturn_ _Noreturn
#        else
#                define _noreturn_ __attribute__((__noreturn__))
#        endif
#endif


#ifndef __COVERITY__
#        define VOID_0 ((void) 0)
#else
#        define VOID_0 ((void *) 0)
#endif

#define ELEMENTSOF(x)           \
        (__builtin_choose_expr( \
                !__builtin_types_compatible_p(typeof(x), typeof(&*(x))), sizeof(x) / sizeof((x)[0]), VOID_0))

/*
 * STRLEN - return the length of a string literal, minus the trailing NUL byte.
 *          Contrary to strlen(), this is a constant expression.
 * @x: a string literal.
 */
#define STRLEN(x) (sizeof("" x "") - 1)

#define CASE_F(X) case X:
#define CASE_F_1(CASE, X) CASE_F(X)
#define CASE_F_2(CASE, X, ...) CASE(X) CASE_F_1(CASE, __VA_ARGS__)
#define CASE_F_3(CASE, X, ...) CASE(X) CASE_F_2(CASE, __VA_ARGS__)
#define CASE_F_4(CASE, X, ...) CASE(X) CASE_F_3(CASE, __VA_ARGS__)
#define CASE_F_5(CASE, X, ...) CASE(X) CASE_F_4(CASE, __VA_ARGS__)
#define CASE_F_6(CASE, X, ...) CASE(X) CASE_F_5(CASE, __VA_ARGS__)
#define CASE_F_7(CASE, X, ...) CASE(X) CASE_F_6(CASE, __VA_ARGS__)
#define CASE_F_8(CASE, X, ...) CASE(X) CASE_F_7(CASE, __VA_ARGS__)
#define CASE_F_9(CASE, X, ...) CASE(X) CASE_F_8(CASE, __VA_ARGS__)
#define CASE_F_10(CASE, X, ...) CASE(X) CASE_F_9(CASE, __VA_ARGS__)
#define CASE_F_11(CASE, X, ...) CASE(X) CASE_F_10(CASE, __VA_ARGS__)
#define CASE_F_12(CASE, X, ...) CASE(X) CASE_F_11(CASE, __VA_ARGS__)
#define CASE_F_13(CASE, X, ...) CASE(X) CASE_F_12(CASE, __VA_ARGS__)
#define CASE_F_14(CASE, X, ...) CASE(X) CASE_F_13(CASE, __VA_ARGS__)
#define CASE_F_15(CASE, X, ...) CASE(X) CASE_F_14(CASE, __VA_ARGS__)
#define CASE_F_16(CASE, X, ...) CASE(X) CASE_F_15(CASE, __VA_ARGS__)
#define CASE_F_17(CASE, X, ...) CASE(X) CASE_F_16(CASE, __VA_ARGS__)
#define CASE_F_18(CASE, X, ...) CASE(X) CASE_F_17(CASE, __VA_ARGS__)
#define CASE_F_19(CASE, X, ...) CASE(X) CASE_F_18(CASE, __VA_ARGS__)
#define CASE_F_20(CASE, X, ...) CASE(X) CASE_F_19(CASE, __VA_ARGS__)

#define GET_CASE_F(                                                                                           \
        _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, NAME, ...) \
        NAME
#define FOR_EACH_MAKE_CASE(...) \
        GET_CASE_F(             \
                __VA_ARGS__,    \
                CASE_F_20,      \
                CASE_F_19,      \
                CASE_F_18,      \
                CASE_F_17,      \
                CASE_F_16,      \
                CASE_F_15,      \
                CASE_F_14,      \
                CASE_F_13,      \
                CASE_F_12,      \
                CASE_F_11,      \
                CASE_F_10,      \
                CASE_F_9,       \
                CASE_F_8,       \
                CASE_F_7,       \
                CASE_F_6,       \
                CASE_F_5,       \
                CASE_F_4,       \
                CASE_F_3,       \
                CASE_F_2,       \
                CASE_F_1)       \
        (CASE_F, __VA_ARGS__)

#define IN_SET(x, ...)                                                                                      \
        ({                                                                                                  \
                bool _found = false;                                                                        \
                /* If the build breaks in the line below, you need to extend the case macros. (We use "long \
                 * double" as type for the array, in the hope that checkers such as ubsan don't complain    \
                 * that the initializers for the array are not representable by the base type. Ideally we'd \
                 * use typeof(x) as base type, but that doesn't work, as we want to use this on bitfields   \
                 * and gcc refuses typeof() on bitfields.) */                                               \
                static const long double __assert_in_set[] _unused_ = { __VA_ARGS__ };                      \
                assert_cc(ELEMENTSOF(__assert_in_set) <= 20);                                               \
                switch (x) {                                                                                \
                        FOR_EACH_MAKE_CASE(__VA_ARGS__)                                                     \
                        _found = true;                                                                      \
                        break;                                                                              \
                default:                                                                                    \
                        break;                                                                              \
                }                                                                                           \
                _found;                                                                                     \
        })

#pragma once
#include <print>
using namespace std;

#define TERM_RED "\033[1;31m"
#define TERM_BLUE "\033[1;36m"
#define TERM_YELLOW "\033[1;33m"
#define TERM_DEFAULT "\033[1;0m"

#define INF(x, ...) \
    println(stdout, TERM_BLUE "[INF]: " \
                 TERM_DEFAULT "\n" x \
     __VA_OPT__(,) __VA_ARGS__)

#define THROW(err_val, x, ...) do {ERR(x, __VA_ARGS__); return err_val;} while(0)

#define ERR(x, ...) \
    println(stderr, TERM_RED "[ERR: " __FILE__ " at {}]: " \
                 TERM_DEFAULT "\n" x "\n", __LINE__\
     __VA_OPT__(,) __VA_ARGS__)
#define TRY(expr) do {u8 __ret_val = expr; if (__ret_val) {\
    println(stderr,\
                 TERM_YELLOW "\t↪ " __FILE__ " at %u\n" \
                 TERM_DEFAULT, __LINE__); return __ret_val;} } while(0)


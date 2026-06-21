#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <dmsdk/sdk.h>

/* SplitMix64 */
/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state. */

static uint64_t splitmix64_next(uint64_t *x) {
	uint64_t z = (*x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

/* Xoshiro256++ */
/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

typedef struct {
    uint64_t s[4];
} Xoshiro256State;

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

uint64_t xoshiro256pp_next(Xoshiro256State *st) {
	const uint64_t result = rotl(st->s[0] + st->s[3], 23) + st->s[0];

	const uint64_t t = st->s[1] << 17;

	st->s[2] ^= st->s[0];
	st->s[3] ^= st->s[1];
	st->s[1] ^= st->s[2];
	st->s[0] ^= st->s[3];

	st->s[2] ^= t;

	st->s[3] = rotl(st->s[3], 45);

	return result;
}

/* defold-xoshiro256 */
/*
Copyright 2026 Glorius Silver

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Except as contained in this notice, the name(s) of the above copyright holders
shall not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define LIB_NAME   "xoshiro256"
#define MODULE_NAME "xoshiro256"


// Seed Xoshiro256State from a single 64-bit value using splitmix64.
static void xoshiro256_seed(Xoshiro256State *st, uint64_t seed) {
    st->s[0] = splitmix64_next(&seed);
    st->s[1] = splitmix64_next(&seed);
    st->s[2] = splitmix64_next(&seed);
    st->s[3] = splitmix64_next(&seed);
}

#define TO_DOUBLE (1.0 / 9007199254740992.0) // 1.0 / 2^53

static inline double xoshiro256pp_next_double(Xoshiro256State *st) {
    // Take the upper 53 bits of the 64-bit output.
    return (double)(xoshiro256pp_next(st) >> 11) * TO_DOUBLE;
}

#define XOSHIRO256_METATABLE "xoshiro256.state"

static Xoshiro256State *check_state(lua_State *L, int arg_idx) {
    return (Xoshiro256State *)luaL_checkudata(L, arg_idx, XOSHIRO256_METATABLE);
}

static uint64_t parse_seed(lua_State *L, int arg_idx) {
    int t = lua_type(L, arg_idx);
    if (t == LUA_TSTRING) {
        const char *str = lua_tostring(L, arg_idx);
        char *end = NULL;
        unsigned long long val = strtoull(str, &end, 10);
        if (end == str || *end != '\0')
            return luaL_error(L, "invalid seed string (expected decimal integer)");
        return (uint64_t)val;
    } else if (t == LUA_TNUMBER) {
        double arg = (double)lua_tonumber(L, arg_idx);
        return (uint64_t)floor(arg);
    }
    return luaL_argerror(L, arg_idx, "expected number or string");
}

// xoshiro256.seed(seed) -> state
static int L_seed_state(lua_State *L) {
    DM_LUA_STACK_CHECK(L, 1);

    uint64_t seed = parse_seed(L, 1);

    Xoshiro256State *st = (Xoshiro256State *)lua_newuserdata(L, sizeof(Xoshiro256State));
    xoshiro256_seed(st, seed);

    luaL_getmetatable(L, XOSHIRO256_METATABLE);
    lua_setmetatable(L, -2);

    return 1;
}

//xoshiro256.clone(state) -> state
static int L_clone_state(lua_State *L) {
    DM_LUA_STACK_CHECK(L, 1);

    Xoshiro256State *old_st = check_state(L, 1);
    Xoshiro256State *new_st = (Xoshiro256State *)lua_newuserdata(L, sizeof(Xoshiro256State));

    new_st->s[0] = old_st->s[0];
    new_st->s[1] = old_st->s[1];
    new_st->s[2] = old_st->s[2];
    new_st->s[3] = old_st->s[3];

    luaL_getmetatable(L, XOSHIRO256_METATABLE);
    lua_setmetatable(L, -2);

    return 1;
}

// xoshiro256.to_string(state) -> string
// 4 uint64 values separated by ':', e.g. "s0:s1:s2:s3"
static int L_state_to_string(lua_State *L) {
    DM_LUA_STACK_CHECK(L, 1);

    Xoshiro256State *st = check_state(L, 1);

    char buf[88]; // 4 * 20 digits + 3 * ':' + '\0' (84 bytes padded to 88 for 8-byte alignment)
    snprintf(buf, sizeof(buf),
             "%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64,
             st->s[0], st->s[1], st->s[2], st->s[3]);

    lua_pushstring(L, buf);
    return 1;
}

// xoshiro256.from_string(str) -> state
static int L_string_to_state(lua_State *L) {
    DM_LUA_STACK_CHECK(L, 1);

    const char *str = luaL_checkstring(L, 1);

    uint64_t v[4];
    char buf[88];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *tok = buf;
    for (int i = 0; i < 4; i++) {
        char *sep = (i < 3) ? strchr(tok, ':') : NULL;
        if (i < 3 && sep == NULL)
            return luaL_error(L, "malformed state string");

        if (sep)
            *sep = '\0';

        char *end = NULL;
        unsigned long long val = strtoull(tok, &end, 10);
        if (end == tok || (*end != '\0'))
            return luaL_error(L, "malformed state string");

        v[i] = (uint64_t)val;
        tok = (sep ? sep + 1 : tok);
    }

    Xoshiro256State *st = (Xoshiro256State *)lua_newuserdata(L, sizeof(Xoshiro256State));
    st->s[0] = v[0];
    st->s[1] = v[1];
    st->s[2] = v[2];
    st->s[3] = v[3];

    luaL_getmetatable(L, XOSHIRO256_METATABLE);
    lua_setmetatable(L, -2);

    return 1;
}

static uint64_t bounded_rand(Xoshiro256State *st, uint64_t n) {
    uint64_t r = xoshiro256pp_next(st);
    return r % n;
}

// xoshiro256.random(state [, m [, n]]) -> number
// Mirrors the behaviour of Lua's math.random
static int L_random(lua_State *L) {
    DM_LUA_STACK_CHECK(L, 1);

    Xoshiro256State *st = check_state(L, 1);

    int nargs = lua_gettop(L) - 1;

    lua_Integer low, up;
    switch (nargs) {
        case 0: {
            double r = xoshiro256pp_next_double(st);
            lua_pushnumber(L, (lua_Number)r);
            return 1;
        }
        case 1:
            low = 1;
            up  = luaL_checkinteger(L, 2);
            break;
        case 2:
            low = luaL_checkinteger(L, 2);
            up  = luaL_checkinteger(L, 3);
            break;
        default:
            return luaL_error(L, "wrong number of arguments");
    }

    luaL_argcheck(L, low <= up, 2, "interval is empty");

#ifdef LUA_MAXINTEGER
    luaL_argcheck(L, low >= 0 || up <= LUA_MAXINTEGER + low, 2, "interval too large");
#else
    luaL_argcheck(L, low >= 0 || up <= (lua_Integer)INT64_MAX + low, 2, "interval too large");
#endif

    uint64_t range = (uint64_t)(up - low) + 1;
    uint64_t i     = bounded_rand(st, range);
    lua_pushinteger(L, (lua_Integer)i + low);
    return 1;
}

// __tostring metamethod for the state userdata
static int L_state_tostring_meta(lua_State *L) {
    Xoshiro256State *st = check_state(L, 1);
    char buf[120];
    snprintf(buf, sizeof(buf),
             "xoshiro256.state(%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ")",
             st->s[0], st->s[1], st->s[2], st->s[3]);
    lua_pushstring(L, buf);
    return 1;
}

static const luaL_reg Module_methods[] = {
    { "seed",        L_seed_state      },
    { "clone",       L_clone_state     },
    { "to_string",   L_state_to_string },
    { "from_string", L_string_to_state },
    { "random",      L_random          },
    { NULL,          NULL              }
};

static void LuaInit(lua_State *L) {
    int top = lua_gettop(L);

    luaL_newmetatable(L, XOSHIRO256_METATABLE);
    lua_pushcfunction(L, L_state_tostring_meta);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    luaL_register(L, MODULE_NAME, Module_methods);
    lua_pop(L, 1);

    assert(top == lua_gettop(L));
}

static dmExtension::Result Initialize(dmExtension::Params *params) {
    LuaInit(params->m_L);
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(xoshiro256, LIB_NAME, 0, 0, Initialize, 0, 0, 0)

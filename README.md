# defold-xoshiro256: Defold Native Extension

A fast, high-quality PRNG for [Defold](https://defold.com) based on the
**xoshiro256++** algorithm, seeded via **splitmix64**.  
xoshiro256++ has a **256-bit state**, passes all known statistical tests, and produces no systematic bias in its floating-point output.  
More info about xoshiro256++ and splitmix64 can be found [here](https://prng.di.unimi.it/).

---

## Installation

Add to your `game.project` dependencies:

```
https://github.com/<your-org>/defold-xoshiro256/archive/main.zip
```

---

## API

### `xoshiro256.seed(seed: string|number): userdata state`

Create a new independent RNG state from a seed.

* `seed` - `number|string`
  Use a decimal string (e.g. `"01234567890123456789"`, max 20 digits) when you need full 64-bit
  precision.  
  Lua numbers are IEEE-754 doubles and cannot represent every `uint64` perfectly.

The seed is expanded to four 64-bit words using four consecutive `splitmix64`
steps, ensuring that the state is never all-zero.  
The returned state object is mutable and updated in-place by `xoshiro256.random()`.

```lua
local state = xoshiro256.seed(os.time())
local state64 = xoshiro256.seed("01234567890123456789")
```

---

### `xoshiro256.random(state: userdata [, m: number [, n: number]]): number`

Generate the next pseudo-random value, advancing `state` in-place.

| Call form | Return value |
|-----------|-------------|
| `random(state)` | Float in **[0, 1)** |
| `random(state, m)` | Integer in **[1, m]** |
| `random(state, m, n)` | Integer in **[m, n]** |

Mirrors the behavior of Lua's built-in `math.random`.

```lua
local state = xoshiro256.seed(42)

print(xoshiro256.random(state))         -- e.g. 0.71341...
print(xoshiro256.random(state, 6))      -- integer 1–6 (dice roll)
print(xoshiro256.random(state, 10, 20)) -- integer 10–20
```

---

### `xoshiro256.to_string(state: userdata): string`

Serialize the state to a portable string `"s0:s1:s2:s3"` (four
colon-separated decimal `uint64` values). Use this for save files or
deterministic replay systems.

```lua
local saved_state = xoshiro256.to_string(state)
-- e.g. "17325485563827056386:3928491082947823749:..."
```

---

### `xoshiro256.from_string(saved_state: string): userdata state`

Deserialize a previously saved state string back into a usable state object.

```lua
local restored_state = xoshiro256.from_string(saved_state)
-- restored_state will continue the exact same random sequence
```

---

## How to replace built-in math.randomseed and math.random with xoshiro256

```lua
__GLOBAL_XOSHIRO_STATE = __GLOBAL_XOSHIRO_STATE or xoshiro256.seed(os.time())

math.randomseed = function(seed)
    __GLOBAL_XOSHIRO_STATE = xoshiro256.seed(seed)
end

math.random = function(m, n)
    return xoshiro256.random(__GLOBAL_XOSHIRO_STATE, m, n)
end
```

## License

The xoshiro256++ and splitmix64 algorithms are dedicated to the public domain
by their authors (David Blackman, Sebastiano Vigna).  
The Defold wrapper code in this repository is released under the **MIT License**.

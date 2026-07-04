---@meta

---@alias xoshiro256.state userdata

---@class xoshiro256
xoshiro256 = {}

---Create state for the pseudo-random generator: equal seeds produce equal sequences of numbers.
---Accepts a number or a string if full 64-bit precision required
---(decimal representation of uint64, max 20 digits in the string).
---State is seeded using splitmix64 algorithm.
---@param seed number | string
---@return xoshiro256.state
function xoshiro256.seed(seed) end

---Creates and returns an independent copy of the state
---@param state xoshiro256.state
---@param out xoshiro256.state? optional state to clone into without memory allocations
---@return xoshiro256.state cloned_state
function xoshiro256.clone(state, out) end

---Convert state to string. Used for saving the state. String is formed as 4 uint64 values separated by ':', e.g. "s0:s1:s2:s3".
---@param state xoshiro256.state
---@return string
function xoshiro256.to_string(state) end

---Convert previously saved string to state.
---@param state_str string
---@return xoshiro256.state
function xoshiro256.from_string(state_str) end

---When called without arguments, returns a uniform pseudo-random real number in the range [0,1).
---When called with an integer number m, returns a uniform pseudo-random integer in the range [1, m].
---When called with two integer numbers m and n, returns a uniform pseudo-random integer in the range [m, n].
---@param state xoshiro256.state
---@param m number
---@param n number
---@return number
---@overload fun(state: xoshiro256.state, m: number): number
---@overload fun(state: xoshiro256.state): number
function xoshiro256.random(state, m, n) end

---Generates a random number in floating range [m, n)
---@param state xoshiro256.state
---@param m number
---@param n number
---@return number
function xoshiro256.float_range(state, m, n) end

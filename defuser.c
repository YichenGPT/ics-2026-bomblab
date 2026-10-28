#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t middle = (uint32_t)(challenge >> 16);
    uint32_t first = (5u * low) ^ (high + 0xe973u);
    uint32_t second = (3u * high + middle) ^ 0x618cu;
    uint32_t values[] = {(3u * (second - low) + 0xbf4cu) ^ first};
    return write_answer(out, cap, values, 1);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (((uint32_t)(challenge >> 2) * 2u) & 255u) + 30u;
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t part = (uint32_t)(challenge >> (25u + 2u * (i - 1u))) & 15u;
        values[i] = 2u * values[i - 1] + (part ^ i);
    }
    return write_answer(out, cap, values, 6);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t index = (uint32_t)challenge & 7u;
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    uint32_t result;
    /* The jump-table order differs from the p3_case_N symbol suffixes. */
    switch (index) {
        case 0: result = 5u * x + 0x4f411u; break;
        case 1: result = 3u * x + 0x5507u; break;
        case 2: result = (x ^ 0x5525u) + (x >> 1); break;
        case 3: result = 3u * x + 0x6263u; break;
        case 4: result = (x << 2) ^ 0x1e49u; break;
        case 5: result = (x << 1) ^ 0x1f02u; break;
        case 6: result = (x ^ 0x787du) + (x >> 2); break;
        default: result = 5u * x + 0x23023u; break;
    }
    uint32_t values[] = {index, result};
    return write_answer(out, cap, values, 2);
}

static uint32_t walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + 0x22u) ^ salt;
    if (target < middle) {
        return (3u * walk(target, low, middle - 1u, salt)) ^ (middle + salt) ^ 2u;
    }
    return (3u * walk(target, middle + 1u, high, salt)) ^ (middle + salt) ^ 5u;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    uint32_t values[] = {walk((uint32_t)challenge & 127u, 0u, 127u, salt)};
    return write_answer(out, cap, values, 1);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* Read from this personal bomb: (value, id, next pointer) at 16-byte intervals. */
    static const struct { uint32_t value, id; unsigned next; } nodes[] = {
        {0x38c2u, 0x85cu, 0}, {0x9358u, 0x548u, 10},
        {0x2ecau, 0x92bu, 5}, {0x3e11u, 0xa60u, 1},
        {0x6132u, 0x30fu, 0}, {0x03b0u, 0x274u, 4},
        {0x7779u, 0x10eu, 7}, {0x8757u, 0xb4fu, 9},
        {0x56d8u, 0x071u, 2}, {0x612du, 0x737u, 8},
        {0xb218u, 0x656u, 6}, {0x9b43u, 0x461u, 3}
    };
    static const unsigned heads[] = {11, 3, 1, 10, 6, 7, 9, 8};
    unsigned node = heads[(uint32_t)challenge & 7u];
    unsigned count = ((uint32_t)(challenge >> 3) & 3u) + 1u;
    uint32_t result = (uint32_t)(challenge >> 6) ^
                      (uint32_t)(challenge >> 32) ^ 0x862u;
    uint32_t last_id = 0;
    for (unsigned i = 0; i < count; ++i) {
        result = (5u * (result + nodes[node].value)) ^ nodes[node].id;
        last_id = nodes[node].id;
        node = nodes[node].next;
    }
    uint32_t values[] = {result, last_id};
    return write_answer(out, cap, values, 2);
}

static uint32_t rotate_left(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static uint32_t operation(unsigned slot, uint32_t x) {
    /* This is the actual p6_ops table order, including repeated entries. */
    switch (slot) {
        case 0: return (x + 0x7ecfu) ^ (x >> 3);
        case 1: return (x ^ (x >> 7)) + 0xdef2u;
        case 2: case 6: return ((x << 5) + 0x2259u) ^ (5u * x);
        case 3: case 7: return (x >> 3) | (x << 29);
        case 4: return (x ^ 0x2980u) + (x << 5);
        default: return rotate_left(x, 3) ^ 0x2da9u;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    unsigned first = (uint32_t)challenge & 7u;
    unsigned second = (uint32_t)(challenge >> 3) & 7u;
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t result = operation(second, operation(first, x) ^ 0x8d96u) + 0x480au;
    uint32_t values[] = {result};
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t a = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t b = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t mode = (uint32_t)context & 3u;
    uint32_t mixed = (rotate_left(a ^ b, 1) + 0x9351u) ^ 0xd220u;
    uint32_t answer = mixed ^ ((mode * 0x9351u) ^ (uint32_t)(context >> 16));
    uint32_t check = answer ^ (b + mode + 0xd220u + (a >> 1));
    /* secret_copy reaches the in-object gate field after 32 bytes (little endian OPEN).
       The numeric string starts at byte 36; no return address is touched. */
    int size = snprintf(out, cap, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                        " %" PRIu32 " %" PRIu32, mode, answer, check);
    return size < 0 || (size_t)size >= cap;
}

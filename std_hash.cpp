//gcc std_hash.cpp -lstdc++
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <stdint.h>
#include <functional>
using namespace std;

unsigned long simple_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U

uint32_t FNV32(const char *s)
{
    uint32_t hash = FNV_OFFSET_32, i;
    for (i = 0; i < strlen(s); i++)
    {
        hash = hash ^ (s[i]);       // xor next byte into the bottom of the hash
        hash = hash * FNV_PRIME_32; // Multiply by prime number found to work well
    }
    return hash;
}

static inline uint32_t murmur_32_scramble(uint32_t k)
{
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}
uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed)
{
    uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--)
    {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--)
    {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

struct custom_map
{
    string sql;
    int count;
    int collisions;
};

//
// generate random test strings
//
#define MAX 1000

static int getRandom(int low, int high)
{
    return rand() % (high + 1 - low) + low;
}

char *GetTestString()
{
    static char sql[256];
    sprintf(sql, "UPDATE table SET total=total+__TOTAL__ WHERE key = %d", getRandom(0, MAX));
    return sql;
}

// change the sparseness of the array
#define SPARSITY (MAX * 5)
#define TESTRUNS 1000
#define UPDATES 10000
int main(int args, char **argc)
{
    int count = 0;
    int entries = 0;

    struct custom_map map_entry[SPARSITY];

    for (int i = 0; i < SPARSITY; i++)
    {
        map_entry[i].sql = "";
        map_entry[i].count = 1;
        map_entry[i].collisions = 0;
    }

    for (int tests = 0; tests < TESTRUNS; tests++)
    {
        for (int i = 0; i < UPDATES; i++)
        {
            string sql = GetTestString();

            // choose your hash
            //int index =FNV32(sql.c_str()) % SPARSITY;
            int index = simple_hash(sql.c_str()) % SPARSITY;
            //int index =murmur3_32((uint8_t *)sql.c_str(),sql.length(),0) % SPARSITY;
            //int index =hash<std::string>{}(sql) % SPARSITY;

            // find or create the sql entry in a sparse array
            do
            {
                if (map_entry[index].sql == sql) // we have the entry
                {
                    map_entry[index].count++;
                    break;
                }
                else if (map_entry[index].sql == "") // blank entry
                {
                    map_entry[index].sql = sql;
                    break;
                }
                else // collision so move to next element
                {
                    map_entry[index].collisions++;
                    index = (index + 1) % SPARSITY;
                }
            } while (1);
        }

        for (int i = 0; i < SPARSITY; i++)
        {
            printf("%d",map_entry[i].sql != ""?1:0);
            if (map_entry[i].sql != "")
            {
                
                count += map_entry[i].count;
                entries += 1;
                map_entry[i].sql = "";
                map_entry[i].count = 1;
            }
        }
    }
    int total_collisions = 0;
    for (int i = 0; i < SPARSITY; i++)
    {
        
        total_collisions += map_entry[i].collisions;
    }
    printf("Total %d %d collisions %d\n", count, entries, total_collisions);
}
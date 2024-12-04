/* MESI Types */
#define INVALID 0b00	/* Data is invalid */
#define SHARED 0b01	    /* Data is shared in other caches */
#define EXCLUSIVE 0b10	/* This cache is exclusive owner */
#define MODIFIED 0b11	/* This cache has a modified version of the data */

/* Snoop Result Types*/
#define NOHIT1 0b10		/* No hit */
#define NOHIT2 0b11     /* No hit */
#define HIT 0b00		/* Hit */
#define HITM 0b01		/* Hit to modified line */

/* Bus Operation Types */
#define READ 0b00		/* Bus Read */
#define WRITE 0b01		/* Bus Write */
#define INVALIDATE 0b10	/* Bus Invalidate */
#define RWIM 0b11		/* Bus Read with Intent to Modify */

/* Current to Higher Cache Message Types */
#define GETLINE 0b00	/* Request a modified line from higher cache */
#define SENDLINE 0b01		/* Send a requested line to higher cache */
#define INVALIDATELINE 0b10	/* Invalidate a line in higher cache */
#define EVICTLINE 0b11		/* Evict a line in higher cache */

/* Address Operations */
#define READ_HD 0b0000		/* Read request from higher data cache */
#define WRITE_HD 0b0001		/* Write request from higher data cache */
#define READ_HI 0b0010		/* Read request from higher instruction cache */
#define READ_S 0b0011		/* Snoop read request */
#define WRITE_S 0b0100		/* Snoop write request */
#define RWIM_S 0b0101		/* Snoop read with intent to modify request */
#define INVALIDATE_S 0b0110	/* Snoop invalidate command */
#define CLEAR 0b1000		/* Clear the cache and reset all states */
#define PRINT 0b1001		/* Print content and state of each valid cache line */

/* Memory Size Defaults */
#define CACHE_SIZE 24		/* 2^(CACHE_SIZE) = True Capacity in Bytes */
#define CACHE_LINE_SIZE 64	/* Number of bytes per line */
#define ADDRESS_SIZE 32		/* Number of bits in address (no need to change) */
#define ASSOCIATIVITY 16	/* Number of ways per set */

/* Tag Array Sizes */
#define TAG_ARRAY_MESI 2


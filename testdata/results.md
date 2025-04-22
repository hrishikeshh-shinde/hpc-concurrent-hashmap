Unique strings of size N = 1e6.
Half of them would be inserted and half of them would be not.

| Operation | ChainHashMapTest(BUCKETS = 1024 \* 1024) | ThreadSafeChainHashMapTest(BUCKETS = 1024 \* 1024) | UnorderedSetTest | ThreadSafeUnorderedSetTest |
| --------- | ---------------------------------------- | -------------------------------------------------- | ---------------- | -------------------------- |
| Insert    | 199.838 ms                               | 38.8408 ms                                         | 186.999 ms       | 331.443 ms                 |
| Search    | 417.872 ms                               | 41.655 ms                                          | 213.353 ms       | 26.0148 ms                 |
| Delete    | 433.294 ms                               | 57.5499 ms                                         | 256.569 ms       | 665.818 ms                 |

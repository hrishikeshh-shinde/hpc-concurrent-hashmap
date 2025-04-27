Unique strings of size N = 1e6.  
Half of them would be inserted and half of them would be not.

| Operation | ChainHashMapTest (BUCKETS = 1024*1024) | ChainHashMapRehashTest | ThreadSafeChainHashMapTest (BUCKETS = 1024*1024) | UnorderedSetTest | ThreadSafeUnorderedSetTest |
|-----------|----------------------------------------|------------------------|--------------------------------------------------|------------------|----------------------------|
| Insert    | 405.038 ms                             | 78.6214 ms                      | 78.543 ms                                        | 395.93 ms        | 632.246 ms                 |
| Search    | 835.803 ms                             | 138.573 ms                      | 101.883 ms                                       | 635.103 ms       | 61.8629 ms                 |
| Delete    | 885.286 ms                             | 138.573 ms                      | 101.883 ms                                       | 638.183 ms       | 61.8629 ms                 |

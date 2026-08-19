# Benchmark
- [x] bump allocator
- [x] memory header (8-bits)
- [x] segregated linked lists for freed memory blocks that fit inside a bin \
    **Searching (Allocating)** \
    ***average:*** pop from the first element in a list `O(1)` or `O(k)` \
    where `k` be the number of bins \
    **Searching (Coalescing)** \
    ***average:*** pop from the first element in a list `O(N)`

- [X] manage free large memory blocks with duplicated trees (one by size and another by address)
    - [X] simple binary search trees \
        **Searching** \
        ***average:*** freed memory blocks are well distributed `O(log N)` \
        ***worst case:*** highly unbalanced `O(N)`
    - [ ] self-balancing binary search trees
- [x] block splitting 
- [ ] coalescing ***※currently*** 

## allocator alogorithm
```python
prev_size = 0
free_list = list(linked_list, largest_bin / 16)
free_large = {by_addr: tree(None), by_size: tree(None)}

def alloc(size: i32) -> pointer :
    size = size + sizeof(header)
    size = align16(size)
    # search from the free list / tree
    ptr = search_fit(size)
    if ptr is not None:
        return ptr
    # bump
    ptr = brk + size
    if ptr > end:
        return None
    ptr = ptr - size + sizeof(header)
    setheader(ptr, size)
    prevsize = size
    total = total + size
    return ptr

def free(ptr: pointer):
    ptr.header.status = FREE

    # try to coalesce
    # TODO

    # adding to list/tree
    size = ptr.header.size
    if size > largest_bin:
        tree_add(
            free_large.by_addr,
            ptr,
            lambda p1, p2: p1 > p2
        )
        tree_add(
            free_large.by_size,
            ptr,
            lambda p1, p2: p1.header.size > p2.header.size
        )
    else:
        index = ptr.header.size / 16 - 1
        list_insert_front(free_list[index], ptr)


def setheader(ptr: pointer, size: i32):
    ptr.header.status = OCCUPIED
    ptr.header.prevsize = prevsize
    ptr.header.size = size

def search_fit(size: i32) -> pointer:
    ptr = None
    index = size / 16 - 1
    # search from list starting from the smallest fit bin
    while index < bin_count:
        if free_list[index] != None:
            ptr = list_pop(free_list[index])
            break
        index += 1

    # search from tree
    if ptr is None:
        ptr = search_tree(size)
    
    # failed
    if ptr is None:
        return None

    # update header
    setheader(ptr,size)

    # split if possible
    # TODO
    
    total = total + size
    return ptr

def search_tree(size: i32) -> pointer:
    ptr = tree_delete(
        free_large.by_size, 
        size, 
        lambda ptr, size: ptr.header.size > x
    )
    if ptr is not None:
        tree_delete(
            free_large.by_addr, 
            ptr,
            lambda p1, p2: p1 > p2
        )
    return ptr
```


# Note
- Memory management is done on payload boundary and header fields are accessed via negative offsets.
- 16-byte alignment guarantee
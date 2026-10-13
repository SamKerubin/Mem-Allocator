# Mem-Allocator

Mem-Allocator is a shared Linux library that handles dynamic memory allocations.
It works as an implementation of the standard functions
`malloc`, `free`, `realloc`, `calloc`, and `aligned_alloc`.

## Installation

Compile the library using `make`.

If you want a global system installation, use `make install`

## Support

Mem-Allocator has support for both 32 bit and 64 bit architectures.

## Testing

It is recommended using tools like `gdb` for testing.

### Setting up GDB for testing

Compile the library.

Run `gdb` + a binary you want to test

```
gdb /bin/ls
```

Before starting the execution, setup the environment for GDB:

```
set environment LD_PRELOAD=./result/lib/libmemalloc.so
set startup-with-shell off
starti
continue
```

If everything goes well, the execution will finish without crashing.
If not, check the backtrace:

```
backtrace
```

## Contributions

As this is a really experimental library, you might find some issues: crashes, low performance.

Feel free to open an issue if you find any trouble!

## Future development

- Adding support for more architectures
- Further testing with other binaries
- Benchmarking Mem-Allocator and glibc's `malloc` implementation
- Usage in a full live system

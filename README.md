# Async I/O Library with io_uring

A modern C++ wrapper around Linux's io_uring interface for high-performance asynchronous I/O operations.

## Overview

This library provides a high-level C++ API for io_uring, Linux's newest asynchronous I/O interface. It abstracts the low-level complexity of io_uring while maintaining its performance characteristics, offering a builder pattern for operation construction and callback-based completion handling.

## Features

- **Fluent Builder API**: Chain method calls to construct I/O operations
- **Callback-based Completion**: Separate success and failure handlers per operation
- **Automatic Submission**: Configurable threshold for automatic batch submission
- **Zero-overhead Abstraction**: Thin wrapper over raw io_uring structures
- **Type-safe Operations**: C++ interface over C io_uring implementation

## Architecture

The library is structured in two layers:

1. **C Layer** (`src/io_uring/`): Low-level io_uring ring management, submission queue, and completion queue handling
2. **C++ Layer** (`src/Async/`): High-level AsyncIOInterface with OPConstructor builder pattern

## Goals

- **Performance**: Minimize overhead while providing ergonomic abstractions
- **Simplicity**: Make io_uring accessible without deep kernel knowledge
- **Extensibility**: Support the full range of io_uring operations as they evolve
- **Reliability**: Robust error handling and resource management
- **Modularity**: Clean separation between C and C++ components

## Project Status

This is an active development library. Core functionality for basic read/write operations is implemented. See `to-do.md` for planned features and enhancements.

## Building

```bash
# Build io_uring C objects
cd src/io_uring
make

# Build the main project (from root)
# Note: Root Makefile needs to be configured
```

## Usage Example

```cpp
AsyncIOInterface io(2048, -1);

int fd = open("file.txt", O_WRONLY | O_CREAT, 0644);
std::string data = "Hello, io_uring!";

io.new_op()
    .op(IORING_OP_WRITE)
    .fd(fd)
    .addr((void*)data.c_str())
    .len(data.size())
    .offset(0)
    .OnSuccess([](int res, int flags) { 
        std::cout << "Written: " << res << " bytes" << std::endl; 
    })
    .OnFailure([](int res, int flags) { 
        std::cerr << "Error: " << res << std::endl; 
    })
    .register_op();

io.submit();
io.await_completions();
close(fd);
```

## Requirements

- Linux kernel 5.1+ (5.11+ recommended for full io_uring feature support)
- GCC/Clang with C++11 support
- liburing development headers (optional, currently using direct syscalls)

## AI Usage

AI has been wisely used as a development tool in this project for specific tasks:

- **Documentation**: Assistance with README and roadmap formatting
- **Build Automation**: Makefile generation for compilation tasks
- **Brainstorming**: Feature exploration and planning discussions

**Important**: This library is NOT AI-written. All core functionality, architecture decisions, io_uring integration, and performance-critical code are human-authored. AI serves purely as a productivity tool for auxiliary tasks, not as a code generator.

## License

This project currently has no license specified.

## Contributing

This project is not accepting contributions at this time as the codebase structure is evolving rapidly and undergoing frequent architectural changes.

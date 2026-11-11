# Smart Pointer Policies in Modern C++ (Loki23)

Source code accompanying a work by V. Boublik, A. Trokhymchuk, A. Romaniuk and O. Zhytnetskyi (National University of Kyiv-Mohyla Academy):
- *Towards the Issue of Smart Pointer Policies in Modern C++*. Theses, TAAPSD 2026.
   
## Where to look

### 1. Loki23 `SmartPtr`: what changed compared to Loki

| File | What it is |
|---|---|
| [`lib/smart_ptr/smart_ptr.cppm`](lib/smart_ptr/smart_ptr.cppm) | The `SmartPtr` host class. Its `requires` clause encodes the dependencies between policies that Loki only described in documentation. For example, `IntrusiveOwnership` is accepted only for a type with its own reference counter and only with raw-pointer storage. Move semantics replace destructive copying. A variadic constructor builds the object in place. |
| [`lib/smart_ptr/storage_policy.cppm`](lib/smart_ptr/storage_policy.cppm) | The `StoragePolicy` concept and its storage policies. Storage holds a general *handle*, not only a pointer: `DefaultStorage`, `LockedStorage`, `FileDescriptorStorage`, `SocketStorage` (POSIX only), `SmallBufferStorage` |
| [`lib/smart_ptr/ownership_policy.cppm`](lib/smart_ptr/ownership_policy.cppm) | The `OwnershipPolicy` / `Clonable` concepts and the ownership policies: `RefCounted`, `RefCountedMT`, `RefLinked`, `DeepCopy`, `Copy`, `NoCopy`, `UniqueOwnership`, `SharedOwnership`, `IntrusiveOwnership` |
| [`lib/smart_ptr/checking_policy.cppm`](lib/smart_ptr/checking_policy.cppm) | What happens when a null handle is dereferenced: `NoCheck`, `AssertCheck`, `ExceptionCheck`, `HaltCheck`. |
| [`lib/smart_ptr/conversion_policy.cppm`](lib/smart_ptr/conversion_policy.cppm) | Whether implicit conversion to the raw handle is allowed: `AllowConversion` or `DisallowConversion` |
| [`src/resource.cpp`](src/resource.cpp) | Demo of one `SmartPtr` managing heap memory, file descriptors, TCP sockets and intrusively counted objects. Each case uses a different policy combination. POSIX only. |
| [`src/smart_ptr.cpp`](src/smart_ptr.cpp) | Minimal example: shared ownership through `RefCounted` and in-place construction |

Every policy requirement is a C++20 concept. A wrong policy is rejected during template argument
substitution, with a short "constraint not satisfied" message instead of a long instantiation
error. Each part of the library is a C++20 module (`import loki23;`), not a header.

```cpp
template <typename Tval,
          typename OwnPolicy   = DeepCopy,
          typename CheckPolicy = NoCheck,
          template <typename> typename StorePolicy = DefaultStorage,
          typename ConvPolicy  = DisallowConversion>
class SmartPtr;
```

### 2. `SmallBufferStorage`: no heap allocation for small objects

| File | What it is |
|---|---|
| [`lib/smart_ptr/storage_policy.cppm`](lib/smart_ptr/storage_policy.cppm) | `SmallBufferStorage<T, N, Allocator>`. If `sizeof(T) <= N` and the alignment fits, `if constexpr` constructs the object in an aligned inline buffer. Otherwise the object goes to the allocator. The allocator member is marked `[[no_unique_address]]`, so a stateless allocator such as `std::allocator` takes no space. |
| [`src/smart_ptr_small_buffer.cpp`](src/smart_ptr_small_buffer.cpp) | Demo of inline and heap placement for different types and buffer sizes, including moves. A `static_assert` in each case checks the compile-time decision. |

The default buffer size, `N = sizeof(void*) * 4`, gives defined behaviour on any platform. In the
code, the storage policy takes the pointee type as its first parameter, because `SmartPtr` expects
a `template <typename> typename` storage policy. To use a custom `N` or a custom allocator, bind them
with an alias template:

```cpp
template <typename U> using TinyBuffer = SmallBufferStorage<U, 4>;

SmartPtr<int,     UniqueOwnership, NoCheck, SmallBuffer> a(42);    // inline (default N = 32 on 64-bit)
SmartPtr<int64_t, UniqueOwnership, NoCheck, TinyBuffer>  b(123);   // does not fit, so it goes to the heap
```

### 3. `RefCounted` / `RefCountedMT`: thread safety as a type-level choice

| File | What it is |
|---|---|
| [`lib/smart_ptr/ownership_policy.cppm`](lib/smart_ptr/ownership_policy.cppm) | `RefCounted` keeps a plain `std::size_t` count with no atomic operations, so single-threaded code does not pay for synchronization it doesn't need. `RefCountedMT` keeps a `std::atomic<std::size_t>`. It increments with `memory_order_relaxed` and decrements with `memory_order_acq_rel`, so the destructor sees every write made by every thread that held a reference. |

The two policies have the same interface, so switching between them changes only the type:

```cpp
using Local  = SmartPtr<Widget, RefCounted>;    // single-threaded: plain counter
using Shared = SmartPtr<Widget, RefCountedMT>;  // multi-threaded: atomic counter
```

`RefCountedMT` keeps the **reference count** correct across threads. It does **not** synchronize
access to the managed object itself. The theses leave this as an open question.

### Other parts of the repository

The rest of `lib/` and `src/` belongs to Loki23 but is outside the scope of the theses. It covers
typelists, hierarchy generation, multimethods, singleton, object factory and a small DI container.
[`src/bench.cpp`](src/bench.cpp) is a micro-benchmark for the whole library, `SmartPtr` included.

## Building

Requires C++23 with C++20 modules support, CMake 4.0+ and the Ninja generator.
`FileDescriptorStorage`, `SocketStorage` and the `resource` demo are POSIX only.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/src/smart_ptr_small_buffer   # SmallBufferStorage
./build/src/resource                 # policy combinations for different resources
./build/src/smart_ptr                # minimal RefCounted example
```

A Debug build, which is the default when no build type is set, turns on a strict warning set and
AddressSanitizer, UBSan and LeakSanitizer.

## Discussion

Feedback is very welcome, including results on other compilers and platforms, criticism of the
design and counter-examples: please use **Discussions** or open an **Issue**.

## License

MIT

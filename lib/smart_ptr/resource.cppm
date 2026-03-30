module;

#include <unistd.h>

export module loki23.smart_ptr.resource;

export import loki23.smart_ptr; // Re-exports SmartPtr + all policies

// ============================================================
// POSIX storage policies
// T is a phantom type (handle is always int); required by the
// template<typename> typename StorePolicy parameter of SmartPtr.
// ============================================================


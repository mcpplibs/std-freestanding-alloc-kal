// The replaceable allocation functions, forwarded to openkal.
//
// WHY THIS IS A PACKAGE AND NOT PART OF THE SUBSET
//
// `operator new` is a whole-program singleton: the language permits exactly one
// definition, and the program is the party entitled to choose it. A library
// that shipped one would be deciding on the program's behalf, and additive
// features offer no way to switch a default off again. The subset therefore
// carries a feature that REQUIRES an allocator capability, and the
// implementations are separate packages that PROVIDE it. The resolver then
// binds exactly one, and two implementations in a single graph are reported
// before the build rather than as a duplicate definition during the link.
//
// WHY THE DECLARATIONS ARE WRITTEN OUT RATHER THAN INCLUDED
//
// `std::align_val_t` is declared in <new>. Reaching that header would require
// the configured libc++ include set that the subset package assembles, and
// depending on the subset from here would close a cycle: the subset's
// `alloc-kal` feature pulls this package. The declarations below are the ones
// the standard specifies, written in the form a replacement implementation is
// expected to use.
//
// ⚠️ TWELVE, AND THE LAST FOUR ARE THE ONES THAT GET FORGOTTEN.
//
// Measured on riscv64-none-elf: defining the eight sized and unsized forms
// leaves the link failing on `operator delete(void*, unsigned long,
// std::align_val_t)`, which libc++ reaches from `__libcpp_deallocate` for every
// container element type. Omitting the aligned overloads produces a second
// link failure after the first is fixed, which is why they are here.

using size_type = __SIZE_TYPE__;

namespace std {
// [support.types] — the type the aligned overloads are selected by.
enum class align_val_t : size_type {};
}  // namespace std

extern "C" {
// openkal.memory, clause 4.2. Declared rather than included so that this
// package needs no header search path of its own; the C ABI is the contract,
// and the specification package is depended upon so that a version mismatch is
// reported when the graph resolves rather than when the link runs.
void* kal_alloc(size_type size, size_type align);
void  kal_free(void* p, size_type size, size_type align);
}

namespace {
// The alignment an unaligned allocation must satisfy.
//
// ⚠️ `__BIGGEST_ALIGNMENT__` and not `alignof(std::max_align_t)`: the latter
// lives in <cstddef>, which would mean depending on the subset package and
// closing a cycle. The macro is predefined by both GCC and Clang and is the
// same quantity. Measured: `alignof(__max_align_t)` does not compile here at
// all — that identifier comes from <stddef.h>, which a package with no C
// library include path does not have.
constexpr size_type kDefaultAlign = __BIGGEST_ALIGNMENT__;

// ⚠️ Size and alignment are forwarded to `kal_free` because openkal's contract
// requires them: an implementation may be a bump allocator, a slab, or a
// wrapper over the C library's, and only the first of those can ignore them.
// The unsized `operator delete` overloads have no size to pass and give zero,
// which the specification defines as "not stated"; an implementation that needs
// the size must therefore record it itself. This is a real constraint on
// implementations and is stated in the README rather than hidden here.
inline void* allocate(size_type n, size_type align) {
    // A zero-sized allocation must still return a distinct non-null pointer.
    return kal_alloc(n ? n : 1, align);
}
}  // namespace

// ── Unaligned ───────────────────────────────────────────────────────────────
void* operator new(size_type n)                       { return allocate(n, kDefaultAlign); }
void* operator new[](size_type n)                     { return allocate(n, kDefaultAlign); }
void  operator delete(void* p) noexcept               { kal_free(p, 0, kDefaultAlign); }
void  operator delete[](void* p) noexcept             { kal_free(p, 0, kDefaultAlign); }
void  operator delete(void* p, size_type n) noexcept  { kal_free(p, n, kDefaultAlign); }
void  operator delete[](void* p, size_type n) noexcept{ kal_free(p, n, kDefaultAlign); }

// ── Aligned ─────────────────────────────────────────────────────────────────
void* operator new(size_type n, std::align_val_t a) {
    return allocate(n, static_cast<size_type>(a));
}
void* operator new[](size_type n, std::align_val_t a) {
    return allocate(n, static_cast<size_type>(a));
}
void operator delete(void* p, std::align_val_t a) noexcept {
    kal_free(p, 0, static_cast<size_type>(a));
}
void operator delete[](void* p, std::align_val_t a) noexcept {
    kal_free(p, 0, static_cast<size_type>(a));
}
void operator delete(void* p, size_type n, std::align_val_t a) noexcept {
    kal_free(p, n, static_cast<size_type>(a));
}
void operator delete[](void* p, size_type n, std::align_val_t a) noexcept {
    kal_free(p, n, static_cast<size_type>(a));
}

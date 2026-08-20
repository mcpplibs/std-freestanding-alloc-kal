# std-freestanding-alloc-kal

The replaceable allocation functions for [`std-freestanding`][subset],
forwarded to [openkal][kal].

```toml
[dependencies]
std-freestanding = { version = "0.3.0", features = ["alloc-kal"] }
```

That is the whole of the consumer's side. The feature both states the
requirement and brings this package, so the package name never has to be
written down.

## What this is for

A freestanding target has no compiled `libc++`, so `operator new` does not
exist. The moment a program uses `std::vector`, `std::string` or a coroutine,
the link fails naming a mangled symbol from a header inside the standard
library. The non-allocating half of the subset — `array`, `span`, `optional`,
`atomic`, `string_view`, `ranges` — needs nothing and is unaffected.

## Why the implementation is a package rather than part of the subset

`operator new` is a whole-program singleton: the language permits one
definition, and the program is entitled to choose it. A library that shipped
one would decide on the program's behalf, and features are additive, so a
consumer would have no way to switch the default off.

The subset therefore declares a feature that **requires** the
`freestanding-allocator` capability, and implementations **provide** it. The
resolver binds exactly one and reports two by name:

```
error: capability 'freestanding-allocator' has multiple providers in the graph:
       [std-freestanding-alloc-kal, std-freestanding-alloc-libc]
```

Two definitions reaching the linker would instead produce a duplicate symbol
naming a mangled operator, which is the same defect with a worse message.

## What an openkal implementation must supply

`kal_alloc(size, align)` and `kal_free(p, size, align)`. On bare metal the
board package supplies them — the console and the heap region are board facts.

⚠️ The unsized `operator delete` overloads have no size to pass and give zero,
which openkal defines as "not stated". An implementation that needs the size in
order to free must therefore record it itself; one built over a C library's
allocator, as the reference bare-metal backend is, does not.

## Choosing the other one

[`std-freestanding-alloc-libc`][libc] forwards to the target's C library
instead, and is the shorter path when the target has one. Exactly one of the two
may be in a graph.

[subset]: https://github.com/mcpplibs/std-freestanding
[kal]: https://github.com/mcpplibs/openkal
[libc]: https://github.com/mcpplibs/std-freestanding-alloc-libc

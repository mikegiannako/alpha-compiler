## Convention Guide

### Naming Conventions

- Use `camelCase` for variables.
- Function names have the format `typeInCamel_ActionInPascal` (e.g., `strBuffer_AppendChar`). If no type is applicable, use `camelCase` for the function name (e.g., `getEscapeChar`).
- Struct names have the format `datatypeDataStructure_vartype` (e.g., `strBuffer_ptr` which means that the strBuffer_ptr type is a point to a struct called `struct strBuffer`).
- Use `ALL_CAPS` for macros and constants.

### File Naming Conventions

- Each file should be named after the primary type it contains, using `snake_case`.
- For most `.c` files there is a corresponding `.h` file with the same name.

### Memory Allocation Conventions

- Use `safeCalloc`, `safeRealloc`, and `safeFree` for memory management.

### Macros Conventions

- Macro names should be in `ALL_CAPS` and use underscores to separate words.
- Macro definitions should be contained withing `do { ... } while (0)` to ensure they behave like statements.
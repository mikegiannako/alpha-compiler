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

- Use `safeCalloc`, `safeRealloc`, and `safeFree` for memory management as well as all the other functions in the memory_operations.h file
- When using one of the functions above a "desc" field is required. This is the description of the operation you're trying to do using this function. This is used for easier debugging. The description should be of the form "duplicating string used for ..."

### Macros Conventions

- Macro names should be in `ALL_CAPS` and use underscores to separate words.
- Macro definitions should be contained withing `do { ... } while (0)` to ensure they behave like statements.
- Exception to the rules above are the memory operation macros which are meant to be used as regular functions
# Core Idioms and Architectural Patterns in CIRCT/MLIR

Here is a round-up of the core idioms and architectural patterns used across the CIRCT codebase for writing passes and lowering transformations. CIRCT leans heavily on MLIR’s declarative and functional abstractions.

## 1. The Pattern Matching Hierarchy
CIRCT passes almost never mutate the IR tree directly in a `walk()`. Instead, they use a robust pattern-matching framework. 

*   **`OpRewritePattern<T>`**: Used for **intra-dialect** transformations. If you have a pass that takes a `comb.add` and optimizes it to a `comb.shl`, you use this.
*   **`OpConversionPattern<T>`**: Used for **inter-dialect** transformations. If you are taking an operation from dialect A (like `ltl.delay`) and rewriting it into dialect B (like `seq.compreg`), you use this. The key difference is it supports `TypeConverter`s so you can systematically map types (like `!ltl.property` -> `i1`). 

**Idiom:** Patterns are stateless by default. If a pattern *needs* state, you pass a reference into its constructor from the Pass driver.

## 2. Transactional Rewriting (`ConversionPatternRewriter`)
In MLIR, because patterns can fail and rollback, you must go through the `rewriter` for all mutations.
*   **`replaceOpWithNewOp<OpType>(oldOp, args...)`**: The most common idiom. This creates a new operation right where the old one was, automatically wires all downstream users of the old operation to the new one, and schedules the old operation for deletion.
*   **`replaceOp(oldOp, existingValues)`**: If the logic to replace an operation already exists in the tree (or you just built it), you can just point the `oldOp` to those existing values.
*   **`modifyOpInPlace(oldOp, lambda)`**: When you aren't replacing an operation, but you are changing an attribute, swapping an operand, or changing its region blocks.

**Idiom:** Never call standard MLIR mutators (like `op->setOperand()`) inside a pattern unless it is wrapped in `modifyOpInPlace`. 

## 3. "Or-Fold" Builders
When building new combinational logic, you will see functions like `comb::createOrFoldNot` or `builder.createOrFold<Op>`. 
*   **Why?** In hardware, it's very common to accidentally generate things like `NOT(NOT(A))`. 
*   **Idiom:** Instead of building a raw `NotOp` and waiting for a canonicalization pass to clean it up later, CIRCT heavily uses "Fold" builders. These functions peek at the inputs; if they can mathematically optimize the operation immediately (e.g., they see the input is already a constant `1`), they will just return the optimized value instead of generating a new IR node.

## 4. The "FailureOr" and "LogicalResult" Pattern
Because MLIR is a compiler framework, exceptions (`throw`/`catch`) are strictly forbidden. 
*   **`LogicalResult`**: This is just a wrapper around a boolean (`success()` or `failure()`). Every pattern's `matchAndRewrite` must return this.
*   **`FailureOr<T>`**: If a helper function needs to return a value, but might fail, it returns this.

**Idiom:** Early exits. Patterns are written to "fail fast". The first 10 lines of a pattern are usually checking types and constraints and returning `failure()` if they don't match.

## 5. Diagnostics
**Idiom:** Rather than printing to `std::cerr`, errors are bound to specific locations in the source code using `op->emitError("message")`. 
This allows MLIR to trace the error back to the exact line/column in the `.mlir` (or even the original SystemVerilog/FIRRTL file) so the user knows exactly what went wrong. When writing tests (`--verify-diagnostics`), you add `// expected-error @below {{message}}` to ensure these checks are firing correctly.

## 6. The Target and the Driver
Every conversion pass ends with a configuration block.
**Idiom:** You don't tell the pass *how* to traverse the tree, you just declare the rules.
1.  **`ConversionTarget`**: You define what is legal (e.g., `target.addLegalDialect<hw::HWDialect>()`) and what is illegal (`target.addIllegalOp<ltl::DelayOp>()`).
2.  **`ApplyPartialConversion`**: You hand the target and your patterns to this framework function. It is called "Partial" because it's okay if some operations in the module are ignored/left alone. (Conversely, `ApplyFullConversion` will crash the compiler if *any* illegal operation survives the pass).
## 7. The Template Pattern for Multi-Op Matching
Often, the exact same transformation applies to multiple operations (e.g., both `verif.assert` and `verif.assume`). Instead of duplicating the struct, CIRCT uses C++ templates for the pattern:
```cpp
template <typename Op>
struct RemoveEnableTrue : public OpRewritePattern<Op> {
  // ...
};

// Registration:
patterns.add<RemoveEnableTrue<verif::AssertOp>, RemoveEnableTrue<verif::AssumeOp>>(context);
```

## 8. Complex Sub-Matching (`matchPattern`)
Instead of manually navigating the IR tree with long chains of `getDefiningOp<...>()` and checking if things exist, CIRCT leverages MLIR's built-in `matchPattern` utility.
```cpp
// Instead of:
// auto sext = input.getDefiningOp<comb::SextOp>();
// if (sext) { Value inner = sext.getInput(); ... }

// You use:
Value inner;
if (matchPattern(input, comb::m_Sext(m_Any(&inner)))) {
  // `inner` is automatically populated with the nested value.
}
```
**Idiom:** Use pattern matchers to declaratively describe the shape of the IR you are looking for. It handles the null-checking for you.

## 9. Canonicalizers (`getCanonicalizationPatterns`)
If a pattern represents a universal mathematical or structural simplification (e.g., `Reverse(Reverse(A)) == A`), it doesn't belong in an ad-hoc pass. It belongs in the operation's canonicalizer.
```cpp
void comb::ReverseOp::getCanonicalizationPatterns(RewritePatternSet &results,
                                                  MLIRContext *context) {
  results.add<ReverseOfReverse>(context);
}
```
**Idiom:** By tying the pattern directly to the operation, the global `circt-opt -canonicalize` pass will automatically apply it anywhere the operation is found.

## 10. Symbol Table Mutations (`SymOpRewritePattern`)
When mutating operations that define symbols (like modules or custom `arc.define` functions), standard patterns can corrupt the global Symbol Table if they blindly rename or delete things.
**Idiom:** Use `SymOpRewritePattern<T>` instead of `OpRewritePattern<T>`. It provides specialized access to a cached `SymbolTable` to ensure cross-references aren't broken during the rewrite.

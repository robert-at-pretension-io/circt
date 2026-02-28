//===- VerifPassPipelines.h - Verif pass pipelines --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This header file defines prototypes that expose pass pipelines.
//
//===----------------------------------------------------------------------===//

#ifndef CIRCT_DIALECT_VERIF_VERIFPASSPIPELINES_H
#define CIRCT_DIALECT_VERIF_VERIFPASSPIPELINES_H

#include "circt/Dialect/Verif/VerifPasses.h"
#include "mlir/Pass/PassManager.h"

namespace circt {
namespace verif {

/// Build the pipeline for preparing a circuit for formal verification.
/// This includes lowering contracts, resolving symbolic values, and flattening wires.
void buildPrepareForFormalPipeline(mlir::OpPassManager &pm);

/// Register all pipelines for the Verif dialect.
void registerPipelines();

} // namespace verif
} // namespace circt

#endif // CIRCT_DIALECT_VERIF_VERIFPASSPIPELINES_H

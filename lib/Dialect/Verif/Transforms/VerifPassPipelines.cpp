//===- VerifPassPipelines.cpp - Verif pass pipelines ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the pass pipelines for the Verif dialect.
//
//===----------------------------------------------------------------------===//

#include "circt/Dialect/Verif/VerifPassPipelines.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/Pass/PassManager.h"

using namespace circt;
using namespace verif;

void circt::verif::buildPrepareForFormalPipeline(mlir::OpPassManager &pm) {
  // 1. First, process any verif.contract operations at the Module level.
  // This extracts logic into standalone verif.formal blocks and injects
  // verif.symbolic_value operations into the hardware.
  pm.addPass(createLowerContractsPass());

  // 2. Next, translate those abstract verif.symbolic_value operations
  // into physical hardware abstractions (e.g., black box ports) that
  // formal SMT solvers can understand.
  pm.addPass(createLowerSymbolicValuesPass());

  // 3. Finally, drop down into the individual HW modules and perform
  // the standard formal prep (flattening wires, removing OM classes).
  pm.addNestedPass<hw::HWModuleOp>(createPrepareForFormalPass());
}

void circt::verif::registerPipelines() {
  mlir::PassPipelineRegistration<>(
      "prepare-for-formal-pipeline",
      "Full pipeline to prepare a circuit for formal verification.",
      buildPrepareForFormalPipeline);
}

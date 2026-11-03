module {
  func.func @dsl.soma.1(%arg0: memref<1024xi32>, %arg1: memref<1024xi32>, %arg2: memref<1024xi32>) attributes {passthrough = ["nounwind"]} {
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    cf.br ^bb1(%c0 : index)
  ^bb1(%0: index):  // 2 preds: ^bb0, ^bb2
    %1 = arith.cmpi slt, %0, %c1024 : index
    cf.cond_br %1, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %2 = memref.load %arg0[%0] : memref<1024xi32>
    %3 = memref.load %arg1[%0] : memref<1024xi32>
    %4 = arith.addi %2, %3 : i32
    memref.store %4, %arg2[%0] : memref<1024xi32>
    %5 = arith.addi %0, %c1 : index
    cf.br ^bb1(%5 : index)
  ^bb3:  // pred: ^bb1
    return
  }
  func.func @dsl.somavet.2(%arg0: memref<1024xi32>) -> i32 attributes {passthrough = ["nounwind"]} {
    %c0_i32 = arith.constant 0 : i32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    cf.br ^bb1(%c0, %c0_i32 : index, i32)
  ^bb1(%0: index, %1: i32):  // 2 preds: ^bb0, ^bb2
    %2 = arith.cmpi slt, %0, %c1024 : index
    cf.cond_br %2, ^bb2, ^bb3
  ^bb2:  // pred: ^bb1
    %3 = memref.load %arg0[%0] : memref<1024xi32>
    %4 = arith.addi %1, %3 : i32
    %5 = arith.addi %0, %c1 : index
    cf.br ^bb1(%5, %4 : index, i32)
  ^bb3:  // pred: ^bb1
    return %1 : i32
  }
}


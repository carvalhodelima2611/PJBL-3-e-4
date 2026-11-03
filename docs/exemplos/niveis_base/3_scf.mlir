module {
  func.func @dsl.soma.1(%arg0: memref<1024xi32>, %arg1: memref<1024xi32>, %arg2: memref<1024xi32>) attributes {passthrough = ["nounwind"]} {
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    scf.for %arg3 = %c0 to %c1024 step %c1 {
      %0 = memref.load %arg0[%arg3] : memref<1024xi32>
      %1 = memref.load %arg1[%arg3] : memref<1024xi32>
      %2 = arith.addi %0, %1 : i32
      memref.store %2, %arg2[%arg3] : memref<1024xi32>
    }
    return
  }
  func.func @dsl.somavet.2(%arg0: memref<1024xi32>) -> i32 attributes {passthrough = ["nounwind"]} {
    %c0_i32 = arith.constant 0 : i32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c1 = arith.constant 1 : index
    %0 = scf.for %arg1 = %c0 to %c1024 step %c1 iter_args(%arg2 = %c0_i32) -> (i32) {
      %1 = memref.load %arg0[%arg1] : memref<1024xi32>
      %2 = arith.addi %arg2, %1 : i32
      scf.yield %2 : i32
    }
    return %0 : i32
  }
}


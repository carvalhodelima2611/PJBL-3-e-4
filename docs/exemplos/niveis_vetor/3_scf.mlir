module {
  func.func @dsl.soma.1(%arg0: memref<1024xi32>, %arg1: memref<1024xi32>, %arg2: memref<1024xi32>) attributes {passthrough = ["nounwind"]} {
    %c0_i32 = arith.constant 0 : i32
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c8 = arith.constant 8 : index
    scf.for %arg3 = %c0 to %c1024 step %c8 {
      %0 = vector.transfer_read %arg0[%arg3], %c0_i32 : memref<1024xi32>, vector<8xi32>
      %1 = vector.transfer_read %arg1[%arg3], %c0_i32 : memref<1024xi32>, vector<8xi32>
      %2 = arith.addi %0, %1 : vector<8xi32>
      vector.transfer_write %2, %arg2[%arg3] : vector<8xi32>, memref<1024xi32>
    }
    return
  }
  func.func @dsl.somavet.2(%arg0: memref<1024xi32>) -> i32 attributes {passthrough = ["nounwind"]} {
    %c0_i32 = arith.constant 0 : i32
    %cst = arith.constant dense<0> : vector<8xi32>
    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c8 = arith.constant 8 : index
    %0 = scf.for %arg1 = %c0 to %c1024 step %c8 iter_args(%arg2 = %cst) -> (vector<8xi32>) {
      %2 = vector.transfer_read %arg0[%arg1], %c0_i32 : memref<1024xi32>, vector<8xi32>
      %3 = arith.addi %arg2, %2 : vector<8xi32>
      scf.yield %3 : vector<8xi32>
    }
    %1 = vector.reduction <add>, %0 : vector<8xi32> into i32
    return %1 : i32
  }
}


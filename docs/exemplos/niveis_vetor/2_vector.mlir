module {
  func.func @dsl.soma.1(%arg0: memref<1024xi32>, %arg1: memref<1024xi32>, %arg2: memref<1024xi32>) attributes {passthrough = ["nounwind"]} {
    %c0_i32 = arith.constant 0 : i32
    affine.for %arg3 = 0 to 1024 step 8 {
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
    %0 = affine.for %arg1 = 0 to 1024 step 8 iter_args(%arg2 = %cst) -> (vector<8xi32>) {
      %2 = vector.transfer_read %arg0[%arg1], %c0_i32 : memref<1024xi32>, vector<8xi32>
      %3 = arith.addi %arg2, %2 : vector<8xi32>
      affine.yield %3 : vector<8xi32>
    }
    %1 = vector.reduction <add>, %0 : vector<8xi32> into i32
    return %1 : i32
  }
}


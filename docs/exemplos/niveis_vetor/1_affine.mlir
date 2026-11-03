// Nucleos vetoriais gerados pelo compilador Macaronica a partir de 'docs/exemplos/nucleo1024.mac'
// Rebaixar com scripts/pipeline.sh (affine -> scf/cf -> dialeto llvm -> LLVM IR)
module {
  // nucleo 1: elemento a elemento '+' com 1024 elementos
  func.func @dsl.soma.1(%a: memref<1024xi32>, %b: memref<1024xi32>, %c: memref<1024xi32>) attributes {passthrough = ["nounwind"]} {
    affine.for %i = 0 to 1024 {
      %x = affine.load %a[%i] : memref<1024xi32>
      %y = affine.load %b[%i] : memref<1024xi32>
      %r = arith.addi %x, %y : i32
      affine.store %r, %c[%i] : memref<1024xi32>
    }
    return
  }

  // nucleo 2: soma de 1024 elementos
  func.func @dsl.somavet.2(%a: memref<1024xi32>) -> i32 attributes {passthrough = ["nounwind"]} {
    %zero = arith.constant 0 : i32
    %s = affine.for %i = 0 to 1024 iter_args(%acc = %zero) -> (i32) {
      %x = affine.load %a[%i] : memref<1024xi32>
      %novo = arith.addi %acc, %x : i32
      affine.yield %novo : i32
    }
    return %s : i32
  }

}

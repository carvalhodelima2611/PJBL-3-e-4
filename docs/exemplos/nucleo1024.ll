; Modulo gerado pelo compilador Macaronica a partir de 'docs/exemplos/nucleo1024.mac'
; IR portavel: sem target triple/datalayout (o alvo e escolhido no llc/clang)

@g_a = global [1024 x i32] zeroinitializer   ; linha 2: global a
@g_b = global [1024 x i32] zeroinitializer   ; linha 2: global b
@g_c = global [1024 x i32] zeroinitializer   ; linha 2: global c

; linha 4: funcao principal
define internal i32 @mac_principal() #0 {
entry:
  br label %corpo
corpo:
  ; linha 6
  call void @dsl.soma.1(ptr @g_a, ptr @g_b, ptr @g_c)   ; operacao vetorial (nucleo MLIR)
  ; linha 7
  %t.1 = call i32 @dsl.somavet.2(ptr @g_c)   ; reducao (nucleo MLIR)
  call void @zl_imprime_int(i32 %t.1)
  call void @zl_imprime_nl()
  ; linha 8
  ret i32 0
}

; ponto de entrada chamado pelo runtime (main/setup ficam em runtime/zl_runtime_*.c)
define i32 @zl_principal() #0 {
entry:
  %r = call i32 @mac_principal()
  ret i32 %r
}

; biblioteca de execucao (runtime/zl_runtime.h): so funcoes de assinatura fixa
declare void @zl_imprime_int(i32) #0
declare void @zl_imprime_flut(float) #0
declare void @zl_imprime_duplo(double) #0
declare void @zl_imprime_bool(i32) #0
declare void @zl_imprime_texto(ptr) #0
declare void @zl_imprime_nl() #0
declare i32 @zl_le_int() #0
declare float @zl_le_flut() #0
declare double @zl_le_duplo() #0
declare void @zl_erro_execucao(i32, i32) #1
declare i32 @zl_pot_inteira(i32, i32) #0
declare void @zl_pino_modo(i32, i32) #0
declare void @zl_pino_escreve(i32, i32) #0
declare i32 @zl_pino_le(i32) #0
declare void @zl_espera(i32) #0
declare double @llvm.sqrt.f64(double)
declare float @llvm.sqrt.f32(float)
declare double @llvm.pow.f64(double, double)
declare float @llvm.pow.f32(float, float)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1)

; nucleos vetoriais da DSL: definidos no arquivo .mlir (ligados com llvm-link)
declare void @dsl.soma.1(ptr, ptr, ptr) #0
declare i32 @dsl.somavet.2(ptr) #0

attributes #0 = { nounwind }
attributes #1 = { nounwind noreturn }

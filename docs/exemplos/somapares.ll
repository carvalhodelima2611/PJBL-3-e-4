; Modulo gerado pelo compilador Macaronica a partir de 'docs/exemplos/somapares.mac'
; IR portavel: sem target triple/datalayout (o alvo e escolhido no llc/clang)

; linha 2: outrafuncao somapares
define i32 @mac_somapares(i32 %n) #0 {
entry:
  %n.addr = alloca i32
  store i32 %n, ptr %n.addr
  %i.addr = alloca i32   ; i (linha 4)
  %s.addr = alloca i32   ; s (linha 4)
  br label %corpo
corpo:
  ; linha 4
  store i32 0, ptr %i.addr
  store i32 0, ptr %s.addr
  ; linha 5
  store i32 0, ptr %s.addr
  ; linha 6
  store i32 0, ptr %i.addr
  br label %repete.cond.1
repete.cond.1:
  %t.1 = load i32, ptr %i.addr
  %t.2 = load i32, ptr %n.addr
  %t.3 = icmp slt i32 %t.1, %t.2
  br i1 %t.3, label %repete.corpo.1, label %repete.fim.1
repete.corpo.1:
  ; linha 8
  %t.4 = load i32, ptr %i.addr
  %t.5 = sdiv i32 %t.4, 2
  %t.6 = mul i32 %t.5, 2
  %t.7 = load i32, ptr %i.addr
  %t.8 = icmp eq i32 %t.6, %t.7
  br i1 %t.8, label %cond.entao.2, label %cond.fim.2
cond.entao.2:
  ; linha 10
  %t.9 = load i32, ptr %s.addr
  %t.10 = load i32, ptr %i.addr
  %t.11 = add i32 %t.9, %t.10
  store i32 %t.11, ptr %s.addr
  br label %cond.fim.2
cond.fim.2:
  br label %repete.inc.1
repete.inc.1:
  ; linha 6
  %t.12 = load i32, ptr %i.addr
  %t.13 = add i32 %t.12, 1
  store i32 %t.13, ptr %i.addr
  br label %repete.cond.1
repete.fim.1:
  ; linha 13
  %t.14 = load i32, ptr %s.addr
  ret i32 %t.14
}

; linha 16: funcao principal
define internal i32 @mac_principal() #0 {
entry:
  br label %corpo
corpo:
  ; linha 18
  %t.1 = call i32 @mac_somapares(i32 10)
  call void @zl_imprime_int(i32 %t.1)
  call void @zl_imprime_nl()
  ; linha 19
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

attributes #0 = { nounwind }
attributes #1 = { nounwind noreturn }

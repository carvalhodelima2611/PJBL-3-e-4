; ModuleID = 'docs/exemplos/somapares.ll'
source_filename = "docs/exemplos/somapares.ll"

; Function Attrs: nounwind
define i32 @mac_somapares(i32 %n) #0 {
entry:
  br label %corpo

corpo:                                            ; preds = %entry
  br label %repete.cond.1

repete.cond.1:                                    ; preds = %repete.inc.1, %corpo
  %i.addr.0 = phi i32 [ 0, %corpo ], [ %t.13, %repete.inc.1 ]
  %s.addr.0 = phi i32 [ 0, %corpo ], [ %s.addr.1, %repete.inc.1 ]
  %t.3 = icmp slt i32 %i.addr.0, %n
  br i1 %t.3, label %repete.corpo.1, label %repete.fim.1

repete.corpo.1:                                   ; preds = %repete.cond.1
  %t.5 = sdiv i32 %i.addr.0, 2
  %t.6 = mul i32 %t.5, 2
  %t.8 = icmp eq i32 %t.6, %i.addr.0
  br i1 %t.8, label %cond.entao.2, label %cond.fim.2

cond.entao.2:                                     ; preds = %repete.corpo.1
  %t.11 = add i32 %s.addr.0, %i.addr.0
  br label %cond.fim.2

cond.fim.2:                                       ; preds = %cond.entao.2, %repete.corpo.1
  %s.addr.1 = phi i32 [ %t.11, %cond.entao.2 ], [ %s.addr.0, %repete.corpo.1 ]
  br label %repete.inc.1

repete.inc.1:                                     ; preds = %cond.fim.2
  %t.13 = add i32 %i.addr.0, 1
  br label %repete.cond.1

repete.fim.1:                                     ; preds = %repete.cond.1
  ret i32 %s.addr.0
}

; Function Attrs: nounwind
define internal i32 @mac_principal() #0 {
entry:
  br label %corpo

corpo:                                            ; preds = %entry
  %t.1 = call i32 @mac_somapares(i32 10)
  call void @zl_imprime_int(i32 %t.1)
  call void @zl_imprime_nl()
  ret i32 0
}

; Function Attrs: nounwind
define i32 @zl_principal() #0 {
entry:
  %r = call i32 @mac_principal()
  ret i32 %r
}

; Function Attrs: nounwind
declare void @zl_imprime_int(i32) #0

; Function Attrs: nounwind
declare void @zl_imprime_flut(float) #0

; Function Attrs: nounwind
declare void @zl_imprime_duplo(double) #0

; Function Attrs: nounwind
declare void @zl_imprime_bool(i32) #0

; Function Attrs: nounwind
declare void @zl_imprime_texto(ptr) #0

; Function Attrs: nounwind
declare void @zl_imprime_nl() #0

; Function Attrs: nounwind
declare i32 @zl_le_int() #0

; Function Attrs: nounwind
declare float @zl_le_flut() #0

; Function Attrs: nounwind
declare double @zl_le_duplo() #0

; Function Attrs: noreturn nounwind
declare void @zl_erro_execucao(i32, i32) #1

; Function Attrs: nounwind
declare i32 @zl_pot_inteira(i32, i32) #0

; Function Attrs: nounwind
declare void @zl_pino_modo(i32, i32) #0

; Function Attrs: nounwind
declare void @zl_pino_escreve(i32, i32) #0

; Function Attrs: nounwind
declare i32 @zl_pino_le(i32) #0

; Function Attrs: nounwind
declare void @zl_espera(i32) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.sqrt.f64(double) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.sqrt.f32(float) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.pow.f64(double, double) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.pow.f32(float, float) #2

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i32(ptr nocapture writeonly, i8, i32, i1 immarg) #3

attributes #0 = { nounwind }
attributes #1 = { noreturn nounwind }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #3 = { nocallback nofree nounwind willreturn memory(argmem: write) }

; ModuleID = 'docs/exemplos/somapares.ll'
source_filename = "docs/exemplos/somapares.ll"

; Function Attrs: nofree norecurse nosync nounwind memory(none)
define i32 @mac_somapares(i32 %n) local_unnamed_addr #0 {
entry:
  %t.37 = icmp sgt i32 %n, 0
  br i1 %t.37, label %repete.corpo.1, label %repete.fim.1

repete.corpo.1:                                   ; preds = %entry, %repete.corpo.1
  %s.addr.09 = phi i32 [ %spec.select, %repete.corpo.1 ], [ 0, %entry ]
  %i.addr.08 = phi i32 [ %t.13, %repete.corpo.1 ], [ 0, %entry ]
  %0 = and i32 %i.addr.08, 1
  %t.8 = icmp eq i32 %0, 0
  %t.11 = select i1 %t.8, i32 %i.addr.08, i32 0
  %spec.select = add i32 %t.11, %s.addr.09
  %t.13 = add nuw nsw i32 %i.addr.08, 1
  %t.3 = icmp slt i32 %t.13, %n
  br i1 %t.3, label %repete.corpo.1, label %repete.fim.1

repete.fim.1:                                     ; preds = %repete.corpo.1, %entry
  %s.addr.0.lcssa = phi i32 [ 0, %entry ], [ %spec.select, %repete.corpo.1 ]
  ret i32 %s.addr.0.lcssa
}

; Function Attrs: nounwind
define noundef i32 @zl_principal() local_unnamed_addr #1 {
entry:
  tail call void @zl_imprime_int(i32 20)
  tail call void @zl_imprime_nl()
  ret i32 0
}

; Function Attrs: nounwind
declare void @zl_imprime_int(i32) local_unnamed_addr #1

; Function Attrs: nounwind
declare void @zl_imprime_nl() local_unnamed_addr #1

attributes #0 = { nofree norecurse nosync nounwind memory(none) }
attributes #1 = { nounwind }

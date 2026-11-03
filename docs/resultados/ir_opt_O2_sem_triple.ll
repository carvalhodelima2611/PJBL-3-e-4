; ModuleID = '/home/claude/w/macaronica/build/bench/10000000/nucleos_C0.ll'
source_filename = "LLVMDialectModule"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.1(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %3, %4
  %5 = phi i64 [ 0, %3 ], [ %12, %4 ]
  %6 = getelementptr i32, ptr %0, i64 %5
  %7 = load i32, ptr %6, align 4
  %8 = getelementptr i32, ptr %1, i64 %5
  %9 = load i32, ptr %8, align 4
  %10 = add i32 %9, %7
  %11 = getelementptr i32, ptr %2, i64 %5
  store i32 %10, ptr %11, align 4
  %12 = add nuw nsw i64 %5, 1
  %13 = icmp ult i64 %5, 9999999
  br i1 %13, label %4, label %14

14:                                               ; preds = %4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i32 @dsl.somavet.2(ptr nocapture readonly %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %1, %2
  %3 = phi i32 [ 0, %1 ], [ %7, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %8, %2 ]
  %5 = getelementptr i32, ptr %0, i64 %4
  %6 = load i32, ptr %5, align 4
  %7 = add i32 %6, %3
  %8 = add nuw nsw i64 %4, 1
  %9 = icmp ult i64 %4, 9999999
  br i1 %9, label %2, label %10

10:                                               ; preds = %2
  ret i32 %7
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.3(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %3, %4
  %5 = phi i64 [ 0, %3 ], [ %12, %4 ]
  %6 = getelementptr float, ptr %0, i64 %5
  %7 = load float, ptr %6, align 4
  %8 = getelementptr float, ptr %1, i64 %5
  %9 = load float, ptr %8, align 4
  %10 = fadd float %7, %9
  %11 = getelementptr float, ptr %2, i64 %5
  store float %10, ptr %11, align 4
  %12 = add nuw nsw i64 %5, 1
  %13 = icmp ult i64 %5, 9999999
  br i1 %13, label %4, label %14

14:                                               ; preds = %4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.somavet.4(ptr nocapture readonly %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %1, %2
  %3 = phi float [ 0.000000e+00, %1 ], [ %7, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %8, %2 ]
  %5 = getelementptr float, ptr %0, i64 %4
  %6 = load float, ptr %5, align 4
  %7 = fadd float %3, %6
  %8 = add nuw nsw i64 %4, 1
  %9 = icmp ult i64 %4, 9999999
  br i1 %9, label %2, label %10

10:                                               ; preds = %2
  ret float %7
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.prodesc.5(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #1 {
  br label %3

3:                                                ; preds = %2, %3
  %4 = phi float [ 0.000000e+00, %2 ], [ %11, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %12, %3 ]
  %6 = getelementptr float, ptr %0, i64 %5
  %7 = load float, ptr %6, align 4
  %8 = getelementptr float, ptr %1, i64 %5
  %9 = load float, ptr %8, align 4
  %10 = fmul float %7, %9
  %11 = fadd float %4, %10
  %12 = add nuw nsw i64 %5, 1
  %13 = icmp ult i64 %5, 9999999
  br i1 %13, label %3, label %14

14:                                               ; preds = %3
  ret float %11
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { nofree norecurse nosync nounwind memory(argmem: read) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}

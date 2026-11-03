; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

; Function Attrs: nounwind
define void @dsl.soma.1(ptr %0, ptr %1, ptr %2) #0 {
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %0, 0
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, ptr %0, 1
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 0, 2
  %7 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, i64 1024, 3, 0
  %8 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %7, i64 1, 4, 0
  %9 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %1, 0
  %10 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %9, ptr %1, 1
  %11 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %10, i64 0, 2
  %12 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %11, i64 1024, 3, 0
  %13 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %12, i64 1, 4, 0
  %14 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %2, 0
  %15 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %14, ptr %2, 1
  %16 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %15, i64 0, 2
  %17 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %16, i64 1024, 3, 0
  %18 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %17, i64 1, 4, 0
  br label %19

19:                                               ; preds = %22, %3
  %20 = phi i64 [ %47, %22 ], [ 0, %3 ]
  %21 = icmp slt i64 %20, 1024
  br i1 %21, label %22, label %48

22:                                               ; preds = %19
  %23 = sub i64 1024, %20
  %24 = trunc i64 %23 to i32
  %25 = insertelement <8 x i32> undef, i32 %24, i32 0
  %26 = shufflevector <8 x i32> %25, <8 x i32> undef, <8 x i32> zeroinitializer
  %27 = icmp sgt <8 x i32> %26, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %28 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %8, 1
  %29 = getelementptr i32, ptr %28, i64 %20
  %30 = call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %29, i32 4, <8 x i1> %27, <8 x i32> zeroinitializer)
  %31 = sub i64 1024, %20
  %32 = trunc i64 %31 to i32
  %33 = insertelement <8 x i32> undef, i32 %32, i32 0
  %34 = shufflevector <8 x i32> %33, <8 x i32> undef, <8 x i32> zeroinitializer
  %35 = icmp sgt <8 x i32> %34, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %36 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %13, 1
  %37 = getelementptr i32, ptr %36, i64 %20
  %38 = call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %37, i32 4, <8 x i1> %35, <8 x i32> zeroinitializer)
  %39 = add <8 x i32> %30, %38
  %40 = sub i64 1024, %20
  %41 = trunc i64 %40 to i32
  %42 = insertelement <8 x i32> undef, i32 %41, i32 0
  %43 = shufflevector <8 x i32> %42, <8 x i32> undef, <8 x i32> zeroinitializer
  %44 = icmp sgt <8 x i32> %43, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %45 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %18, 1
  %46 = getelementptr i32, ptr %45, i64 %20
  call void @llvm.masked.store.v8i32.p0(<8 x i32> %39, ptr %46, i32 4, <8 x i1> %44)
  %47 = add i64 %20, 8
  br label %19

48:                                               ; preds = %19
  ret void
}

; Function Attrs: nounwind
define i32 @dsl.somavet.2(ptr %0) #0 {
  %2 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } undef, ptr %0, 0
  %3 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %2, ptr %0, 1
  %4 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %3, i64 0, 2
  %5 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %4, i64 1024, 3, 0
  %6 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %5, i64 1, 4, 0
  br label %7

7:                                                ; preds = %11, %1
  %8 = phi i64 [ %21, %11 ], [ 0, %1 ]
  %9 = phi <8 x i32> [ %20, %11 ], [ zeroinitializer, %1 ]
  %10 = icmp slt i64 %8, 1024
  br i1 %10, label %11, label %22

11:                                               ; preds = %7
  %12 = sub i64 1024, %8
  %13 = trunc i64 %12 to i32
  %14 = insertelement <8 x i32> undef, i32 %13, i32 0
  %15 = shufflevector <8 x i32> %14, <8 x i32> undef, <8 x i32> zeroinitializer
  %16 = icmp sgt <8 x i32> %15, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %17 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %6, 1
  %18 = getelementptr i32, ptr %17, i64 %8
  %19 = call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %18, i32 4, <8 x i1> %16, <8 x i32> zeroinitializer)
  %20 = add <8 x i32> %9, %19
  %21 = add i64 %8, 8
  br label %7

22:                                               ; preds = %7
  %23 = call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %9)
  ret i32 %23
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i32> @llvm.masked.load.v8i32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i32>) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: write)
declare void @llvm.masked.store.v8i32.p0(<8 x i32>, ptr nocapture, i32 immarg, <8 x i1>) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #3

attributes #0 = { nounwind }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: read) }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(argmem: write) }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}

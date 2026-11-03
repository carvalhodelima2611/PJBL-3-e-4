; ModuleID = '/home/claude/w/macaronica/build/bench/10000000/nucleos_C2.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.1(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %65, %4 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 10000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr i32, ptr %0, i64 %5
  %12 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %11, i32 4, <8 x i1> %10, <8 x i32> zeroinitializer)
  %13 = getelementptr i32, ptr %1, i64 %5
  %14 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %13, i32 4, <8 x i1> %10, <8 x i32> zeroinitializer)
  %15 = add <8 x i32> %14, %12
  %16 = getelementptr i32, ptr %2, i64 %5
  tail call void @llvm.masked.store.v8i32.p0(<8 x i32> %15, ptr %16, i32 4, <8 x i1> %10)
  %17 = add nuw nsw i64 %5, 8
  %18 = trunc i64 %17 to i32
  %19 = sub nuw nsw i32 10000000, %18
  %20 = insertelement <8 x i32> poison, i32 %19, i64 0
  %21 = shufflevector <8 x i32> %20, <8 x i32> poison, <8 x i32> zeroinitializer
  %22 = icmp sgt <8 x i32> %21, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %23 = getelementptr i32, ptr %0, i64 %17
  %24 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %23, i32 4, <8 x i1> %22, <8 x i32> zeroinitializer)
  %25 = getelementptr i32, ptr %1, i64 %17
  %26 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %25, i32 4, <8 x i1> %22, <8 x i32> zeroinitializer)
  %27 = add <8 x i32> %26, %24
  %28 = getelementptr i32, ptr %2, i64 %17
  tail call void @llvm.masked.store.v8i32.p0(<8 x i32> %27, ptr %28, i32 4, <8 x i1> %22)
  %29 = add nuw nsw i64 %5, 16
  %30 = trunc i64 %29 to i32
  %31 = sub nuw nsw i32 10000000, %30
  %32 = insertelement <8 x i32> poison, i32 %31, i64 0
  %33 = shufflevector <8 x i32> %32, <8 x i32> poison, <8 x i32> zeroinitializer
  %34 = icmp sgt <8 x i32> %33, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %35 = getelementptr i32, ptr %0, i64 %29
  %36 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %35, i32 4, <8 x i1> %34, <8 x i32> zeroinitializer)
  %37 = getelementptr i32, ptr %1, i64 %29
  %38 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %37, i32 4, <8 x i1> %34, <8 x i32> zeroinitializer)
  %39 = add <8 x i32> %38, %36
  %40 = getelementptr i32, ptr %2, i64 %29
  tail call void @llvm.masked.store.v8i32.p0(<8 x i32> %39, ptr %40, i32 4, <8 x i1> %34)
  %41 = add nuw nsw i64 %5, 24
  %42 = trunc i64 %41 to i32
  %43 = sub nuw nsw i32 10000000, %42
  %44 = insertelement <8 x i32> poison, i32 %43, i64 0
  %45 = shufflevector <8 x i32> %44, <8 x i32> poison, <8 x i32> zeroinitializer
  %46 = icmp sgt <8 x i32> %45, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %47 = getelementptr i32, ptr %0, i64 %41
  %48 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %47, i32 4, <8 x i1> %46, <8 x i32> zeroinitializer)
  %49 = getelementptr i32, ptr %1, i64 %41
  %50 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %49, i32 4, <8 x i1> %46, <8 x i32> zeroinitializer)
  %51 = add <8 x i32> %50, %48
  %52 = getelementptr i32, ptr %2, i64 %41
  tail call void @llvm.masked.store.v8i32.p0(<8 x i32> %51, ptr %52, i32 4, <8 x i1> %46)
  %53 = add nuw nsw i64 %5, 32
  %54 = trunc i64 %53 to i32
  %55 = sub nuw nsw i32 10000000, %54
  %56 = insertelement <8 x i32> poison, i32 %55, i64 0
  %57 = shufflevector <8 x i32> %56, <8 x i32> poison, <8 x i32> zeroinitializer
  %58 = icmp sgt <8 x i32> %57, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %59 = getelementptr i32, ptr %0, i64 %53
  %60 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %59, i32 4, <8 x i1> %58, <8 x i32> zeroinitializer)
  %61 = getelementptr i32, ptr %1, i64 %53
  %62 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %61, i32 4, <8 x i1> %58, <8 x i32> zeroinitializer)
  %63 = add <8 x i32> %62, %60
  %64 = getelementptr i32, ptr %2, i64 %53
  tail call void @llvm.masked.store.v8i32.p0(<8 x i32> %63, ptr %64, i32 4, <8 x i1> %58)
  %65 = add nuw nsw i64 %5, 40
  %66 = icmp ult i64 %5, 9999960
  br i1 %66, label %4, label %67

67:                                               ; preds = %4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i32 @dsl.somavet.2(ptr nocapture readonly %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi <8 x i32> [ zeroinitializer, %1 ], [ %93, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %94, %2 ]
  %5 = trunc i64 %4 to i32
  %6 = sub nuw nsw i32 10000000, %5
  %7 = insertelement <8 x i32> poison, i32 %6, i64 0
  %8 = shufflevector <8 x i32> %7, <8 x i32> poison, <8 x i32> zeroinitializer
  %9 = icmp sgt <8 x i32> %8, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %10 = getelementptr i32, ptr %0, i64 %4
  %11 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %10, i32 4, <8 x i1> %9, <8 x i32> zeroinitializer)
  %12 = add <8 x i32> %11, %3
  %13 = or disjoint i64 %4, 8
  %14 = trunc i64 %13 to i32
  %15 = sub nuw nsw i32 10000000, %14
  %16 = insertelement <8 x i32> poison, i32 %15, i64 0
  %17 = shufflevector <8 x i32> %16, <8 x i32> poison, <8 x i32> zeroinitializer
  %18 = icmp sgt <8 x i32> %17, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %19 = getelementptr i32, ptr %0, i64 %13
  %20 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %19, i32 4, <8 x i1> %18, <8 x i32> zeroinitializer)
  %21 = add <8 x i32> %20, %12
  %22 = add nuw nsw i64 %4, 16
  %23 = trunc i64 %22 to i32
  %24 = sub nuw nsw i32 10000000, %23
  %25 = insertelement <8 x i32> poison, i32 %24, i64 0
  %26 = shufflevector <8 x i32> %25, <8 x i32> poison, <8 x i32> zeroinitializer
  %27 = icmp sgt <8 x i32> %26, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %28 = getelementptr i32, ptr %0, i64 %22
  %29 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %28, i32 4, <8 x i1> %27, <8 x i32> zeroinitializer)
  %30 = add <8 x i32> %29, %21
  %31 = add nuw nsw i64 %4, 24
  %32 = trunc i64 %31 to i32
  %33 = sub nuw nsw i32 10000000, %32
  %34 = insertelement <8 x i32> poison, i32 %33, i64 0
  %35 = shufflevector <8 x i32> %34, <8 x i32> poison, <8 x i32> zeroinitializer
  %36 = icmp sgt <8 x i32> %35, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %37 = getelementptr i32, ptr %0, i64 %31
  %38 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %37, i32 4, <8 x i1> %36, <8 x i32> zeroinitializer)
  %39 = add <8 x i32> %38, %30
  %40 = add nuw nsw i64 %4, 32
  %41 = trunc i64 %40 to i32
  %42 = sub nuw nsw i32 10000000, %41
  %43 = insertelement <8 x i32> poison, i32 %42, i64 0
  %44 = shufflevector <8 x i32> %43, <8 x i32> poison, <8 x i32> zeroinitializer
  %45 = icmp sgt <8 x i32> %44, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %46 = getelementptr i32, ptr %0, i64 %40
  %47 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %46, i32 4, <8 x i1> %45, <8 x i32> zeroinitializer)
  %48 = add <8 x i32> %47, %39
  %49 = add nuw nsw i64 %4, 40
  %50 = trunc i64 %49 to i32
  %51 = sub nuw nsw i32 10000000, %50
  %52 = insertelement <8 x i32> poison, i32 %51, i64 0
  %53 = shufflevector <8 x i32> %52, <8 x i32> poison, <8 x i32> zeroinitializer
  %54 = icmp sgt <8 x i32> %53, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %55 = getelementptr i32, ptr %0, i64 %49
  %56 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %55, i32 4, <8 x i1> %54, <8 x i32> zeroinitializer)
  %57 = add <8 x i32> %56, %48
  %58 = add nuw nsw i64 %4, 48
  %59 = trunc i64 %58 to i32
  %60 = sub nuw nsw i32 10000000, %59
  %61 = insertelement <8 x i32> poison, i32 %60, i64 0
  %62 = shufflevector <8 x i32> %61, <8 x i32> poison, <8 x i32> zeroinitializer
  %63 = icmp sgt <8 x i32> %62, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %64 = getelementptr i32, ptr %0, i64 %58
  %65 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %64, i32 4, <8 x i1> %63, <8 x i32> zeroinitializer)
  %66 = add <8 x i32> %65, %57
  %67 = add nuw nsw i64 %4, 56
  %68 = trunc i64 %67 to i32
  %69 = sub nuw nsw i32 10000000, %68
  %70 = insertelement <8 x i32> poison, i32 %69, i64 0
  %71 = shufflevector <8 x i32> %70, <8 x i32> poison, <8 x i32> zeroinitializer
  %72 = icmp sgt <8 x i32> %71, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %73 = getelementptr i32, ptr %0, i64 %67
  %74 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %73, i32 4, <8 x i1> %72, <8 x i32> zeroinitializer)
  %75 = add <8 x i32> %74, %66
  %76 = add nuw nsw i64 %4, 64
  %77 = trunc i64 %76 to i32
  %78 = sub nuw nsw i32 10000000, %77
  %79 = insertelement <8 x i32> poison, i32 %78, i64 0
  %80 = shufflevector <8 x i32> %79, <8 x i32> poison, <8 x i32> zeroinitializer
  %81 = icmp sgt <8 x i32> %80, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %82 = getelementptr i32, ptr %0, i64 %76
  %83 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %82, i32 4, <8 x i1> %81, <8 x i32> zeroinitializer)
  %84 = add <8 x i32> %83, %75
  %85 = add nuw nsw i64 %4, 72
  %86 = trunc i64 %85 to i32
  %87 = sub nuw nsw i32 10000000, %86
  %88 = insertelement <8 x i32> poison, i32 %87, i64 0
  %89 = shufflevector <8 x i32> %88, <8 x i32> poison, <8 x i32> zeroinitializer
  %90 = icmp sgt <8 x i32> %89, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %91 = getelementptr i32, ptr %0, i64 %85
  %92 = tail call <8 x i32> @llvm.masked.load.v8i32.p0(ptr %91, i32 4, <8 x i1> %90, <8 x i32> zeroinitializer)
  %93 = add <8 x i32> %92, %84
  %94 = add nuw nsw i64 %4, 80
  %95 = icmp ult i64 %4, 9999920
  br i1 %95, label %2, label %96

96:                                               ; preds = %2
  %97 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %93)
  ret i32 %97
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.3(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %4, %3
  %5 = phi i64 [ 0, %3 ], [ %65, %4 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 10000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr float, ptr %0, i64 %5
  %12 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %11, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %13 = getelementptr float, ptr %1, i64 %5
  %14 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %13, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %15 = fadd <8 x float> %12, %14
  %16 = getelementptr float, ptr %2, i64 %5
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %15, ptr %16, i32 4, <8 x i1> %10)
  %17 = add nuw nsw i64 %5, 8
  %18 = trunc i64 %17 to i32
  %19 = sub nuw nsw i32 10000000, %18
  %20 = insertelement <8 x i32> poison, i32 %19, i64 0
  %21 = shufflevector <8 x i32> %20, <8 x i32> poison, <8 x i32> zeroinitializer
  %22 = icmp sgt <8 x i32> %21, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %23 = getelementptr float, ptr %0, i64 %17
  %24 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %23, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %25 = getelementptr float, ptr %1, i64 %17
  %26 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %25, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %27 = fadd <8 x float> %24, %26
  %28 = getelementptr float, ptr %2, i64 %17
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %27, ptr %28, i32 4, <8 x i1> %22)
  %29 = add nuw nsw i64 %5, 16
  %30 = trunc i64 %29 to i32
  %31 = sub nuw nsw i32 10000000, %30
  %32 = insertelement <8 x i32> poison, i32 %31, i64 0
  %33 = shufflevector <8 x i32> %32, <8 x i32> poison, <8 x i32> zeroinitializer
  %34 = icmp sgt <8 x i32> %33, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %35 = getelementptr float, ptr %0, i64 %29
  %36 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %35, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %37 = getelementptr float, ptr %1, i64 %29
  %38 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %37, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %39 = fadd <8 x float> %36, %38
  %40 = getelementptr float, ptr %2, i64 %29
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %39, ptr %40, i32 4, <8 x i1> %34)
  %41 = add nuw nsw i64 %5, 24
  %42 = trunc i64 %41 to i32
  %43 = sub nuw nsw i32 10000000, %42
  %44 = insertelement <8 x i32> poison, i32 %43, i64 0
  %45 = shufflevector <8 x i32> %44, <8 x i32> poison, <8 x i32> zeroinitializer
  %46 = icmp sgt <8 x i32> %45, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %47 = getelementptr float, ptr %0, i64 %41
  %48 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %47, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %49 = getelementptr float, ptr %1, i64 %41
  %50 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %49, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %51 = fadd <8 x float> %48, %50
  %52 = getelementptr float, ptr %2, i64 %41
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %51, ptr %52, i32 4, <8 x i1> %46)
  %53 = add nuw nsw i64 %5, 32
  %54 = trunc i64 %53 to i32
  %55 = sub nuw nsw i32 10000000, %54
  %56 = insertelement <8 x i32> poison, i32 %55, i64 0
  %57 = shufflevector <8 x i32> %56, <8 x i32> poison, <8 x i32> zeroinitializer
  %58 = icmp sgt <8 x i32> %57, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %59 = getelementptr float, ptr %0, i64 %53
  %60 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %59, i32 4, <8 x i1> %58, <8 x float> zeroinitializer)
  %61 = getelementptr float, ptr %1, i64 %53
  %62 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %61, i32 4, <8 x i1> %58, <8 x float> zeroinitializer)
  %63 = fadd <8 x float> %60, %62
  %64 = getelementptr float, ptr %2, i64 %53
  tail call void @llvm.masked.store.v8f32.p0(<8 x float> %63, ptr %64, i32 4, <8 x i1> %58)
  %65 = add nuw nsw i64 %5, 40
  %66 = icmp ult i64 %5, 9999960
  br i1 %66, label %4, label %67

67:                                               ; preds = %4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.somavet.4(ptr nocapture readonly %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi <8 x float> [ zeroinitializer, %1 ], [ %93, %2 ]
  %4 = phi i64 [ 0, %1 ], [ %94, %2 ]
  %5 = trunc i64 %4 to i32
  %6 = sub nuw nsw i32 10000000, %5
  %7 = insertelement <8 x i32> poison, i32 %6, i64 0
  %8 = shufflevector <8 x i32> %7, <8 x i32> poison, <8 x i32> zeroinitializer
  %9 = icmp sgt <8 x i32> %8, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %10 = getelementptr float, ptr %0, i64 %4
  %11 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %10, i32 4, <8 x i1> %9, <8 x float> zeroinitializer)
  %12 = fadd <8 x float> %3, %11
  %13 = or disjoint i64 %4, 8
  %14 = trunc i64 %13 to i32
  %15 = sub nuw nsw i32 10000000, %14
  %16 = insertelement <8 x i32> poison, i32 %15, i64 0
  %17 = shufflevector <8 x i32> %16, <8 x i32> poison, <8 x i32> zeroinitializer
  %18 = icmp sgt <8 x i32> %17, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %19 = getelementptr float, ptr %0, i64 %13
  %20 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %19, i32 4, <8 x i1> %18, <8 x float> zeroinitializer)
  %21 = fadd <8 x float> %12, %20
  %22 = add nuw nsw i64 %4, 16
  %23 = trunc i64 %22 to i32
  %24 = sub nuw nsw i32 10000000, %23
  %25 = insertelement <8 x i32> poison, i32 %24, i64 0
  %26 = shufflevector <8 x i32> %25, <8 x i32> poison, <8 x i32> zeroinitializer
  %27 = icmp sgt <8 x i32> %26, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %28 = getelementptr float, ptr %0, i64 %22
  %29 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %28, i32 4, <8 x i1> %27, <8 x float> zeroinitializer)
  %30 = fadd <8 x float> %21, %29
  %31 = add nuw nsw i64 %4, 24
  %32 = trunc i64 %31 to i32
  %33 = sub nuw nsw i32 10000000, %32
  %34 = insertelement <8 x i32> poison, i32 %33, i64 0
  %35 = shufflevector <8 x i32> %34, <8 x i32> poison, <8 x i32> zeroinitializer
  %36 = icmp sgt <8 x i32> %35, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %37 = getelementptr float, ptr %0, i64 %31
  %38 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %37, i32 4, <8 x i1> %36, <8 x float> zeroinitializer)
  %39 = fadd <8 x float> %30, %38
  %40 = add nuw nsw i64 %4, 32
  %41 = trunc i64 %40 to i32
  %42 = sub nuw nsw i32 10000000, %41
  %43 = insertelement <8 x i32> poison, i32 %42, i64 0
  %44 = shufflevector <8 x i32> %43, <8 x i32> poison, <8 x i32> zeroinitializer
  %45 = icmp sgt <8 x i32> %44, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %46 = getelementptr float, ptr %0, i64 %40
  %47 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %46, i32 4, <8 x i1> %45, <8 x float> zeroinitializer)
  %48 = fadd <8 x float> %39, %47
  %49 = add nuw nsw i64 %4, 40
  %50 = trunc i64 %49 to i32
  %51 = sub nuw nsw i32 10000000, %50
  %52 = insertelement <8 x i32> poison, i32 %51, i64 0
  %53 = shufflevector <8 x i32> %52, <8 x i32> poison, <8 x i32> zeroinitializer
  %54 = icmp sgt <8 x i32> %53, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %55 = getelementptr float, ptr %0, i64 %49
  %56 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %55, i32 4, <8 x i1> %54, <8 x float> zeroinitializer)
  %57 = fadd <8 x float> %48, %56
  %58 = add nuw nsw i64 %4, 48
  %59 = trunc i64 %58 to i32
  %60 = sub nuw nsw i32 10000000, %59
  %61 = insertelement <8 x i32> poison, i32 %60, i64 0
  %62 = shufflevector <8 x i32> %61, <8 x i32> poison, <8 x i32> zeroinitializer
  %63 = icmp sgt <8 x i32> %62, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %64 = getelementptr float, ptr %0, i64 %58
  %65 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %64, i32 4, <8 x i1> %63, <8 x float> zeroinitializer)
  %66 = fadd <8 x float> %57, %65
  %67 = add nuw nsw i64 %4, 56
  %68 = trunc i64 %67 to i32
  %69 = sub nuw nsw i32 10000000, %68
  %70 = insertelement <8 x i32> poison, i32 %69, i64 0
  %71 = shufflevector <8 x i32> %70, <8 x i32> poison, <8 x i32> zeroinitializer
  %72 = icmp sgt <8 x i32> %71, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %73 = getelementptr float, ptr %0, i64 %67
  %74 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %73, i32 4, <8 x i1> %72, <8 x float> zeroinitializer)
  %75 = fadd <8 x float> %66, %74
  %76 = add nuw nsw i64 %4, 64
  %77 = trunc i64 %76 to i32
  %78 = sub nuw nsw i32 10000000, %77
  %79 = insertelement <8 x i32> poison, i32 %78, i64 0
  %80 = shufflevector <8 x i32> %79, <8 x i32> poison, <8 x i32> zeroinitializer
  %81 = icmp sgt <8 x i32> %80, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %82 = getelementptr float, ptr %0, i64 %76
  %83 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %82, i32 4, <8 x i1> %81, <8 x float> zeroinitializer)
  %84 = fadd <8 x float> %75, %83
  %85 = add nuw nsw i64 %4, 72
  %86 = trunc i64 %85 to i32
  %87 = sub nuw nsw i32 10000000, %86
  %88 = insertelement <8 x i32> poison, i32 %87, i64 0
  %89 = shufflevector <8 x i32> %88, <8 x i32> poison, <8 x i32> zeroinitializer
  %90 = icmp sgt <8 x i32> %89, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %91 = getelementptr float, ptr %0, i64 %85
  %92 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %91, i32 4, <8 x i1> %90, <8 x float> zeroinitializer)
  %93 = fadd <8 x float> %84, %92
  %94 = add nuw nsw i64 %4, 80
  %95 = icmp ult i64 %4, 9999920
  br i1 %95, label %2, label %96

96:                                               ; preds = %2
  %97 = tail call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %93)
  ret float %97
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.prodesc.5(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #1 {
  br label %3

3:                                                ; preds = %3, %2
  %4 = phi <8 x float> [ zeroinitializer, %2 ], [ %64, %3 ]
  %5 = phi i64 [ 0, %2 ], [ %65, %3 ]
  %6 = trunc i64 %5 to i32
  %7 = sub nuw nsw i32 10000000, %6
  %8 = insertelement <8 x i32> poison, i32 %7, i64 0
  %9 = shufflevector <8 x i32> %8, <8 x i32> poison, <8 x i32> zeroinitializer
  %10 = icmp sgt <8 x i32> %9, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %11 = getelementptr float, ptr %0, i64 %5
  %12 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %11, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %13 = getelementptr float, ptr %1, i64 %5
  %14 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %13, i32 4, <8 x i1> %10, <8 x float> zeroinitializer)
  %15 = fmul <8 x float> %12, %14
  %16 = fadd <8 x float> %4, %15
  %17 = add nuw nsw i64 %5, 8
  %18 = trunc i64 %17 to i32
  %19 = sub nuw nsw i32 10000000, %18
  %20 = insertelement <8 x i32> poison, i32 %19, i64 0
  %21 = shufflevector <8 x i32> %20, <8 x i32> poison, <8 x i32> zeroinitializer
  %22 = icmp sgt <8 x i32> %21, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %23 = getelementptr float, ptr %0, i64 %17
  %24 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %23, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %25 = getelementptr float, ptr %1, i64 %17
  %26 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %25, i32 4, <8 x i1> %22, <8 x float> zeroinitializer)
  %27 = fmul <8 x float> %24, %26
  %28 = fadd <8 x float> %16, %27
  %29 = add nuw nsw i64 %5, 16
  %30 = trunc i64 %29 to i32
  %31 = sub nuw nsw i32 10000000, %30
  %32 = insertelement <8 x i32> poison, i32 %31, i64 0
  %33 = shufflevector <8 x i32> %32, <8 x i32> poison, <8 x i32> zeroinitializer
  %34 = icmp sgt <8 x i32> %33, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %35 = getelementptr float, ptr %0, i64 %29
  %36 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %35, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %37 = getelementptr float, ptr %1, i64 %29
  %38 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %37, i32 4, <8 x i1> %34, <8 x float> zeroinitializer)
  %39 = fmul <8 x float> %36, %38
  %40 = fadd <8 x float> %28, %39
  %41 = add nuw nsw i64 %5, 24
  %42 = trunc i64 %41 to i32
  %43 = sub nuw nsw i32 10000000, %42
  %44 = insertelement <8 x i32> poison, i32 %43, i64 0
  %45 = shufflevector <8 x i32> %44, <8 x i32> poison, <8 x i32> zeroinitializer
  %46 = icmp sgt <8 x i32> %45, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %47 = getelementptr float, ptr %0, i64 %41
  %48 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %47, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %49 = getelementptr float, ptr %1, i64 %41
  %50 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %49, i32 4, <8 x i1> %46, <8 x float> zeroinitializer)
  %51 = fmul <8 x float> %48, %50
  %52 = fadd <8 x float> %40, %51
  %53 = add nuw nsw i64 %5, 32
  %54 = trunc i64 %53 to i32
  %55 = sub nuw nsw i32 10000000, %54
  %56 = insertelement <8 x i32> poison, i32 %55, i64 0
  %57 = shufflevector <8 x i32> %56, <8 x i32> poison, <8 x i32> zeroinitializer
  %58 = icmp sgt <8 x i32> %57, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %59 = getelementptr float, ptr %0, i64 %53
  %60 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %59, i32 4, <8 x i1> %58, <8 x float> zeroinitializer)
  %61 = getelementptr float, ptr %1, i64 %53
  %62 = tail call <8 x float> @llvm.masked.load.v8f32.p0(ptr %61, i32 4, <8 x i1> %58, <8 x float> zeroinitializer)
  %63 = fmul <8 x float> %60, %62
  %64 = fadd <8 x float> %52, %63
  %65 = add nuw nsw i64 %5, 40
  %66 = icmp ult i64 %5, 9999960
  br i1 %66, label %3, label %67

67:                                               ; preds = %3
  %68 = tail call float @llvm.vector.reduce.fadd.v8f32(float 0.000000e+00, <8 x float> %64)
  ret float %68
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x i32> @llvm.masked.load.v8i32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x i32>) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: write)
declare void @llvm.masked.store.v8i32.p0(<8 x i32>, ptr nocapture, i32 immarg, <8 x i1>) #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #4

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read)
declare <8 x float> @llvm.masked.load.v8f32.p0(ptr nocapture, i32 immarg, <8 x i1>, <8 x float>) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: write)
declare void @llvm.masked.store.v8f32.p0(<8 x float>, ptr nocapture, i32 immarg, <8 x i1>) #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #4

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: read) }
attributes #3 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: write) }
attributes #4 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}

; ModuleID = '/home/claude/w/macaronica/build/bench/10000000/nucleos_C1f.ll'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.1(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
vector.memcheck:
  %3 = ptrtoint ptr %2 to i64
  %4 = ptrtoint ptr %0 to i64
  %5 = ptrtoint ptr %1 to i64
  %6 = sub i64 %3, %4
  %diff.check = icmp ult i64 %6, 128
  %7 = sub i64 %3, %5
  %diff.check1 = icmp ult i64 %7, 128
  %conflict.rdx = or i1 %diff.check, %diff.check1
  br i1 %conflict.rdx, label %scalar.ph, label %vector.body

vector.body:                                      ; preds = %vector.memcheck, %vector.body
  %index = phi i64 [ %index.next.1, %vector.body ], [ 0, %vector.memcheck ]
  %8 = getelementptr i32, ptr %0, i64 %index
  %9 = getelementptr i32, ptr %8, i64 8
  %10 = getelementptr i32, ptr %8, i64 16
  %11 = getelementptr i32, ptr %8, i64 24
  %wide.load = load <8 x i32>, ptr %8, align 4
  %wide.load2 = load <8 x i32>, ptr %9, align 4
  %wide.load3 = load <8 x i32>, ptr %10, align 4
  %wide.load4 = load <8 x i32>, ptr %11, align 4
  %12 = getelementptr i32, ptr %1, i64 %index
  %13 = getelementptr i32, ptr %12, i64 8
  %14 = getelementptr i32, ptr %12, i64 16
  %15 = getelementptr i32, ptr %12, i64 24
  %wide.load5 = load <8 x i32>, ptr %12, align 4
  %wide.load6 = load <8 x i32>, ptr %13, align 4
  %wide.load7 = load <8 x i32>, ptr %14, align 4
  %wide.load8 = load <8 x i32>, ptr %15, align 4
  %16 = add <8 x i32> %wide.load5, %wide.load
  %17 = add <8 x i32> %wide.load6, %wide.load2
  %18 = add <8 x i32> %wide.load7, %wide.load3
  %19 = add <8 x i32> %wide.load8, %wide.load4
  %20 = getelementptr i32, ptr %2, i64 %index
  %21 = getelementptr i32, ptr %20, i64 8
  %22 = getelementptr i32, ptr %20, i64 16
  %23 = getelementptr i32, ptr %20, i64 24
  store <8 x i32> %16, ptr %20, align 4
  store <8 x i32> %17, ptr %21, align 4
  store <8 x i32> %18, ptr %22, align 4
  store <8 x i32> %19, ptr %23, align 4
  %index.next = or disjoint i64 %index, 32
  %24 = getelementptr i32, ptr %0, i64 %index.next
  %25 = getelementptr i32, ptr %24, i64 8
  %26 = getelementptr i32, ptr %24, i64 16
  %27 = getelementptr i32, ptr %24, i64 24
  %wide.load.1 = load <8 x i32>, ptr %24, align 4
  %wide.load2.1 = load <8 x i32>, ptr %25, align 4
  %wide.load3.1 = load <8 x i32>, ptr %26, align 4
  %wide.load4.1 = load <8 x i32>, ptr %27, align 4
  %28 = getelementptr i32, ptr %1, i64 %index.next
  %29 = getelementptr i32, ptr %28, i64 8
  %30 = getelementptr i32, ptr %28, i64 16
  %31 = getelementptr i32, ptr %28, i64 24
  %wide.load5.1 = load <8 x i32>, ptr %28, align 4
  %wide.load6.1 = load <8 x i32>, ptr %29, align 4
  %wide.load7.1 = load <8 x i32>, ptr %30, align 4
  %wide.load8.1 = load <8 x i32>, ptr %31, align 4
  %32 = add <8 x i32> %wide.load5.1, %wide.load.1
  %33 = add <8 x i32> %wide.load6.1, %wide.load2.1
  %34 = add <8 x i32> %wide.load7.1, %wide.load3.1
  %35 = add <8 x i32> %wide.load8.1, %wide.load4.1
  %36 = getelementptr i32, ptr %2, i64 %index.next
  %37 = getelementptr i32, ptr %36, i64 8
  %38 = getelementptr i32, ptr %36, i64 16
  %39 = getelementptr i32, ptr %36, i64 24
  store <8 x i32> %32, ptr %36, align 4
  store <8 x i32> %33, ptr %37, align 4
  store <8 x i32> %34, ptr %38, align 4
  store <8 x i32> %35, ptr %39, align 4
  %index.next.1 = add nuw nsw i64 %index, 64
  %40 = icmp eq i64 %index.next.1, 10000000
  br i1 %40, label %middle.block, label %vector.body, !llvm.loop !1

scalar.ph:                                        ; preds = %vector.memcheck, %scalar.ph
  %41 = phi i64 [ %111, %scalar.ph ], [ 0, %vector.memcheck ]
  %42 = getelementptr i32, ptr %0, i64 %41
  %43 = load i32, ptr %42, align 4
  %44 = getelementptr i32, ptr %1, i64 %41
  %45 = load i32, ptr %44, align 4
  %46 = add i32 %45, %43
  %47 = getelementptr i32, ptr %2, i64 %41
  store i32 %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr i32, ptr %0, i64 %48
  %50 = load i32, ptr %49, align 4
  %51 = getelementptr i32, ptr %1, i64 %48
  %52 = load i32, ptr %51, align 4
  %53 = add i32 %52, %50
  %54 = getelementptr i32, ptr %2, i64 %48
  store i32 %53, ptr %54, align 4
  %55 = add nuw nsw i64 %41, 2
  %56 = getelementptr i32, ptr %0, i64 %55
  %57 = load i32, ptr %56, align 4
  %58 = getelementptr i32, ptr %1, i64 %55
  %59 = load i32, ptr %58, align 4
  %60 = add i32 %59, %57
  %61 = getelementptr i32, ptr %2, i64 %55
  store i32 %60, ptr %61, align 4
  %62 = add nuw nsw i64 %41, 3
  %63 = getelementptr i32, ptr %0, i64 %62
  %64 = load i32, ptr %63, align 4
  %65 = getelementptr i32, ptr %1, i64 %62
  %66 = load i32, ptr %65, align 4
  %67 = add i32 %66, %64
  %68 = getelementptr i32, ptr %2, i64 %62
  store i32 %67, ptr %68, align 4
  %69 = add nuw nsw i64 %41, 4
  %70 = getelementptr i32, ptr %0, i64 %69
  %71 = load i32, ptr %70, align 4
  %72 = getelementptr i32, ptr %1, i64 %69
  %73 = load i32, ptr %72, align 4
  %74 = add i32 %73, %71
  %75 = getelementptr i32, ptr %2, i64 %69
  store i32 %74, ptr %75, align 4
  %76 = add nuw nsw i64 %41, 5
  %77 = getelementptr i32, ptr %0, i64 %76
  %78 = load i32, ptr %77, align 4
  %79 = getelementptr i32, ptr %1, i64 %76
  %80 = load i32, ptr %79, align 4
  %81 = add i32 %80, %78
  %82 = getelementptr i32, ptr %2, i64 %76
  store i32 %81, ptr %82, align 4
  %83 = add nuw nsw i64 %41, 6
  %84 = getelementptr i32, ptr %0, i64 %83
  %85 = load i32, ptr %84, align 4
  %86 = getelementptr i32, ptr %1, i64 %83
  %87 = load i32, ptr %86, align 4
  %88 = add i32 %87, %85
  %89 = getelementptr i32, ptr %2, i64 %83
  store i32 %88, ptr %89, align 4
  %90 = add nuw nsw i64 %41, 7
  %91 = getelementptr i32, ptr %0, i64 %90
  %92 = load i32, ptr %91, align 4
  %93 = getelementptr i32, ptr %1, i64 %90
  %94 = load i32, ptr %93, align 4
  %95 = add i32 %94, %92
  %96 = getelementptr i32, ptr %2, i64 %90
  store i32 %95, ptr %96, align 4
  %97 = add nuw nsw i64 %41, 8
  %98 = getelementptr i32, ptr %0, i64 %97
  %99 = load i32, ptr %98, align 4
  %100 = getelementptr i32, ptr %1, i64 %97
  %101 = load i32, ptr %100, align 4
  %102 = add i32 %101, %99
  %103 = getelementptr i32, ptr %2, i64 %97
  store i32 %102, ptr %103, align 4
  %104 = add nuw nsw i64 %41, 9
  %105 = getelementptr i32, ptr %0, i64 %104
  %106 = load i32, ptr %105, align 4
  %107 = getelementptr i32, ptr %1, i64 %104
  %108 = load i32, ptr %107, align 4
  %109 = add i32 %108, %106
  %110 = getelementptr i32, ptr %2, i64 %104
  store i32 %109, ptr %110, align 4
  %111 = add nuw nsw i64 %41, 10
  %exitcond.not.9 = icmp eq i64 %111, 10000000
  br i1 %exitcond.not.9, label %middle.block, label %scalar.ph, !llvm.loop !4

middle.block:                                     ; preds = %vector.body, %scalar.ph
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define i32 @dsl.somavet.2(ptr nocapture readonly %0) local_unnamed_addr #1 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <8 x i32> [ zeroinitializer, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr i32, ptr %0, i64 %index
  %2 = getelementptr i32, ptr %1, i64 8
  %3 = getelementptr i32, ptr %1, i64 16
  %4 = getelementptr i32, ptr %1, i64 24
  %wide.load = load <8 x i32>, ptr %1, align 4
  %wide.load4 = load <8 x i32>, ptr %2, align 4
  %wide.load5 = load <8 x i32>, ptr %3, align 4
  %wide.load6 = load <8 x i32>, ptr %4, align 4
  %5 = add <8 x i32> %wide.load, %vec.phi
  %6 = add <8 x i32> %wide.load4, %vec.phi1
  %7 = add <8 x i32> %wide.load5, %vec.phi2
  %8 = add <8 x i32> %wide.load6, %vec.phi3
  %9 = getelementptr i32, ptr %0, i64 %index
  %10 = getelementptr i32, ptr %9, i64 32
  %11 = getelementptr i32, ptr %9, i64 40
  %12 = getelementptr i32, ptr %9, i64 48
  %13 = getelementptr i32, ptr %9, i64 56
  %wide.load.1 = load <8 x i32>, ptr %10, align 4
  %wide.load4.1 = load <8 x i32>, ptr %11, align 4
  %wide.load5.1 = load <8 x i32>, ptr %12, align 4
  %wide.load6.1 = load <8 x i32>, ptr %13, align 4
  %14 = add <8 x i32> %wide.load.1, %5
  %15 = add <8 x i32> %wide.load4.1, %6
  %16 = add <8 x i32> %wide.load5.1, %7
  %17 = add <8 x i32> %wide.load6.1, %8
  %18 = getelementptr i32, ptr %0, i64 %index
  %19 = getelementptr i32, ptr %18, i64 64
  %20 = getelementptr i32, ptr %18, i64 72
  %21 = getelementptr i32, ptr %18, i64 80
  %22 = getelementptr i32, ptr %18, i64 88
  %wide.load.2 = load <8 x i32>, ptr %19, align 4
  %wide.load4.2 = load <8 x i32>, ptr %20, align 4
  %wide.load5.2 = load <8 x i32>, ptr %21, align 4
  %wide.load6.2 = load <8 x i32>, ptr %22, align 4
  %23 = add <8 x i32> %wide.load.2, %14
  %24 = add <8 x i32> %wide.load4.2, %15
  %25 = add <8 x i32> %wide.load5.2, %16
  %26 = add <8 x i32> %wide.load6.2, %17
  %27 = getelementptr i32, ptr %0, i64 %index
  %28 = getelementptr i32, ptr %27, i64 96
  %29 = getelementptr i32, ptr %27, i64 104
  %30 = getelementptr i32, ptr %27, i64 112
  %31 = getelementptr i32, ptr %27, i64 120
  %wide.load.3 = load <8 x i32>, ptr %28, align 4
  %wide.load4.3 = load <8 x i32>, ptr %29, align 4
  %wide.load5.3 = load <8 x i32>, ptr %30, align 4
  %wide.load6.3 = load <8 x i32>, ptr %31, align 4
  %32 = add <8 x i32> %wide.load.3, %23
  %33 = add <8 x i32> %wide.load4.3, %24
  %34 = add <8 x i32> %wide.load5.3, %25
  %35 = add <8 x i32> %wide.load6.3, %26
  %36 = getelementptr i32, ptr %0, i64 %index
  %37 = getelementptr i32, ptr %36, i64 128
  %38 = getelementptr i32, ptr %36, i64 136
  %39 = getelementptr i32, ptr %36, i64 144
  %40 = getelementptr i32, ptr %36, i64 152
  %wide.load.4 = load <8 x i32>, ptr %37, align 4
  %wide.load4.4 = load <8 x i32>, ptr %38, align 4
  %wide.load5.4 = load <8 x i32>, ptr %39, align 4
  %wide.load6.4 = load <8 x i32>, ptr %40, align 4
  %41 = add <8 x i32> %wide.load.4, %32
  %42 = add <8 x i32> %wide.load4.4, %33
  %43 = add <8 x i32> %wide.load5.4, %34
  %44 = add <8 x i32> %wide.load6.4, %35
  %index.next.4 = add nuw nsw i64 %index, 160
  %45 = icmp eq i64 %index.next.4, 10000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <8 x i32> %42, %41
  %bin.rdx7 = add <8 x i32> %43, %bin.rdx
  %bin.rdx8 = add <8 x i32> %44, %bin.rdx7
  %46 = tail call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %bin.rdx8)
  ret i32 %46
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite)
define void @dsl.soma.3(ptr nocapture readonly %0, ptr nocapture readonly %1, ptr nocapture writeonly %2) local_unnamed_addr #0 {
vector.memcheck:
  %3 = ptrtoint ptr %2 to i64
  %4 = ptrtoint ptr %0 to i64
  %5 = ptrtoint ptr %1 to i64
  %6 = sub i64 %3, %4
  %diff.check = icmp ult i64 %6, 128
  %7 = sub i64 %3, %5
  %diff.check1 = icmp ult i64 %7, 128
  %conflict.rdx = or i1 %diff.check, %diff.check1
  br i1 %conflict.rdx, label %scalar.ph, label %vector.body

vector.body:                                      ; preds = %vector.memcheck, %vector.body
  %index = phi i64 [ %index.next.1, %vector.body ], [ 0, %vector.memcheck ]
  %8 = getelementptr float, ptr %0, i64 %index
  %9 = getelementptr float, ptr %8, i64 8
  %10 = getelementptr float, ptr %8, i64 16
  %11 = getelementptr float, ptr %8, i64 24
  %wide.load = load <8 x float>, ptr %8, align 4
  %wide.load2 = load <8 x float>, ptr %9, align 4
  %wide.load3 = load <8 x float>, ptr %10, align 4
  %wide.load4 = load <8 x float>, ptr %11, align 4
  %12 = getelementptr float, ptr %1, i64 %index
  %13 = getelementptr float, ptr %12, i64 8
  %14 = getelementptr float, ptr %12, i64 16
  %15 = getelementptr float, ptr %12, i64 24
  %wide.load5 = load <8 x float>, ptr %12, align 4
  %wide.load6 = load <8 x float>, ptr %13, align 4
  %wide.load7 = load <8 x float>, ptr %14, align 4
  %wide.load8 = load <8 x float>, ptr %15, align 4
  %16 = fadd reassoc <8 x float> %wide.load, %wide.load5
  %17 = fadd reassoc <8 x float> %wide.load2, %wide.load6
  %18 = fadd reassoc <8 x float> %wide.load3, %wide.load7
  %19 = fadd reassoc <8 x float> %wide.load4, %wide.load8
  %20 = getelementptr float, ptr %2, i64 %index
  %21 = getelementptr float, ptr %20, i64 8
  %22 = getelementptr float, ptr %20, i64 16
  %23 = getelementptr float, ptr %20, i64 24
  store <8 x float> %16, ptr %20, align 4
  store <8 x float> %17, ptr %21, align 4
  store <8 x float> %18, ptr %22, align 4
  store <8 x float> %19, ptr %23, align 4
  %index.next = or disjoint i64 %index, 32
  %24 = getelementptr float, ptr %0, i64 %index.next
  %25 = getelementptr float, ptr %24, i64 8
  %26 = getelementptr float, ptr %24, i64 16
  %27 = getelementptr float, ptr %24, i64 24
  %wide.load.1 = load <8 x float>, ptr %24, align 4
  %wide.load2.1 = load <8 x float>, ptr %25, align 4
  %wide.load3.1 = load <8 x float>, ptr %26, align 4
  %wide.load4.1 = load <8 x float>, ptr %27, align 4
  %28 = getelementptr float, ptr %1, i64 %index.next
  %29 = getelementptr float, ptr %28, i64 8
  %30 = getelementptr float, ptr %28, i64 16
  %31 = getelementptr float, ptr %28, i64 24
  %wide.load5.1 = load <8 x float>, ptr %28, align 4
  %wide.load6.1 = load <8 x float>, ptr %29, align 4
  %wide.load7.1 = load <8 x float>, ptr %30, align 4
  %wide.load8.1 = load <8 x float>, ptr %31, align 4
  %32 = fadd reassoc <8 x float> %wide.load.1, %wide.load5.1
  %33 = fadd reassoc <8 x float> %wide.load2.1, %wide.load6.1
  %34 = fadd reassoc <8 x float> %wide.load3.1, %wide.load7.1
  %35 = fadd reassoc <8 x float> %wide.load4.1, %wide.load8.1
  %36 = getelementptr float, ptr %2, i64 %index.next
  %37 = getelementptr float, ptr %36, i64 8
  %38 = getelementptr float, ptr %36, i64 16
  %39 = getelementptr float, ptr %36, i64 24
  store <8 x float> %32, ptr %36, align 4
  store <8 x float> %33, ptr %37, align 4
  store <8 x float> %34, ptr %38, align 4
  store <8 x float> %35, ptr %39, align 4
  %index.next.1 = add nuw nsw i64 %index, 64
  %40 = icmp eq i64 %index.next.1, 10000000
  br i1 %40, label %middle.block, label %vector.body, !llvm.loop !6

scalar.ph:                                        ; preds = %vector.memcheck, %scalar.ph
  %41 = phi i64 [ %111, %scalar.ph ], [ 0, %vector.memcheck ]
  %42 = getelementptr float, ptr %0, i64 %41
  %43 = load float, ptr %42, align 4
  %44 = getelementptr float, ptr %1, i64 %41
  %45 = load float, ptr %44, align 4
  %46 = fadd reassoc float %43, %45
  %47 = getelementptr float, ptr %2, i64 %41
  store float %46, ptr %47, align 4
  %48 = or disjoint i64 %41, 1
  %49 = getelementptr float, ptr %0, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = getelementptr float, ptr %1, i64 %48
  %52 = load float, ptr %51, align 4
  %53 = fadd reassoc float %50, %52
  %54 = getelementptr float, ptr %2, i64 %48
  store float %53, ptr %54, align 4
  %55 = add nuw nsw i64 %41, 2
  %56 = getelementptr float, ptr %0, i64 %55
  %57 = load float, ptr %56, align 4
  %58 = getelementptr float, ptr %1, i64 %55
  %59 = load float, ptr %58, align 4
  %60 = fadd reassoc float %57, %59
  %61 = getelementptr float, ptr %2, i64 %55
  store float %60, ptr %61, align 4
  %62 = add nuw nsw i64 %41, 3
  %63 = getelementptr float, ptr %0, i64 %62
  %64 = load float, ptr %63, align 4
  %65 = getelementptr float, ptr %1, i64 %62
  %66 = load float, ptr %65, align 4
  %67 = fadd reassoc float %64, %66
  %68 = getelementptr float, ptr %2, i64 %62
  store float %67, ptr %68, align 4
  %69 = add nuw nsw i64 %41, 4
  %70 = getelementptr float, ptr %0, i64 %69
  %71 = load float, ptr %70, align 4
  %72 = getelementptr float, ptr %1, i64 %69
  %73 = load float, ptr %72, align 4
  %74 = fadd reassoc float %71, %73
  %75 = getelementptr float, ptr %2, i64 %69
  store float %74, ptr %75, align 4
  %76 = add nuw nsw i64 %41, 5
  %77 = getelementptr float, ptr %0, i64 %76
  %78 = load float, ptr %77, align 4
  %79 = getelementptr float, ptr %1, i64 %76
  %80 = load float, ptr %79, align 4
  %81 = fadd reassoc float %78, %80
  %82 = getelementptr float, ptr %2, i64 %76
  store float %81, ptr %82, align 4
  %83 = add nuw nsw i64 %41, 6
  %84 = getelementptr float, ptr %0, i64 %83
  %85 = load float, ptr %84, align 4
  %86 = getelementptr float, ptr %1, i64 %83
  %87 = load float, ptr %86, align 4
  %88 = fadd reassoc float %85, %87
  %89 = getelementptr float, ptr %2, i64 %83
  store float %88, ptr %89, align 4
  %90 = add nuw nsw i64 %41, 7
  %91 = getelementptr float, ptr %0, i64 %90
  %92 = load float, ptr %91, align 4
  %93 = getelementptr float, ptr %1, i64 %90
  %94 = load float, ptr %93, align 4
  %95 = fadd reassoc float %92, %94
  %96 = getelementptr float, ptr %2, i64 %90
  store float %95, ptr %96, align 4
  %97 = add nuw nsw i64 %41, 8
  %98 = getelementptr float, ptr %0, i64 %97
  %99 = load float, ptr %98, align 4
  %100 = getelementptr float, ptr %1, i64 %97
  %101 = load float, ptr %100, align 4
  %102 = fadd reassoc float %99, %101
  %103 = getelementptr float, ptr %2, i64 %97
  store float %102, ptr %103, align 4
  %104 = add nuw nsw i64 %41, 9
  %105 = getelementptr float, ptr %0, i64 %104
  %106 = load float, ptr %105, align 4
  %107 = getelementptr float, ptr %1, i64 %104
  %108 = load float, ptr %107, align 4
  %109 = fadd reassoc float %106, %108
  %110 = getelementptr float, ptr %2, i64 %104
  store float %109, ptr %110, align 4
  %111 = add nuw nsw i64 %41, 10
  %exitcond.not.9 = icmp eq i64 %111, 10000000
  br i1 %exitcond.not.9, label %middle.block, label %scalar.ph, !llvm.loop !7

middle.block:                                     ; preds = %vector.body, %scalar.ph
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.somavet.4(ptr nocapture readonly %0) local_unnamed_addr #1 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.4, %vector.body ]
  %vec.phi = phi <8 x float> [ <float 0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %41, %vector.body ]
  %vec.phi1 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %42, %vector.body ]
  %vec.phi2 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %43, %vector.body ]
  %vec.phi3 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %44, %vector.body ]
  %1 = getelementptr float, ptr %0, i64 %index
  %2 = getelementptr float, ptr %1, i64 8
  %3 = getelementptr float, ptr %1, i64 16
  %4 = getelementptr float, ptr %1, i64 24
  %wide.load = load <8 x float>, ptr %1, align 4
  %wide.load4 = load <8 x float>, ptr %2, align 4
  %wide.load5 = load <8 x float>, ptr %3, align 4
  %wide.load6 = load <8 x float>, ptr %4, align 4
  %5 = fadd reassoc <8 x float> %vec.phi, %wide.load
  %6 = fadd reassoc <8 x float> %vec.phi1, %wide.load4
  %7 = fadd reassoc <8 x float> %vec.phi2, %wide.load5
  %8 = fadd reassoc <8 x float> %vec.phi3, %wide.load6
  %9 = getelementptr float, ptr %0, i64 %index
  %10 = getelementptr float, ptr %9, i64 32
  %11 = getelementptr float, ptr %9, i64 40
  %12 = getelementptr float, ptr %9, i64 48
  %13 = getelementptr float, ptr %9, i64 56
  %wide.load.1 = load <8 x float>, ptr %10, align 4
  %wide.load4.1 = load <8 x float>, ptr %11, align 4
  %wide.load5.1 = load <8 x float>, ptr %12, align 4
  %wide.load6.1 = load <8 x float>, ptr %13, align 4
  %14 = fadd reassoc <8 x float> %5, %wide.load.1
  %15 = fadd reassoc <8 x float> %6, %wide.load4.1
  %16 = fadd reassoc <8 x float> %7, %wide.load5.1
  %17 = fadd reassoc <8 x float> %8, %wide.load6.1
  %18 = getelementptr float, ptr %0, i64 %index
  %19 = getelementptr float, ptr %18, i64 64
  %20 = getelementptr float, ptr %18, i64 72
  %21 = getelementptr float, ptr %18, i64 80
  %22 = getelementptr float, ptr %18, i64 88
  %wide.load.2 = load <8 x float>, ptr %19, align 4
  %wide.load4.2 = load <8 x float>, ptr %20, align 4
  %wide.load5.2 = load <8 x float>, ptr %21, align 4
  %wide.load6.2 = load <8 x float>, ptr %22, align 4
  %23 = fadd reassoc <8 x float> %14, %wide.load.2
  %24 = fadd reassoc <8 x float> %15, %wide.load4.2
  %25 = fadd reassoc <8 x float> %16, %wide.load5.2
  %26 = fadd reassoc <8 x float> %17, %wide.load6.2
  %27 = getelementptr float, ptr %0, i64 %index
  %28 = getelementptr float, ptr %27, i64 96
  %29 = getelementptr float, ptr %27, i64 104
  %30 = getelementptr float, ptr %27, i64 112
  %31 = getelementptr float, ptr %27, i64 120
  %wide.load.3 = load <8 x float>, ptr %28, align 4
  %wide.load4.3 = load <8 x float>, ptr %29, align 4
  %wide.load5.3 = load <8 x float>, ptr %30, align 4
  %wide.load6.3 = load <8 x float>, ptr %31, align 4
  %32 = fadd reassoc <8 x float> %23, %wide.load.3
  %33 = fadd reassoc <8 x float> %24, %wide.load4.3
  %34 = fadd reassoc <8 x float> %25, %wide.load5.3
  %35 = fadd reassoc <8 x float> %26, %wide.load6.3
  %36 = getelementptr float, ptr %0, i64 %index
  %37 = getelementptr float, ptr %36, i64 128
  %38 = getelementptr float, ptr %36, i64 136
  %39 = getelementptr float, ptr %36, i64 144
  %40 = getelementptr float, ptr %36, i64 152
  %wide.load.4 = load <8 x float>, ptr %37, align 4
  %wide.load4.4 = load <8 x float>, ptr %38, align 4
  %wide.load5.4 = load <8 x float>, ptr %39, align 4
  %wide.load6.4 = load <8 x float>, ptr %40, align 4
  %41 = fadd reassoc <8 x float> %32, %wide.load.4
  %42 = fadd reassoc <8 x float> %33, %wide.load4.4
  %43 = fadd reassoc <8 x float> %34, %wide.load5.4
  %44 = fadd reassoc <8 x float> %35, %wide.load6.4
  %index.next.4 = add nuw nsw i64 %index, 160
  %45 = icmp eq i64 %index.next.4, 10000000
  br i1 %45, label %middle.block, label %vector.body, !llvm.loop !8

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd reassoc <8 x float> %42, %41
  %bin.rdx7 = fadd reassoc <8 x float> %43, %bin.rdx
  %bin.rdx8 = fadd reassoc <8 x float> %44, %bin.rdx7
  %46 = tail call reassoc float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %bin.rdx8)
  ret float %46
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read)
define float @dsl.prodesc.5(ptr nocapture readonly %0, ptr nocapture readonly %1) local_unnamed_addr #1 {
vector.ph:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next.3, %vector.body ]
  %vec.phi = phi <8 x float> [ <float 0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %62, %vector.body ]
  %vec.phi1 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %63, %vector.body ]
  %vec.phi2 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %64, %vector.body ]
  %vec.phi3 = phi <8 x float> [ <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %vector.ph ], [ %65, %vector.body ]
  %2 = getelementptr float, ptr %0, i64 %index
  %3 = getelementptr float, ptr %2, i64 8
  %4 = getelementptr float, ptr %2, i64 16
  %5 = getelementptr float, ptr %2, i64 24
  %wide.load = load <8 x float>, ptr %2, align 4
  %wide.load4 = load <8 x float>, ptr %3, align 4
  %wide.load5 = load <8 x float>, ptr %4, align 4
  %wide.load6 = load <8 x float>, ptr %5, align 4
  %6 = getelementptr float, ptr %1, i64 %index
  %7 = getelementptr float, ptr %6, i64 8
  %8 = getelementptr float, ptr %6, i64 16
  %9 = getelementptr float, ptr %6, i64 24
  %wide.load7 = load <8 x float>, ptr %6, align 4
  %wide.load8 = load <8 x float>, ptr %7, align 4
  %wide.load9 = load <8 x float>, ptr %8, align 4
  %wide.load10 = load <8 x float>, ptr %9, align 4
  %10 = fmul reassoc <8 x float> %wide.load, %wide.load7
  %11 = fmul reassoc <8 x float> %wide.load4, %wide.load8
  %12 = fmul reassoc <8 x float> %wide.load5, %wide.load9
  %13 = fmul reassoc <8 x float> %wide.load6, %wide.load10
  %14 = fadd reassoc <8 x float> %vec.phi, %10
  %15 = fadd reassoc <8 x float> %vec.phi1, %11
  %16 = fadd reassoc <8 x float> %vec.phi2, %12
  %17 = fadd reassoc <8 x float> %vec.phi3, %13
  %index.next = or disjoint i64 %index, 32
  %18 = getelementptr float, ptr %0, i64 %index.next
  %19 = getelementptr float, ptr %18, i64 8
  %20 = getelementptr float, ptr %18, i64 16
  %21 = getelementptr float, ptr %18, i64 24
  %wide.load.1 = load <8 x float>, ptr %18, align 4
  %wide.load4.1 = load <8 x float>, ptr %19, align 4
  %wide.load5.1 = load <8 x float>, ptr %20, align 4
  %wide.load6.1 = load <8 x float>, ptr %21, align 4
  %22 = getelementptr float, ptr %1, i64 %index.next
  %23 = getelementptr float, ptr %22, i64 8
  %24 = getelementptr float, ptr %22, i64 16
  %25 = getelementptr float, ptr %22, i64 24
  %wide.load7.1 = load <8 x float>, ptr %22, align 4
  %wide.load8.1 = load <8 x float>, ptr %23, align 4
  %wide.load9.1 = load <8 x float>, ptr %24, align 4
  %wide.load10.1 = load <8 x float>, ptr %25, align 4
  %26 = fmul reassoc <8 x float> %wide.load.1, %wide.load7.1
  %27 = fmul reassoc <8 x float> %wide.load4.1, %wide.load8.1
  %28 = fmul reassoc <8 x float> %wide.load5.1, %wide.load9.1
  %29 = fmul reassoc <8 x float> %wide.load6.1, %wide.load10.1
  %30 = fadd reassoc <8 x float> %14, %26
  %31 = fadd reassoc <8 x float> %15, %27
  %32 = fadd reassoc <8 x float> %16, %28
  %33 = fadd reassoc <8 x float> %17, %29
  %index.next.1 = or disjoint i64 %index, 64
  %34 = getelementptr float, ptr %0, i64 %index.next.1
  %35 = getelementptr float, ptr %34, i64 8
  %36 = getelementptr float, ptr %34, i64 16
  %37 = getelementptr float, ptr %34, i64 24
  %wide.load.2 = load <8 x float>, ptr %34, align 4
  %wide.load4.2 = load <8 x float>, ptr %35, align 4
  %wide.load5.2 = load <8 x float>, ptr %36, align 4
  %wide.load6.2 = load <8 x float>, ptr %37, align 4
  %38 = getelementptr float, ptr %1, i64 %index.next.1
  %39 = getelementptr float, ptr %38, i64 8
  %40 = getelementptr float, ptr %38, i64 16
  %41 = getelementptr float, ptr %38, i64 24
  %wide.load7.2 = load <8 x float>, ptr %38, align 4
  %wide.load8.2 = load <8 x float>, ptr %39, align 4
  %wide.load9.2 = load <8 x float>, ptr %40, align 4
  %wide.load10.2 = load <8 x float>, ptr %41, align 4
  %42 = fmul reassoc <8 x float> %wide.load.2, %wide.load7.2
  %43 = fmul reassoc <8 x float> %wide.load4.2, %wide.load8.2
  %44 = fmul reassoc <8 x float> %wide.load5.2, %wide.load9.2
  %45 = fmul reassoc <8 x float> %wide.load6.2, %wide.load10.2
  %46 = fadd reassoc <8 x float> %30, %42
  %47 = fadd reassoc <8 x float> %31, %43
  %48 = fadd reassoc <8 x float> %32, %44
  %49 = fadd reassoc <8 x float> %33, %45
  %index.next.2 = or disjoint i64 %index, 96
  %50 = getelementptr float, ptr %0, i64 %index.next.2
  %51 = getelementptr float, ptr %50, i64 8
  %52 = getelementptr float, ptr %50, i64 16
  %53 = getelementptr float, ptr %50, i64 24
  %wide.load.3 = load <8 x float>, ptr %50, align 4
  %wide.load4.3 = load <8 x float>, ptr %51, align 4
  %wide.load5.3 = load <8 x float>, ptr %52, align 4
  %wide.load6.3 = load <8 x float>, ptr %53, align 4
  %54 = getelementptr float, ptr %1, i64 %index.next.2
  %55 = getelementptr float, ptr %54, i64 8
  %56 = getelementptr float, ptr %54, i64 16
  %57 = getelementptr float, ptr %54, i64 24
  %wide.load7.3 = load <8 x float>, ptr %54, align 4
  %wide.load8.3 = load <8 x float>, ptr %55, align 4
  %wide.load9.3 = load <8 x float>, ptr %56, align 4
  %wide.load10.3 = load <8 x float>, ptr %57, align 4
  %58 = fmul reassoc <8 x float> %wide.load.3, %wide.load7.3
  %59 = fmul reassoc <8 x float> %wide.load4.3, %wide.load8.3
  %60 = fmul reassoc <8 x float> %wide.load5.3, %wide.load9.3
  %61 = fmul reassoc <8 x float> %wide.load6.3, %wide.load10.3
  %62 = fadd reassoc <8 x float> %46, %58
  %63 = fadd reassoc <8 x float> %47, %59
  %64 = fadd reassoc <8 x float> %48, %60
  %65 = fadd reassoc <8 x float> %49, %61
  %index.next.3 = add nuw nsw i64 %index, 128
  %66 = icmp eq i64 %index.next.3, 10000000
  br i1 %66, label %middle.block, label %vector.body, !llvm.loop !9

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd reassoc <8 x float> %63, %62
  %bin.rdx11 = fadd reassoc <8 x float> %64, %bin.rdx
  %bin.rdx12 = fadd reassoc <8 x float> %65, %bin.rdx11
  %67 = tail call reassoc float @llvm.vector.reduce.fadd.v8f32(float -0.000000e+00, <8 x float> %bin.rdx12)
  ret float %67
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.vector.reduce.fadd.v8f32(float, <8 x float>) #2

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) }
attributes #1 = { nofree norecurse nosync nounwind memory(argmem: read) }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
!4 = distinct !{!4, !2}
!5 = distinct !{!5, !2, !3}
!6 = distinct !{!6, !2, !3}
!7 = distinct !{!7, !2}
!8 = distinct !{!8, !2, !3}
!9 = distinct !{!9, !2, !3}

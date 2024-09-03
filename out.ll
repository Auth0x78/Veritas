; ModuleID = 'main.vrs'
source_filename = "main.vrs"

declare i32 @printf(i8*, ...)

declare i32 @scanf(i8*, ...)

define i32 @main(...) {
entry:
  %a = alloca float, align 4
  store float 0x402270A3E0000000, float* %a, align 4
  %b = alloca i32, align 4
  store i32 10, i32* %b, align 4
  %aload = load float, float* %a, align 4
  %bload = load i32, i32* %b, align 4
  %sitofp = sitofp i32 %bload to double
  %subtmp = fsub double %sitofp, 1.000000e-01
  %fpcast = fpext float %aload to double
  %multmp = fmul double %fpcast, %subtmp
  %fptosi = fptosi double %multmp to i32
  ret i32 %fptosi
}

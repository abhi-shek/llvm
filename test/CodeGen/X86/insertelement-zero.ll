; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse3 | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE3
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+ssse3 | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSSE3
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse4.1 | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE41
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx2 | FileCheck %s --check-prefixes=ALL,AVX,AVX2,AVX2-SLOW
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx2,+fast-variable-shuffle | FileCheck %s --check-prefixes=ALL,AVX,AVX2,AVX2-FAST

define <2 x double> @insert_v2f64_z1(<2 x double> %a) {
; SSE2-LABEL: insert_v2f64_z1:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorpd %xmm1, %xmm1
; SSE2-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v2f64_z1:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorpd %xmm1, %xmm1
; SSE3-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v2f64_z1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorpd %xmm1, %xmm1
; SSSE3-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v2f64_z1:
; SSE41:       # %bb.0:
; SSE41-NEXT:    xorpd %xmm1, %xmm1
; SSE41-NEXT:    blendpd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v2f64_z1:
; AVX:       # %bb.0:
; AVX-NEXT:    vxorpd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vblendpd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; AVX-NEXT:    retq
  %1 = insertelement <2 x double> %a, double 0.0, i32 0
  ret <2 x double> %1
}

define <4 x double> @insert_v4f64_0zz3(<4 x double> %a) {
; SSE2-LABEL: insert_v4f64_0zz3:
; SSE2:       # %bb.0:
; SSE2-NEXT:    movq {{.*#+}} xmm0 = xmm0[0],zero
; SSE2-NEXT:    xorpd %xmm2, %xmm2
; SSE2-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v4f64_0zz3:
; SSE3:       # %bb.0:
; SSE3-NEXT:    movq {{.*#+}} xmm0 = xmm0[0],zero
; SSE3-NEXT:    xorpd %xmm2, %xmm2
; SSE3-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v4f64_0zz3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = xmm0[0],zero
; SSSE3-NEXT:    xorpd %xmm2, %xmm2
; SSSE3-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v4f64_0zz3:
; SSE41:       # %bb.0:
; SSE41-NEXT:    movq {{.*#+}} xmm0 = xmm0[0],zero
; SSE41-NEXT:    xorpd %xmm2, %xmm2
; SSE41-NEXT:    blendpd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v4f64_0zz3:
; AVX:       # %bb.0:
; AVX-NEXT:    vxorpd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2],ymm0[3]
; AVX-NEXT:    retq
  %1 = insertelement <4 x double> %a, double 0.0, i32 1
  %2 = insertelement <4 x double> %1, double 0.0, i32 2
  ret <4 x double> %2
}

define <2 x i64> @insert_v2i64_z1(<2 x i64> %a) {
; SSE2-LABEL: insert_v2i64_z1:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorpd %xmm1, %xmm1
; SSE2-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v2i64_z1:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorpd %xmm1, %xmm1
; SSE3-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v2i64_z1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorpd %xmm1, %xmm1
; SSSE3-NEXT:    movsd {{.*#+}} xmm0 = xmm1[0],xmm0[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v2i64_z1:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm1, %xmm1
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm1[0,1,2,3],xmm0[4,5,6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: insert_v2i64_z1:
; AVX1:       # %bb.0:
; AVX1-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm1[0,1,2,3],xmm0[4,5,6,7]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_v2i64_z1:
; AVX2:       # %bb.0:
; AVX2-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vblendps {{.*#+}} xmm0 = xmm1[0,1],xmm0[2,3]
; AVX2-NEXT:    retq
  %1 = insertelement <2 x i64> %a, i64 0, i32 0
  ret <2 x i64> %1
}

define <4 x i64> @insert_v4i64_01z3(<4 x i64> %a) {
; SSE2-LABEL: insert_v4i64_01z3:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorpd %xmm2, %xmm2
; SSE2-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v4i64_01z3:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorpd %xmm2, %xmm2
; SSE3-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v4i64_01z3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorpd %xmm2, %xmm2
; SSSE3-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v4i64_01z3:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm1 = xmm2[0,1,2,3],xmm1[4,5,6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: insert_v4i64_01z3:
; AVX1:       # %bb.0:
; AVX1-NEXT:    vxorpd %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2],ymm0[3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_v4i64_01z3:
; AVX2:       # %bb.0:
; AVX2-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vblendps {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5],ymm0[6,7]
; AVX2-NEXT:    retq
  %1 = insertelement <4 x i64> %a, i64 0, i32 2
  ret <4 x i64> %1
}

define <4 x float> @insert_v4f32_01z3(<4 x float> %a) {
; SSE2-LABEL: insert_v4f32_01z3:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSE2-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v4f32_01z3:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorps %xmm1, %xmm1
; SSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSE3-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v4f32_01z3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorps %xmm1, %xmm1
; SSSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSSE3-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v4f32_01z3:
; SSE41:       # %bb.0:
; SSE41-NEXT:    xorps %xmm1, %xmm1
; SSE41-NEXT:    blendps {{.*#+}} xmm0 = xmm0[0,1],xmm1[2],xmm0[3]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v4f32_01z3:
; AVX:       # %bb.0:
; AVX-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vblendps {{.*#+}} xmm0 = xmm0[0,1],xmm1[2],xmm0[3]
; AVX-NEXT:    retq
  %1 = insertelement <4 x float> %a, float 0.0, i32 2
  ret <4 x float> %1
}

define <8 x float> @insert_v8f32_z12345z7(<8 x float> %a) {
; SSE2-LABEL: insert_v8f32_z12345z7:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorps %xmm2, %xmm2
; SSE2-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSE2-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSE2-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v8f32_z12345z7:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorps %xmm2, %xmm2
; SSE3-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSE3-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v8f32_z12345z7:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorps %xmm2, %xmm2
; SSSE3-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSSE3-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v8f32_z12345z7:
; SSE41:       # %bb.0:
; SSE41-NEXT:    xorps %xmm2, %xmm2
; SSE41-NEXT:    blendps {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSE41-NEXT:    blendps {{.*#+}} xmm1 = xmm1[0,1],xmm2[2],xmm1[3]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v8f32_z12345z7:
; AVX:       # %bb.0:
; AVX-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vblendps {{.*#+}} ymm0 = ymm1[0],ymm0[1,2,3,4,5],ymm1[6],ymm0[7]
; AVX-NEXT:    retq
  %1 = insertelement <8 x float> %a, float 0.0, i32 0
  %2 = insertelement <8 x float> %1, float 0.0, i32 6
  ret <8 x float> %2
}

define <4 x i32> @insert_v4i32_01z3(<4 x i32> %a) {
; SSE2-LABEL: insert_v4i32_01z3:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSE2-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v4i32_01z3:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorps %xmm1, %xmm1
; SSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSE3-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v4i32_01z3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorps %xmm1, %xmm1
; SSSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,0],xmm0[3,0]
; SSSE3-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0,2]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v4i32_01z3:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm1, %xmm1
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm1[4,5],xmm0[6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: insert_v4i32_01z3:
; AVX1:       # %bb.0:
; AVX1-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm1[4,5],xmm0[6,7]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_v4i32_01z3:
; AVX2:       # %bb.0:
; AVX2-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vblendps {{.*#+}} xmm0 = xmm0[0,1],xmm1[2],xmm0[3]
; AVX2-NEXT:    retq
  %1 = insertelement <4 x i32> %a, i32 0, i32 2
  ret <4 x i32> %1
}

define <8 x i32> @insert_v8i32_z12345z7(<8 x i32> %a) {
; SSE2-LABEL: insert_v8i32_z12345z7:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorps %xmm2, %xmm2
; SSE2-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSE2-NEXT:    xorps %xmm2, %xmm2
; SSE2-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSE2-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v8i32_z12345z7:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorps %xmm2, %xmm2
; SSE3-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSE3-NEXT:    xorps %xmm2, %xmm2
; SSE3-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v8i32_z12345z7:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorps %xmm2, %xmm2
; SSSE3-NEXT:    movss {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3]
; SSSE3-NEXT:    xorps %xmm2, %xmm2
; SSSE3-NEXT:    shufps {{.*#+}} xmm2 = xmm2[0,0],xmm1[3,0]
; SSSE3-NEXT:    shufps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0,2]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v8i32_z12345z7:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm2[0,1],xmm0[2,3,4,5,6,7]
; SSE41-NEXT:    pblendw {{.*#+}} xmm1 = xmm1[0,1,2,3],xmm2[4,5],xmm1[6,7]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v8i32_z12345z7:
; AVX:       # %bb.0:
; AVX-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vblendps {{.*#+}} ymm0 = ymm1[0],ymm0[1,2,3,4,5],ymm1[6],ymm0[7]
; AVX-NEXT:    retq
  %1 = insertelement <8 x i32> %a, i32 0, i32 0
  %2 = insertelement <8 x i32> %1, i32 0, i32 6
  ret <8 x i32> %2
}

define <8 x i16> @insert_v8i16_z12345z7(<8 x i16> %a) {
; SSE2-LABEL: insert_v8i16_z12345z7:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorl %eax, %eax
; SSE2-NEXT:    pinsrw $0, %eax, %xmm0
; SSE2-NEXT:    pinsrw $6, %eax, %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v8i16_z12345z7:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorl %eax, %eax
; SSE3-NEXT:    pinsrw $0, %eax, %xmm0
; SSE3-NEXT:    pinsrw $6, %eax, %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v8i16_z12345z7:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorl %eax, %eax
; SSSE3-NEXT:    pinsrw $0, %eax, %xmm0
; SSSE3-NEXT:    pinsrw $6, %eax, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v8i16_z12345z7:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm1, %xmm1
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm1[0],xmm0[1,2,3,4,5],xmm1[6],xmm0[7]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v8i16_z12345z7:
; AVX:       # %bb.0:
; AVX-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpblendw {{.*#+}} xmm0 = xmm1[0],xmm0[1,2,3,4,5],xmm1[6],xmm0[7]
; AVX-NEXT:    retq
  %1 = insertelement <8 x i16> %a, i16 0, i32 0
  %2 = insertelement <8 x i16> %1, i16 0, i32 6
  ret <8 x i16> %2
}

define <16 x i16> @insert_v16i16_z12345z789ABCDEz(<16 x i16> %a) {
; SSE2-LABEL: insert_v16i16_z12345z789ABCDEz:
; SSE2:       # %bb.0:
; SSE2-NEXT:    xorl %eax, %eax
; SSE2-NEXT:    pinsrw $0, %eax, %xmm0
; SSE2-NEXT:    pinsrw $6, %eax, %xmm0
; SSE2-NEXT:    pinsrw $7, %eax, %xmm1
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v16i16_z12345z789ABCDEz:
; SSE3:       # %bb.0:
; SSE3-NEXT:    xorl %eax, %eax
; SSE3-NEXT:    pinsrw $0, %eax, %xmm0
; SSE3-NEXT:    pinsrw $6, %eax, %xmm0
; SSE3-NEXT:    pinsrw $7, %eax, %xmm1
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v16i16_z12345z789ABCDEz:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    xorl %eax, %eax
; SSSE3-NEXT:    pinsrw $0, %eax, %xmm0
; SSSE3-NEXT:    pinsrw $6, %eax, %xmm0
; SSSE3-NEXT:    pinsrw $7, %eax, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v16i16_z12345z789ABCDEz:
; SSE41:       # %bb.0:
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm2[0],xmm0[1,2,3,4,5],xmm2[6],xmm0[7]
; SSE41-NEXT:    pblendw {{.*#+}} xmm1 = xmm1[0,1,2,3,4,5,6],xmm2[7]
; SSE41-NEXT:    retq
;
; AVX-LABEL: insert_v16i16_z12345z789ABCDEz:
; AVX:       # %bb.0:
; AVX-NEXT:    vandps {{.*}}(%rip), %ymm0, %ymm0
; AVX-NEXT:    retq
  %1 = insertelement <16 x i16> %a, i16 0, i32 0
  %2 = insertelement <16 x i16> %1, i16 0, i32 6
  %3 = insertelement <16 x i16> %2, i16 0, i32 15
  ret <16 x i16> %3
}

define <16 x i8> @insert_v16i8_z123456789ABCDEz(<16 x i8> %a) {
; SSE2-LABEL: insert_v16i8_z123456789ABCDEz:
; SSE2:       # %bb.0:
; SSE2-NEXT:    andps {{.*}}(%rip), %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v16i8_z123456789ABCDEz:
; SSE3:       # %bb.0:
; SSE3-NEXT:    andps {{.*}}(%rip), %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v16i8_z123456789ABCDEz:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    andps {{.*}}(%rip), %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v16i8_z123456789ABCDEz:
; SSE41:       # %bb.0:
; SSE41-NEXT:    xorl %eax, %eax
; SSE41-NEXT:    pinsrb $0, %eax, %xmm0
; SSE41-NEXT:    pinsrb $15, %eax, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: insert_v16i8_z123456789ABCDEz:
; AVX1:       # %bb.0:
; AVX1-NEXT:    xorl %eax, %eax
; AVX1-NEXT:    vpinsrb $0, %eax, %xmm0, %xmm0
; AVX1-NEXT:    vpinsrb $15, %eax, %xmm0, %xmm0
; AVX1-NEXT:    retq
;
; AVX2-SLOW-LABEL: insert_v16i8_z123456789ABCDEz:
; AVX2-SLOW:       # %bb.0:
; AVX2-SLOW-NEXT:    xorl %eax, %eax
; AVX2-SLOW-NEXT:    vpinsrb $0, %eax, %xmm0, %xmm0
; AVX2-SLOW-NEXT:    vpinsrb $15, %eax, %xmm0, %xmm0
; AVX2-SLOW-NEXT:    retq
;
; AVX2-FAST-LABEL: insert_v16i8_z123456789ABCDEz:
; AVX2-FAST:       # %bb.0:
; AVX2-FAST-NEXT:    vandps {{.*}}(%rip), %xmm0, %xmm0
; AVX2-FAST-NEXT:    retq
  %1 = insertelement <16 x i8> %a, i8 0, i32 0
  %2 = insertelement <16 x i8> %1, i8 0, i32 15
  ret <16 x i8> %2
}

define <32 x i8> @insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz(<32 x i8> %a) {
; SSE2-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; SSE2:       # %bb.0:
; SSE2-NEXT:    andps {{.*}}(%rip), %xmm0
; SSE2-NEXT:    andps {{.*}}(%rip), %xmm1
; SSE2-NEXT:    retq
;
; SSE3-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; SSE3:       # %bb.0:
; SSE3-NEXT:    andps {{.*}}(%rip), %xmm0
; SSE3-NEXT:    andps {{.*}}(%rip), %xmm1
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    andps {{.*}}(%rip), %xmm0
; SSSE3-NEXT:    andps {{.*}}(%rip), %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; SSE41:       # %bb.0:
; SSE41-NEXT:    xorl %eax, %eax
; SSE41-NEXT:    pinsrb $0, %eax, %xmm0
; SSE41-NEXT:    pinsrb $15, %eax, %xmm0
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm1 = xmm1[0,1,2,3,4,5,6],xmm2[7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; AVX1:       # %bb.0:
; AVX1-NEXT:    xorl %eax, %eax
; AVX1-NEXT:    vpinsrb $0, %eax, %xmm0, %xmm1
; AVX1-NEXT:    vpinsrb $15, %eax, %xmm1, %xmm1
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3,4,5,6],xmm2[7]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-SLOW-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; AVX2-SLOW:       # %bb.0:
; AVX2-SLOW-NEXT:    xorl %eax, %eax
; AVX2-SLOW-NEXT:    vpinsrb $0, %eax, %xmm0, %xmm1
; AVX2-SLOW-NEXT:    vpinsrb $15, %eax, %xmm1, %xmm1
; AVX2-SLOW-NEXT:    vextracti128 $1, %ymm0, %xmm0
; AVX2-SLOW-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX2-SLOW-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3,4,5,6],xmm2[7]
; AVX2-SLOW-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; AVX2-SLOW-NEXT:    retq
;
; AVX2-FAST-LABEL: insert_v32i8_z123456789ABCDEzGHIJKLMNOPQRSTzz:
; AVX2-FAST:       # %bb.0:
; AVX2-FAST-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm1
; AVX2-FAST-NEXT:    vextracti128 $1, %ymm0, %xmm0
; AVX2-FAST-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX2-FAST-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3,4,5,6],xmm2[7]
; AVX2-FAST-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; AVX2-FAST-NEXT:    retq
  %1 = insertelement <32 x i8> %a, i8 0, i32 0
  %2 = insertelement <32 x i8> %1, i8 0, i32 15
  %3 = insertelement <32 x i8> %2, i8 0, i32 30
  %4 = insertelement <32 x i8> %3, i8 0, i32 31
  ret <32 x i8> %4
}

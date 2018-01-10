# Check 64bit AVX512{_4FMAPS,VL} instructions

	.allow_index_reg
	.text
_start:
	v4fmaddps	(%rcx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	(%rcx), %xmm8, %xmm1{%k7}	 # AVX512{_4FMAPS,VL}
	v4fmaddps	(%rcx), %xmm8, %xmm1{%k7}{z}	 # AVX512{_4FMAPS,VL}
	v4fmaddps	-123456(%rax,%r14,8), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	0x7f0(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	0x800(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	-0x800(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	-0x810(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	(%rcx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	(%rcx), %ymm8, %ymm1{%k7}	 # AVX512{_4FMAPS,VL}
	v4fmaddps	(%rcx), %ymm8, %ymm1{%k7}{z}	 # AVX512{_4FMAPS,VL}
	v4fmaddps	-123456(%rax,%r14,8), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	0x7f0(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	0x800(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fmaddps	-0x800(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	-0x810(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %xmm8, %xmm1{%k7}	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %xmm8, %xmm1{%k7}{z}	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	-123456(%rax,%r14,8), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	0x7f0(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	0x800(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	-0x800(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	-0x810(%rdx), %xmm8, %xmm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %ymm8, %ymm1{%k7}	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	(%rcx), %ymm8, %ymm1{%k7}{z}	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	-123456(%rax,%r14,8), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	0x7f0(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	0x800(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	-0x800(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	-0x810(%rdx), %ymm8, %ymm1	 # AVX512{_4FMAPS,VL}

	.intel_syntax noprefix
	v4fmaddps	xmm1, xmm8, [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1{k7}, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1{k7}{z}, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rax+r14*8-123456]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rdx+0x7f0]	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rdx+0x800]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rdx-0x800]	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	xmm1, xmm8, XMMWORD PTR [rdx-0x810]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1, ymm8, [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1{k7}, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1{k7}{z}, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rax+r14*8-123456]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rdx+0x7f0]	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rdx+0x800]	 # AVX512{_4FMAPS,VL}
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rdx-0x800]	 # AVX512{_4FMAPS,VL} Disp8
	v4fmaddps	ymm1, ymm8, XMMWORD PTR [rdx-0x810]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1, xmm8, [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1{k7}, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1{k7}{z}, xmm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rax+r14*8-123456]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rdx+0x7f0]	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rdx+0x800]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rdx-0x800]	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	xmm1, xmm8, XMMWORD PTR [rdx-0x810]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1, ymm8, [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1{k7}, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1{k7}{z}, ymm8, XMMWORD PTR [rcx]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rax+r14*8-123456]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rdx+0x7f0]	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rdx+0x800]	 # AVX512{_4FMAPS,VL}
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rdx-0x800]	 # AVX512{_4FMAPS,VL} Disp8
	v4fnmaddps	ymm1, ymm8, XMMWORD PTR [rdx-0x810]	 # AVX512{_4FMAPS,VL}

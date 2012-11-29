// cpu.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "cpu.h"
#include "misc.h"
#include <algorithm>

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
#include <signal.h>
#include <setjmp.h>
#endif

#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
#include <emmintrin.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

#ifdef CRYPTOPP_CPUID_AVAILABLE

#if _MSC_VER >= 1400 && CRYPTOPP_BOOL_X64

bool CpuId(word32 input, word32 *output)
{
	__cpuid((int *)output, input);
	return true;
}

#else

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
extern "C" {
typedef void (*SigHandler)(int);

static jmp_buf s_jmpNoCPUID;
static void SigIllHandlerCPUID(int)
{
	longjmp(s_jmpNoCPUID, 1);
}

static jmp_buf s_jmpNoSSE2;
static void SigIllHandlerSSE2(int)
{
	longjmp(s_jmpNoSSE2, 1);
}
}
#endif

bool CpuId(word32 input, word32 *output)
{
#ifdef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
    __try
	{
		__asm
		{
			mov eax, input
			cpuid
			mov edi, output
			mov [edi], eax
			mov [edi+4], ebx
			mov [edi+8], ecx
			mov [edi+12], edx
		}
	}
    __except (1)
	{
		return false;
    }
	return true;
#else
	SigHandler oldHandler = signal(SIGILL, SigIllHandlerCPUID);
	if (oldHandler == SIG_ERR)
		return false;

	bool result = true;
	if (setjmp(s_jmpNoCPUID))
		result = false;
	else
	{
		asm
		(
			// save ebx in case -fPIC is being used
#if CRYPTOPP_BOOL_X86
			"push %%ebx; cpuid; mov %%ebx, %%edi; pop %%ebx"
#else
			"pushq %%rbx; cpuid; mov %%ebx, %%edi; popq %%rbx"
#endif
			: "=a" (output[0]), "=D" (output[1]), "=c" (output[2]), "=d" (output[3])
			: "a" (input)
		);
	}

	signal(SIGILL, oldHandler);
	return result;
#endif
}

#endif

static bool TrySSE2()
{
#if CRYPTOPP_BOOL_X64
	return true;
#elif defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
    __try
	{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
        AS2(por xmm0, xmm0)        // executing SSE2 instruction
#elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
		__m128i x = _mm_setzero_si128();
		return _mm_cvtsi128_si32(x) == 0;
#endif
	}
    __except (1)
	{
		return false;
    }
	return true;
#else
	SigHandler oldHandler = signal(SIGILL, SigIllHandlerSSE2);
	if (oldHandler == SIG_ERR)
		return false;

	bool result = true;
	if (setjmp(s_jmpNoSSE2))
		result = false;
	else
	{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
		__asm __volatile ("por %xmm0, %xmm0");
#elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
		__m128i x = _mm_setzero_si128();
		result = _mm_cvtsi128_si32(x) == 0;
#endif
	}

	signal(SIGILL, oldHandler);
	return result;
#endif
}

bool g_x86DetectionDone = false;
bool g_hasISSE = false, g_hasSSE2 = false, g_hasSSSE3 = false, g_hasMMX = false, g_hasAESNI = false, g_hasCLMUL = false, g_isP4 = false;
word32 g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

void DetectX86Features()
{
	word32 cpuid[4], cpuid1[4];
	if (!CpuId(0, cpuid))
		return;
	if (!CpuId(1, cpuid1))
		return;

	g_hasMMX = (cpuid1[3] & (1 << 23)) != 0;
	if ((cpuid1[3] & (1 << 26)) != 0)
		g_hasSSE2 = TrySSE2();
	g_hasSSSE3 = g_hasSSE2 && (cpuid1[2] & (1<<9));
	g_hasAESNI = g_hasSSE2 && (cpuid1[2] & (1<<25));
	g_hasCLMUL = g_hasSSE2 && (cpuid1[2] & (1<<1));

	if ((cpuid1[3] & (1 << 25)) != 0)
		g_hasISSE = true;
	else
	{
		word32 cpuid2[4];
		CpuId(0x080000000, cpuid2);
		if (cpuid2[0] >= 0x080000001)
		{
			CpuId(0x080000001, cpuid2);
			g_hasISSE = (cpuid2[3] & (1 << 22)) != 0;
		}
	}

	std::swap(cpuid[2], cpuid[3]);
	if (memcmp(cpuid+1, "GenuineIntel", 12) == 0)
	{
		g_isP4 = ((cpuid1[0] >> 8) & 0xf) == 0xf;
		g_cacheLineSize = 8 * GETBYTE(cpuid1[1], 1);
	}
	else if (memcmp(cpuid+1, "AuthenticAMD", 12) == 0)
	{
		CpuId(0x80000005, cpuid);
		g_cacheLineSize = GETBYTE(cpuid[2], 0);
	}

	if (!g_cacheLineSize)
		g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

	g_x86DetectionDone = true;
}

#endif

NAMESPACE_END

#endif

/* reloc_aa64.c - position independent ARM Aarch64 ELF shared object relocator
   Copyright (C) 2014 Linaro Ltd. <ard.biesheuvel@linaro.org>
   Copyright (C) 1999 Hewlett-Packard Co.
	Contributed by David Mosberger <davidm@hpl.hp.com>.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials
      provided with the distribution.
    * Neither the name of Hewlett-Packard Co. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
    BE LIABLE FOR ANYDIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
    OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
    THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#include <efi.h>
#include <efilib.h>

#include <stddef.h>

#include <elf.h>

static const wchar_t hex[] = L"0123456789abcdef";

static void
printval_(EFI_SYSTEM_TABLE *systab, int size, uint64_t value)
{
	wchar_t buf[21] = L"0x0000000000000000\r\n";
	int shift;
	uint64_t mask;
	int i;

	switch(size) {
	case 1:
		shift = 4;
		break;
	case 2:
		shift = 12;
		break;
	case 4:
		shift = 28;
		break;
	case 8:
	default:
		shift = 60;
		break;
	}
	mask = 0xfull << shift;

	for (i = 2; mask != 0; i += 2) {
		buf[i] = hex[(value & mask) >> shift];
		mask >>= 4;
		shift -= 4;
		buf[i+1] = hex[(value & mask) >> shift];
		mask >>= 4;
		shift -= 4;
	}
	buf[i+0] = L'\r';
	buf[i+1] = L'\n';
	buf[i+2] = L'\0';

	systab->ConOut->OutputString(systab->ConOut, buf);
}

#define printval(a) printval_(systab, sizeof(a), (uint64_t)(a))

static void
print_(EFI_SYSTEM_TABLE *systab, wchar_t *str)
{
	systab->ConOut->OutputString(systab->ConOut, str);
}

extern void *_DYNAMIC;
extern void *ImageBase;
#define print(x) print_(systab, (x))

EFI_STATUS _relocate (
		      EFI_HANDLE image EFI_UNUSED,
		      EFI_SYSTEM_TABLE *systab EFI_UNUSED)
{
	long relsz = 0, relent = 0;
	Elf64_Rela *rel = 0;
	unsigned long *addr;
	int i;
	uint64_t ldbase = (uint64_t)ImageBase;
	Elf64_Dyn *dyn = _DYNAMIC;

	print(L"in _relocate()\r\n");
	print(L"ldbase: "); printval(ldbase);
	print(L"dyn: "); printval(dyn);
	print(L"image: "); printval(image);
	print(L"systab: "); printval(systab);
	print(L"_DYNAMIC: "); printval(&_DYNAMIC);

	for (i = 0; dyn[i].d_tag != DT_NULL; ++i) {
		switch (dyn[i].d_tag) {
			case DT_RELA:
				rel = (Elf64_Rela*)
					((unsigned long)dyn[i].d_un.d_ptr
					 + ldbase);
				break;

			case DT_RELASZ:
				relsz = dyn[i].d_un.d_val;
				break;

			case DT_RELAENT:
				relent = dyn[i].d_un.d_val;
				break;

			default:
				break;
		}
	}

	if (!rel && relent == 0) {
		systab->ConOut->OutputString(systab->ConOut, L"!rel && relent == 0\r\n");
		return EFI_SUCCESS;
	}

	if (!rel || relent == 0) {
		systab->ConOut->OutputString(systab->ConOut, L"!rel || relent == 0\r\n");
		return EFI_LOAD_ERROR;
	}

	while (relsz > 0) {
		/* apply the relocs */
		switch (ELF64_R_TYPE (rel->r_info)) {
			case R_AARCH64_NONE:
				break;

			case R_AARCH64_RELATIVE:
				addr = (unsigned long *)
					(ldbase + rel->r_offset);
				*addr = ldbase + rel->r_addend;
				break;

			default:
				break;
		}
		rel = (Elf64_Rela*) ((char *) rel + relent);
		relsz -= relent;
	}
	systab->ConOut->OutputString(systab->ConOut, L"returning success\r\n");
	return EFI_SUCCESS;
}

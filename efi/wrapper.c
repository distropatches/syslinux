/*
 * Copyright (C) 2011 Intel Corporation; author Matt Fleming
 *
 * Wrap the ELF shared library in a PE32 (32bit) or PE32+ (64bit) suit.
 *
 * Syslinux plays some games with the ELF sections that are not easily
 * converted to a PE32 executable. For instance, Syslinux requires
 * that a symbol hash table be present (GNU hash or SysV) so that
 * symbols in ELF modules can be resolved at runtime but the EFI
 * firmware loader doesn't like that and refuses to load the file.
 *
 * We pretend that we have an EFI executable with a single .text
 * section so that the EFI loader will load it and jump to the entry
 * point. Once the Syslinux ELF shared object has control we can do
 * whatever we want.
 */
#include <linux/elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wrapper.h"

#if __SIZEOF_POINTER__ == 4
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Addr Elf_Addr;
#elif __SIZEOF_POINTER__ == 8
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Addr Elf_Addr;
#else
#error "unsupported architecture"
#endif

/*
 * 'so_size' is the file size of the ELF shared object.
 *  'class' dictates how the header is written
 * 	For 32bit machines (class == ELFCLASS32), the optional
 * 	header includes PE32 header fields
 * 	For 64bit machines (class == ELFCLASS64), the optional
 * 	header includes PE32+header fields
 */
static void write_header(FILE *f, __uint32_t entry, __uint32_t so_size, __uint8_t class)
{
	struct optional_hdr o_hdr;
	struct optional_hdr_pe32p o_hdr_pe32p;
	struct section t_sec, r_sec;
	struct extra_hdr e_hdr;
	struct extra_hdr_pe32p e_hdr_pe32p;
	struct coff_hdr c_hdr;
	struct header hdr;
	struct coff_reloc c_rel;
	__uint32_t total_sz = so_size;
	__uint32_t dummy = 0;
	__uint32_t hdr_sz;
	__uint32_t reloc_start, reloc_end;


	memset(&hdr, 0, sizeof(hdr));
	hdr.msdos_signature = MSDOS_SIGNATURE;

	/*
	 * The relocs table pointer needs to be >= 0x40 for PE files. It
	 * informs things like file(1) that we are not an MS-DOS
	 * executable.
	 */
	hdr.relocs_ptr = 0x40;

	hdr.pe_hdr = OFFSETOF(struct header, pe_signature);
	hdr.pe_signature = PE_SIGNATURE;
	fwrite(&hdr, sizeof(hdr), 1, f);

	memset(&c_hdr, 0, sizeof(c_hdr));
	c_hdr.nr_sections = 2;
	c_hdr.nr_syms = 1;
	if (class == ELFCLASS32) {
		hdr_sz = sizeof(o_hdr) + sizeof(t_sec) + sizeof(e_hdr) +
				sizeof(r_sec) + sizeof(c_hdr) + sizeof(hdr) + sizeof(c_rel)
				+ sizeof(dummy);
		total_sz += hdr_sz;
		entry += hdr_sz;
		c_hdr.arch = IMAGE_FILE_MACHINE_I386;
		c_hdr.characteristics = IMAGE_FILE_32BIT_MACHINE |
			IMAGE_FILE_DEBUG_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE |
			IMAGE_FILE_LINE_NUMBERS_STRIPPED;
		c_hdr.optional_hdr_sz = sizeof(o_hdr) + sizeof(e_hdr);
		fwrite(&c_hdr, sizeof(c_hdr), 1, f);
		memset(&o_hdr, 0, sizeof(o_hdr));
		o_hdr.format = PE32_FORMAT;
		o_hdr.major_linker_version = 0x02;
		o_hdr.minor_linker_version = 0x14;
		o_hdr.code_sz = total_sz;
		o_hdr.entry_point = entry;
		fwrite(&o_hdr, sizeof(o_hdr), 1, f);
		memset(&e_hdr, 0, sizeof(e_hdr));
		e_hdr.section_align = 4096;
		e_hdr.file_align = 512;
		e_hdr.image_sz = total_sz;
		e_hdr.headers_sz = 512;
		e_hdr.subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION;
		e_hdr.rva_and_sizes_nr = 1;
		fwrite(&e_hdr, sizeof(e_hdr), 1, f);
	}
	else if (class == ELFCLASS64) {
		hdr_sz = sizeof(o_hdr_pe32p) + sizeof(t_sec) + sizeof(e_hdr_pe32p) +
				sizeof(r_sec) + sizeof(c_hdr) + sizeof(hdr) + sizeof(c_rel)
				+ sizeof(dummy);
		total_sz += hdr_sz;
		entry += hdr_sz;
		c_hdr.arch = IMAGE_FILE_MACHINE_X86_64;
		c_hdr.characteristics = IMAGE_FILE_DEBUG_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE |
			IMAGE_FILE_LINE_NUMBERS_STRIPPED;
		c_hdr.optional_hdr_sz = sizeof(o_hdr_pe32p) + sizeof(e_hdr_pe32p);
		fwrite(&c_hdr, sizeof(c_hdr), 1, f);
		memset(&o_hdr_pe32p, 0, sizeof(o_hdr_pe32p));
		o_hdr_pe32p.format = PE32P_FORMAT;
		o_hdr_pe32p.major_linker_version = 0x02;
		o_hdr_pe32p.minor_linker_version = 0x14;
		o_hdr_pe32p.code_sz = total_sz;
		o_hdr_pe32p.entry_point = entry;
		fwrite(&o_hdr_pe32p, sizeof(o_hdr_pe32p), 1, f);
		memset(&e_hdr_pe32p, 0, sizeof(e_hdr));
		e_hdr_pe32p.section_align = 4096;
		e_hdr_pe32p.file_align = 512;
		e_hdr_pe32p.image_sz = total_sz;
		e_hdr_pe32p.headers_sz = 512;
		e_hdr_pe32p.subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION;
		e_hdr_pe32p.rva_and_sizes_nr = 1;
		fwrite(&e_hdr_pe32p, sizeof(e_hdr_pe32p), 1, f);
	}

	memset(&t_sec, 0, sizeof(t_sec));
	strcpy((char *)t_sec.name, ".text");
	t_sec.virtual_sz = total_sz;
	t_sec.raw_data_sz = total_sz;
	t_sec.characteristics = IMAGE_SCN_CNT_CODE |
		IMAGE_SCN_ALIGN_16BYTES | IMAGE_SCN_MEM_EXECUTE |
		IMAGE_SCN_MEM_READ;
	fwrite(&t_sec, sizeof(t_sec), 1, f);

	/*
	 * Write our dummy relocation and reloc section.
	 */
	memset(&r_sec, 0, sizeof(r_sec));
	strcpy((char *)r_sec.name, ".reloc");
	r_sec.virtual_sz = sizeof(c_rel);
	r_sec.virtual_address = ftell(f) + sizeof(r_sec);
	r_sec.raw_data_sz = r_sec.virtual_sz;
	r_sec.raw_data = r_sec.virtual_address;
	r_sec.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA |
		IMAGE_SCN_ALIGN_1BYTES | IMAGE_SCN_MEM_DISCARDABLE |
		IMAGE_SCN_MEM_READ;
	fwrite(&r_sec, sizeof(r_sec), 1, f);

	memset(&c_rel, 0, sizeof(c_rel));
	c_rel.virtual_address = ftell(f) + sizeof(c_rel);
	c_rel.symtab_index = 10;
	fwrite(&c_rel, sizeof(c_rel), 1, f);
	fwrite(&dummy, sizeof(dummy), 1, f);

}

static void usage(char *progname)
{
	fprintf(stderr,	"usage: %s <ELF shared object> <output file>\n",
		progname);
}

int main(int argc, char **argv)
{
	struct stat st;
	Elf32_Ehdr e32_hdr;
	Elf64_Ehdr e64_hdr;
	__uint32_t entry;
	__uint8_t class;
	unsigned char *id;
	FILE *f_in, *f_out;
	void *buf;
	size_t rv;

	if (argc < 3) {
		usage(argv[0]);
		exit(0);
	}

	f_in = fopen(argv[1], "r");
	if (!f_in) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	if (stat(argv[1], &st) != 0) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	f_out = fopen(argv[2], "w");
	if (!f_out) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	/*
	 * Parse the ELF header and find the entry point.
	 */
 	fread((void *)&e32_hdr, sizeof(e32_hdr), 1, f_in);
	if (e32_hdr.e_ident[EI_CLASS] == ELFCLASS32) {
		id = e32_hdr.e_ident;
		class = ELFCLASS32;
		entry = e32_hdr.e_entry;
	}
	else if (e32_hdr.e_ident[EI_CLASS] == ELFCLASS64) {
		/* read the header again for x86_64 
		 * note that the elf header entry point is 64bit whereas
		 * the entry point in PE/COFF format is 32bit!*/
		class = ELFCLASS64;
		rewind(f_in);
		fread((void *)&e64_hdr, sizeof(e64_hdr), 1, f_in);
		id = e64_hdr.e_ident;
		entry = e64_hdr.e_entry;
	} else {
		fprintf(stderr, "Unsupported architecture\n");
		exit(EXIT_FAILURE);
	}
		
	if (id[EI_MAG0] != ELFMAG0 ||
	    id[EI_MAG1] != ELFMAG1 ||
	    id[EI_MAG2] != ELFMAG2 ||
	    id[EI_MAG3] != ELFMAG3) {
		fprintf(stderr, "Input file not ELF shared object\n");
		exit(EXIT_FAILURE);
	}

	buf = malloc(st.st_size);
	if (!buf) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	write_header(f_out, entry, st.st_size, class);

	/* Write out the entire ELF shared object */
	rewind(f_in);
	rv = fread(buf, st.st_size, 1, f_in);
	if (!rv && ferror(f_in)) {
		fprintf(stderr, "Failed to read all bytes from input\n");
		exit(EXIT_FAILURE);
	}

	fwrite(buf, st.st_size, rv, f_out);
	return 0;
}

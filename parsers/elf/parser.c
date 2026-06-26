#include <elf.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

const unsigned char elfsig[4] = {
	0x7F, 'E', 'L', 'F'
};

int main(void)
{
	//file pointer.
	FILE *fp = fopen("hello", "rb");

	//read header.
	Elf64_Ehdr hdr;
	fread(&hdr, sizeof(hdr), 1, fp);

	//validate header.
	if(memcmp(hdr.e_ident, elfsig, 4) == 0)
	{
		printf("ELF detected\n");
	}

	//printing some fields.
	printf("Version : %04X\n", hdr.e_version);
	printf("Type : %04X\n", hdr.e_type);
	printf("Architecture : %04X\n", hdr.e_machine);
	printf("Obj File Type : %04X\n", hdr.e_type);
	printf("Virtual Entry Point Address: %08lX\n", hdr.e_entry);

	//program header offset.
	printf("\n___PROGRAM_HEADER___\n");
	printf("Table Entry Size : %04X\n", hdr.e_phentsize);
	printf("Table Entry Count : %04X\n", hdr.e_phnum);

	//program headers.
	if(fseek(fp, hdr.e_phoff, SEEK_SET) != 0) // moved to the program headers offset.
	{
		perror("Error seeking to program headers\n");
		fclose(fp);
		return 1;
	}

	//allocate and read program headers.
	size_t table_size = hdr.e_phnum * hdr.e_phentsize;
	Elf64_Phdr *phdr_table = malloc(table_size);
	if(!phdr_table)
	{
		perror("Memory allocation failed\n");
		fclose(fp);
		return 1;
	}

	if(fread(phdr_table, 1, table_size, fp) != table_size)
	{
		perror("Error reading program headers\n");
		free(phdr_table);
		fclose(fp);
		return 1;
	}

	//loop though each program header.
	printf("%-12s %-12s %-16s %-16s\n", "Type", "Offset", "VirtAddr", "FileSize");
    	printf("-------------------------------------------------------------\n");

    	for (int i = 0; i < hdr.e_phnum; i++) 
	{
        	// Handle different standard segment types
        	const char *type_str;
        	switch (phdr_table[i].p_type) 
		{
            		case PT_NULL:    type_str = "NULL"; break;
            		case PT_LOAD:    type_str = "LOAD"; break;
            		case PT_DYNAMIC: type_str = "DYNAMIC"; break;
            		case PT_INTERP:  type_str = "INTERP"; break;
            		case PT_NOTE:    type_str = "NOTE"; break;
            		case PT_PHDR:    type_str = "PHDR"; break;
            		case PT_TLS:     type_str = "TLS"; break;
            		default:         type_str = "UNKNOWN"; break;
        	}

        	printf("%-12s 0x%-10lx 0x%-14lx 0x%-14lx\n", 
               		type_str,
               		(unsigned long)phdr_table[i].p_offset,
               		(unsigned long)phdr_table[i].p_vaddr,
               		(unsigned long)phdr_table[i].p_filesz);
    	}

	//parse section headers.
	if(fseek(fp, hdr.e_shoff, SEEK_SET) != 0)// move fp to the program header section.
	{
		perror("error seeking program headers\n");
		fclose(fp);
		return 1;
	}

	printf("\n___SECTION_HEADERS___\n");
	//allocate and read section headers.
	size_t shtable_size = hdr.e_shnum * hdr.e_shentsize;
	Elf64_Shdr *shdr = malloc(shtable_size);
	if(fread(shdr, hdr.e_shentsize, hdr.e_shnum, fp) != hdr.e_shnum)
	{
		perror("Failed to read section headers");
		free(shdr);
		fclose(fp);
		return 1;
	}

	//get sec header string table.
	//shstrndx contains the strings that correspond to section names.
	Elf64_Shdr shstrndx_hdr = shdr[hdr.e_shstrndx];
	char *shstrtab = malloc(shstrndx_hdr.sh_size);
	fseek(fp, shstrndx_hdr.sh_offset, SEEK_SET);
	fread(shstrtab, 1, shstrndx_hdr.sh_size, fp);

	//loop through and print each sections details.
	printf("Number of Sections: %d\n", hdr.e_shnum);
    	printf("%-20s %-16s %-10s\n", "Section Name", "Address", "Size");
    	printf("------------------------------------------------\n");

    	for (int i = 0; i < hdr.e_shnum; i++) {
        	char *name = &shstrtab[shdr[i].sh_name];
        	printf("%-20s 0x%-14lx %-10lu\n", name, shdr[i].sh_addr, shdr[i].sh_size);
    	}

	//symtab details.
	printf("\n___SYMTAB_INFO___\n");
	fseek(fp, hdr.e_shoff + (hdr.e_shstrndx * hdr.e_shentsize), SEEK_SET);
    	Elf64_Shdr shstrtab_hdr;
    	fread(&shstrtab_hdr, sizeof(Elf64_Shdr), 1, fp);
    	fseek(fp, shstrtab_hdr.sh_offset, SEEK_SET);
    	fread(shstrtab, shstrtab_hdr.sh_size, 1, fp);

    	// Iterate through section headers to find the Symbol Table (.symtab) and String Table (.strtab)
    	Elf64_Shdr symtab_hdr;
    	Elf64_Shdr strtab_hdr;
    	int symtab_found = 0;

    	fseek(fp, hdr.e_shoff, SEEK_SET);
    	for (int i = 0; i < hdr.e_shnum; i++)
	{
        	Elf64_Shdr shdr;
        	fread(&shdr, sizeof(Elf64_Shdr), 1, fp);
        	char *name = &shstrtab[shdr.sh_name];
        	if (shdr.sh_type == SHT_SYMTAB)
		{
            		symtab_hdr = shdr;
            		symtab_found = 1;
        	}
		else if (shdr.sh_type == SHT_STRTAB && symtab_found && !symtab_hdr.sh_link) 
		{
            		// In a standard ELF, the symbol table header's `sh_link` points to its string table
            		// For simplicity, we just grab the first STRTAB section here
            		strtab_hdr = shdr;
        	}
    	}

    	if (!symtab_found)
	{
        	printf("No symbol table found.\n");
        	free(shstrtab);
        	fclose(fp);
        	return 1;
    	}

    	// Load the String Table
	char *strtab = malloc(strtab_hdr.sh_size);
    	fseek(fp, strtab_hdr.sh_offset, SEEK_SET);
    	fread(strtab, strtab_hdr.sh_size, 1, fp);

    	// Calculate the number of symbols
    	int num_syms = symtab_hdr.sh_size / sizeof(Elf64_Sym);
    	printf("Total Symbols: %d\n", num_syms);
    	printf("%-20s %-10s %-10s %s\n", "NAME", "VALUE", "SIZE", "TYPE");

    	// Read and print each symbol
    	fseek(fp, symtab_hdr.sh_offset, SEEK_SET);
    	for (int i = 0; i < num_syms; i++) 
	{
        	Elf64_Sym sym;
        	fread(&sym, sizeof(Elf64_Sym), 1, fp);

        	// Ignore symbols with no name
        	if (sym.st_name == 0) continue;

        	char *sym_name = &strtab[sym.st_name];
        	unsigned char type = ELF64_ST_TYPE(sym.st_info);

        	printf("%-20s %016lx %010lx ", sym_name, sym.st_value, sym.st_size);

       	 	switch (type) 
		{
            		case STT_NOTYPE:  printf("NOTYPE\n"); break;
            		case STT_OBJECT:  printf("OBJECT\n"); break;
            		case STT_FUNC:    printf("FUNC\n"); break;
            		case STT_SECTION: printf("SECTION\n"); break;
            		case STT_FILE:    printf("FILE\n"); break;
            		default:          printf("UNKNOWN\n"); break;
        	}
    	}

	printf("\n___DYNAMIC_SYMBOL_TABLE___\n");

	Elf64_Shdr *dynsym_hdr = NULL;
	Elf64_Shdr *dynstr_hdr = NULL;

	/* Locate .dynsym and .dynstr */
	for (int i = 0; i < hdr.e_shnum; i++)
	{
    		char *name = &shstrtab[shdr[i].sh_name];

    		if (strcmp(name, ".dynsym") == 0)
        		dynsym_hdr = &shdr[i];

    		else if (strcmp(name, ".dynstr") == 0)
        		dynstr_hdr = &shdr[i];
	}

	if (!dynsym_hdr || !dynstr_hdr)
	{
    		printf("No dynamic symbol table found.\n");
	}
	else
	{
    		char *dynstr = malloc(dynstr_hdr->sh_size);

    		fseek(fp, dynstr_hdr->sh_offset, SEEK_SET);
    		fread(dynstr, 1, dynstr_hdr->sh_size, fp);

    		int num_dynsym = dynsym_hdr->sh_size / sizeof(Elf64_Sym);

    		printf("%-30s %-18s %-10s %-10s\n",
           		"NAME",
           		"VALUE",
           		"SIZE",
           		"TYPE");

    		fseek(fp, dynsym_hdr->sh_offset, SEEK_SET);

    		for (int i = 0; i < num_dynsym; i++)
    		{
        		Elf64_Sym sym;

        		fread(&sym, sizeof(sym), 1, fp);

        		if (sym.st_name == 0)
            			continue;

        		char *symname = dynstr + sym.st_name;

        		unsigned char type = ELF64_ST_TYPE(sym.st_info);

        		printf("%-30s %016lx %-10lu ",
               			symname,
               			sym.st_value,
               			sym.st_size);

        		switch(type)
        		{
            			case STT_NOTYPE: printf("NOTYPE"); break;
            			case STT_OBJECT: printf("OBJECT"); break;
            			case STT_FUNC: printf("FUNC"); break;
            			case STT_SECTION: printf("SECTION"); break;
            			case STT_FILE: printf("FILE"); break;
            			case STT_COMMON: printf("COMMON"); break;
            			case STT_TLS: printf("TLS"); break;
            			default: printf("UNKNOWN");
        		}

        		printf("\n");
    		}

    		free(dynstr);
	}

	free(shdr);
	free(shstrtab);
	free(phdr_table);

	fclose(fp);

	return 0;
}

#include <sys/mman.h>   /* For mmap flags */
#include <fcntl.h>      /* For file open flags */
#include <unistd.h>     /* For SEEK_SET, etc. */


#define SYS_WRITE 4
#define SYS_READ 3
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_LSEEK 19

#define O_RDWR 2
#define O_RDONLY 0
#define SEEK_END 2
#define SEEK_SET 0

#define STDOUT 1

#define Elf32_Off  unsigned int
#define Elf32_Addr unsigned int
#define Elf32_Word unsigned int
#define Elf32_Half unsigned short

/* The ELF header structure */
typedef struct {
    unsigned char e_ident[16]; /* ELF Identification */
    Elf32_Half e_type;         /* Object file type */
    Elf32_Half e_machine;      /* Machine type */
    Elf32_Word e_version;      /* Object file version */
    Elf32_Addr e_entry;        /* Entry point address */
    Elf32_Off  e_phoff;        /* Program header offset */
    Elf32_Off  e_shoff;        /* Section header offset */
    Elf32_Word e_flags;        /* Processor-specific flags */
    Elf32_Half e_ehsize;       /* ELF header size */
    Elf32_Half e_phentsize;    /* Size of program header entry */
    Elf32_Half e_phnum;        /* Number of program header entries */
    Elf32_Half e_shentsize;    /* Size of section header entry */
    Elf32_Half e_shnum;        /* Number of section header entries */
    Elf32_Half e_shstrndx;     /* Section name string table index */
} Elf32_Ehdr;

/* The ELF program header structure */
typedef struct {
    Elf32_Word p_type;   /* Entry type (e.g., PT_LOAD = 1) */
    Elf32_Off  p_offset; /* File offset */
    Elf32_Addr p_vaddr;  /* Virtual address */
    Elf32_Addr p_paddr;  /* Physical address */
    Elf32_Word p_filesz; /* File size */
    Elf32_Word p_memsz;  /* Memory size */
    Elf32_Word p_flags;  /* Flags (e.g., R/W/E) */
    Elf32_Word p_align;  /* Memory/file alignment */
} Elf32_Phdr;


extern int system_call(int syscall_num, ...);

static unsigned int my_strlen(const char* str) {
    unsigned int len = 0;
    while (str[len]) len++;
    return len;
}

static void write_str(const char* str) {
    unsigned int len = my_strlen(str);
    system_call(SYS_WRITE, STDOUT, (int)str, len);
}

static void print_address_hex(unsigned int addr) {
    char buffer[12];
    buffer[0] = '0';
    buffer[1] = 'x';

    int i;
    for (i = 9; i >= 2; i--) {
        int digit = addr & 0xF;
        buffer[i] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        addr >>= 4;
    }
    buffer[10] = ' '; 
    buffer[11] = '\0';
    write_str(buffer);
}

int foreach_phdr(void* map_start, void (*func)(Elf32_Phdr*, int), int arg) {
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*)map_start;

    if (elf_header->e_ident[0] != 0x7f ||
        elf_header->e_ident[1] != 'E' ||
        elf_header->e_ident[2] != 'L' ||
        elf_header->e_ident[3] != 'F') {
        write_str("Error: Not a valid ELF file.\n");
        return 1;
    }

    Elf32_Phdr* phdr_table = (Elf32_Phdr*)((char*)map_start + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        func(&phdr_table[i], arg);
    }
    return 0;
}

static const char* phdr_type_to_str(Elf32_Word p_type) {
    switch (p_type) {
        case 0: return "NULL";
        case 1: return "LOAD";
        case 2: return "DYNAMIC";
        case 3: return "INTERP";
        case 4: return "NOTE";
        case 5: return "SHLIB";
        case 6: return "PHDR";
        case 7: return "TLS";
        case 0x70000000: return "LOPROC";
        case 0x7fffffff: return "HIPROC";
        default: return "UNKNOWN";
    }
}

static void flags_to_str(Elf32_Word p_flags, char* out) {
    int idx = 0;
    if (p_flags & 4) out[idx++] = 'R';
    if (p_flags & 2) out[idx++] = 'W';
    if (p_flags & 1) out[idx++] = 'E';
    out[idx] = '\0';
}

void print_phdr_readelf(Elf32_Phdr* phdr, int _) {
    const char* ptype = phdr_type_to_str(phdr->p_type);

    char flags[4];
    flags_to_str(phdr->p_flags, flags);

    write_str(ptype);
    write_str(" ");

    print_address_hex(phdr->p_offset);
    print_address_hex(phdr->p_vaddr);
    print_address_hex(phdr->p_paddr);
    print_address_hex(phdr->p_filesz);
    print_address_hex(phdr->p_memsz);

    write_str(flags);
    write_str(" ");

    print_address_hex(phdr->p_align);
    write_str("\n");
}


static void print_phdr_readelf_and_mmap(Elf32_Phdr* phdr, int _) {
    print_phdr_readelf(phdr, _);

    int prot = 0;
    if (phdr->p_flags & 4) prot |= PROT_READ;
    if (phdr->p_flags & 2) prot |= PROT_WRITE;
    if (phdr->p_flags & 1) prot |= PROT_EXEC;

    write_str("Derived mmap prot flags: ");

    print_address_hex((unsigned int)prot);

    write_str("Derived mapping flags: MAP_PRIVATE | MAP_FIXED\n\n");
}

void load_phdr(Elf32_Phdr *phdr, int fd) {

    if (phdr->p_type != 1) {
        return;
    }

    print_phdr_readelf(phdr, 0);

    int prot = 0;
    if (phdr->p_flags & 4) prot |= PROT_READ;   
    if (phdr->p_flags & 2) prot |= PROT_WRITE;  
    if (phdr->p_flags & 1) prot |= PROT_EXEC;   

    unsigned int page_size = 0x1000;
    unsigned int aligned_offset = phdr->p_offset & 0xfffff000;
    unsigned int aligned_vaddr = phdr->p_vaddr & 0xfffff000;
    unsigned int padding = phdr->p_vaddr & 0xfff;
    unsigned int map_size = phdr->p_filesz + padding;

    void* mapped = mmap((void*)aligned_vaddr, map_size, prot, MAP_PRIVATE | MAP_FIXED, fd, aligned_offset);

    if (mapped == (void*)-1) {
        write_str("Error: mmap failed for segment.\n");
        return;
    }

    write_str("Segment mapped successfully.\n");
}

extern int startup(int argc, char** argv, void (*start)());

int main(int argc, char* argv[], char* envp[]) {
    if (argc < 2) {
        write_str("Usage: ./loader <elf_file> [args...]\n");
        return 1;
    }

    int fd = system_call(SYS_OPEN, (int)argv[1], 0, O_RDWR); 
    if (fd < 0) {
        write_str("Error: Failed to open file.\n");
        return 1;
    }

    int file_size = system_call(SYS_LSEEK, fd, 0, SEEK_END);
    system_call(SYS_LSEEK, fd, 0, SEEK_SET);

    void* map_start = mmap((void*)0, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == (void*)-1) {
        write_str("Error: Failed to map ELF file.\n");
        system_call(SYS_CLOSE, fd, 0, 0);
        return 1;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)map_start;
    foreach_phdr(map_start, load_phdr, fd);

    startup(argc-1, argv+1, (void*)(ehdr->e_entry));
    system_call(SYS_CLOSE, fd, 0, 0);

    return 0;
}

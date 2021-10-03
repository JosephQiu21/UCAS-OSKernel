#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define OS_SIZE_LOC 0x01fc

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph = 0;     // Program header counter
    int nbytes = 0; // Writing bytes counter
    int first = 1;  // Segment counter
    int is_bootblock = 1;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* create a new image file */
    img = fopen("image", "w+");
    /* for each input file */
    while (nfiles-- > 0) {

        /* open input file */
        fp = fopen(*files, "r+");
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            printf("\tsegment %d\n", ph);

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;

        // Write kernel size
        if(!is_bootblock) write_os_size(nbytes, img);
        else is_bootblock = 0;

        nbytes = 0;
    }
    fclose(img);
}

// read ELF header 
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    if(!fread(ehdr, sizeof(Elf64_Ehdr), 1, fp)){
        error("Warning: Format of input file is not ELF64\n");
    }
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    if(!fread(phdr, sizeof(Elf64_Phdr), 1, fp)){
        error("Warning: Fail to read program header\n");
    }
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
    int num_segment = (phdr.p_filesz + 511)/512;    // Number of segments
    if(options.extended){
        printf("\t\toffset 0x%lx\t\tvaddr 0x%lx\n", phdr.p_offset, phdr.p_vaddr);
        printf("\t\tfilesz 0x%lx\t\tmemsz 0x%lx\n", phdr.p_filesz, phdr.p_memsz);
    }

    // Read program header into buffer
    fseek(fp, phdr.p_offset, SEEK_SET);
    char *buffer = (char *)malloc(num_segment * 512 * sizeof(char));
    memset(buffer, 0, num_segment * 512 * sizeof(char));
    fread(buffer, phdr.p_filesz, 1, fp);

    // Write buffer into image
    fseek(img, (*first - 1) * 512, SEEK_SET);
    fwrite(buffer, num_segment * 512, 1, img);

    // Update nbytes and first
    *nbytes += num_segment * 512;
    *first += num_segment;
 
    if(phdr.p_filesz && options.extended == 1){
        printf("\t\twriting 0x%lx bytes\n", phdr.p_filesz);
        printf("\t\tpadding up to 0x%x\n", (*first - 1) * 512);
    }
    
}

static void write_os_size(int nbytes, FILE * img)
{
    // write os size into the last 4 bytes of the 1st segment
    int kernel_size = nbytes/512;
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    char buffer[2] = {kernel_size & 0xff, (kernel_size >> 8) & 0xff};
    fwrite(buffer, 1, 2, img);
    if(options.extended) printf("os_size: %d sectors\n", nbytes/512);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."


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
    int ph, nbytes = 0, first = 1;
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

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes, img);
    fclose(img);
}

// read ELF header 
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(&ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fread(&phdr, sizeof(Elf64_Phdr), 1, fp + sizeof(Elf64_Ehdr) + ph * sizeof(Elf64_Phdr));
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
    if(*first == 1){
        fwrite(fp + phdr.p_offset, phdr.p_filesz, 1, img);
        fwrite(0, phdr.p_memsz - phdr.p_filesz, 1, img + phdr.p_filesz);
        *first = 0;
        *nbytes += 512;
        if(options.extended){
            printf("0x50200000: bootblock\n");
            printf("\tsegment 0\n");
            printf("\t\toffset 0x%041x\tvaddr 0x50200000\n", phdr.p_offset);
            printf("\t\tfilesz 0x%041x\tmemsz 0x%041x\n", phdr.p_filesz, phdr.p_memsz);
            printf("\t\twriting 0x%041x bytes\n", phdr.p_memsz);
            printf("\t\tpadding up to 0x%041x\n", 512);
            printf("0x50201000: kernel\n");
        }
    }else{
        fwrite(fp + phdr.p_offset, phdr.p_filesz, 1, img + *nbytes);
        fwrite(0, phdr.p_memsz - phdr.p_filesz, 1, img + *nbytes + phdr.p_filesz);
        if(options.extended){
            printf("\tsegment %d\n", *nbytes/512);
            printf("\t\toffset 0x%041x\tvaddr 0x%081x\n", phdr.p_offset, 1344278528 + *nbytes);
            printf("\t\tfilesz 0x%041x\tmemsz 0x%041x\n", phdr.p_filesz, phdr.p_memsz);
            printf("\t\twriting 0x%041x bytes\n", phdr.p_memsz);
            printf("\t\tpadding up to 0x%041x\n", 1024 + *nbytes);
        }
        *nbytes += 512;
    }
    printf("os_size: %d sectors\n", *nbytes);
}

static void write_os_size(int nbytes, FILE * img)
{
    // write os size into the last 4 bytes of the 1st section
    fwrite(&nbytes, 2, 1, img + 508);
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

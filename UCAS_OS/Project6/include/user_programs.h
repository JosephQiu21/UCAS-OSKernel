
extern unsigned char _elf___test_test_shell_elf[];
int _length___test_test_shell_elf;
extern unsigned char _elf___test_rw_elf[];
int _length___test_rw_elf;
extern unsigned char _elf___test_fly_elf[];
int _length___test_fly_elf;
extern unsigned char _elf___test_lock_elf[];
int _length___test_lock_elf;
extern unsigned char _elf___test_mailbox_elf[];
int _length___test_mailbox_elf;
extern unsigned char _elf___test_recv_elf[];
int _length___test_recv_elf;
extern unsigned char _elf___test_send_elf[];
int _length___test_send_elf;
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 7
extern ElfFile elf_files[7];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);

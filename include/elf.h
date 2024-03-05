#ifndef _ELF_H
#define _ELF_H

#define ELF_MAGIC 0x464C457FU  //elf头的魔数用来判断是不是这个格式
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* 32位elf头 */
struct Elf32_Ehdr {
   unsigned char e_ident[16];
   Elf32_Half    e_type;
   Elf32_Half    e_machine;
   Elf32_Word    e_version;
   Elf32_Addr    e_entry;
   Elf32_Off     e_phoff;
   Elf32_Off     e_shoff;
   Elf32_Word    e_flags;
   Elf32_Half    e_ehsize;
   Elf32_Half    e_phentsize;
   Elf32_Half    e_phnum;
   Elf32_Half    e_shentsize;
   Elf32_Half    e_shnum;
   Elf32_Half    e_shstrndx;
};

/* 程序头表Program header.就是段描述头 */
struct Elf32_Phdr {
   Elf32_Word p_type;		 // 见下面的enum segment_type
   Elf32_Off  p_offset;
   Elf32_Addr p_vaddr;
   Elf32_Addr p_paddr;
   Elf32_Word p_filesz;
   Elf32_Word p_memsz;
   Elf32_Word p_flags;
   Elf32_Word p_align;
};

/* 段类型 */
enum segment_type {
   PT_NULL,            // 忽略
   PT_LOAD,            // 可加载程序段
   PT_DYNAMIC,         // 动态加载信息 
   PT_INTERP,          // 动态加载器名称
   PT_NOTE,            // 一些辅助信息
   PT_SHLIB,           // 保留
   PT_PHDR             // 程序头表
};


#endif


//elf头
//struct elf_header {
//  uint magic;  
//  uchar elf[12];
//  ushort type;
//  ushort machine;
//  uint version;
//  uint entry;
//  uint phoff;
//  uint shoff;
//  uint flags;
//  ushort ehsize;
//  ushort phentsize;
//  ushort phnum;
//  ushort shentsize;
//  ushort shnum;
//  ushort shstrndx;
//};
//
////程序头
//struct prog_header {
//  uint type;
//  uint off;
//  uint vaddr;
//  uint paddr;
//  uint filesz;
//  uint memsz;
//  uint flags;
//  uint align;
//};

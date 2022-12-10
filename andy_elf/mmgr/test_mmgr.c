#include <stdio.h>
#include "include/sys/mmgr.h"
#include <string.h>

#if __WORDSIZE == 64
typedef uint64_t vaddr;
#else
typedef uint32_t vaddr;
#endif

/**
Tests for the mmgr module. Note that this MUST be compiled with -m32 in order for the
pointer arithmetic to work.
*/

int main(int argc, char *argv[]) {
  int failures =0;
  int successes=0;

  puts("find_next_unallocated_page should return the next free page in the first directory, if that's where it is.");
  if(test_find_next_unallocated_basic()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("find_next_unallocated_page should go into more directories if it needs to.");
  if(test_find_next_unallocated_multipage()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("find_next_unallocated_page should return an error if nothing is available.");
  if(test_find_next_unallocated_nomem()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("k_map_if_required should return a new mapping if the given physical block is not present");
  if(test_k_map_if_required_not_present()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("k_map_if_required should return the existing mapping if the given physical block is present");
  if(test_k_map_if_required_present()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("vm_map_next_unallocated_pages should map the first available continuous block to the given list of pointers");
  if(test_vm_map_next_unallocated_pages()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("vm_map_next_unallocated_pages should return null of not enough pages could be found to the given list of pointers");
  if(test_vm_map_next_unallocated_pages_nomem()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  puts("vm_map_next_unallocated_pages should return null if pages=0");
  if(test_vm_map_next_unallocated_pages_inval()==0) {
    puts("FAILED\n");
    ++failures;
  } else {
    puts("SUCCESS\n");
    ++successes;
  }

  printf("%d test successes and %d failures\n", successes, failures);
  if(failures>1) return 1;
  return 0;
}

/*
find_next_unallocated_page should return the next free page in the first directory, if that's where it is.
*/
int test_find_next_unallocated_basic() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) first_entry[1024];

  memset(fake_directory, 0, 4096);
  memset(first_entry, 0, 4096);

  fake_directory[0] = ((uint32_t)&first_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  first_entry[0] = MP_PRESENT;
  first_entry[1] = MP_PRESENT;

  int16_t dir=0;
  int16_t off=0;
  int8_t result = find_next_unallocated_page(&fake_directory, &dir, &off);
  if(result!=0) {
    puts("find_next_unallocated_page returned unexpected failure on test 1\n");
    return 0;
  }

  if(dir!=0) {
    printf("find_next_unallocated_page returned unexpected directory number %d\n", dir);
    return 0;
  }

  if(off!=2) {
    printf("find_next_unallocated_page returned unexpected offset %d\n", off);
    return 0;
  }
  return 1;
}

/*
find_next_unallocated_page should go into more directories if it needs to.
*/
int test_find_next_unallocated_multipage() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) first_entry[1024];
  uint32_t __attribute__ ((aligned(4096))) second_entry[1024];
  uint32_t __attribute__ ((aligned(4096))) third_entry[1024];

  memset(fake_directory, 0, 4096);
  memset(first_entry, 0, 4096);
  memset(second_entry, 0, 4096);
  memset(third_entry, 0, 4096);

  fake_directory[0] = ((uint32_t)&first_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  fake_directory[1] = ((uint32_t)&second_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  fake_directory[2] = ((uint32_t)&third_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;

  for(register int i=0; i<1024; i++) {
    first_entry[i] = MP_PRESENT | MP_READWRITE;
    second_entry[i] = MP_PRESENT | MP_READWRITE;
  }

  for(register int i=0;i<301;i++) {
    third_entry[i] = MP_PRESENT | MP_READWRITE;
  }

  for(register int i=306;i<1024;i++) {
    third_entry[i] = MP_PRESENT | MP_READWRITE;
  }

  int16_t dir=0;
  int16_t off=0;
  int8_t result = find_next_unallocated_page(&fake_directory, &dir, &off);
  if(result!=0) {
    puts("find_next_unallocated_page returned unexpected failure on test 1\n");
    return 0;
  }

  if(dir!=2) {
    printf("find_next_unallocated_page returned unexpected directory number %d\n", dir);
    return 0;
  }

  if(off!=301) {
    printf("find_next_unallocated_page returned unexpected offset %d\n", off);
    return 0;
  }
  return 1;
}

/*
find_next_unallocated_page should return an error if nothing is available.
*/
int test_find_next_unallocated_nomem() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) pages[1024][1024];

  memset(fake_directory, 0, 4096);

  for(register int i=0; i<1024; i++) {
    fake_directory[i] = ((uint32_t)&pages[i] & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  }

  //fill the entire directory
  for(register int i=0; i<1024; i++) {
    for(register int j=0; j<1024; j++) {
      pages[i][j]= MP_PRESENT | MP_READWRITE;
    }
  }


  int16_t dir=0;
  int16_t off=0;
  int8_t result = find_next_unallocated_page(&fake_directory, &dir, &off);
  if(result!=1) {
    puts("find_next_unallocated_page returned unexpected success on no mem test\n");
    return 0;
  }

  //dir and off should be unchanged
  if(dir!=0) {
    printf("find_next_unallocated_page returned unexpected directory number %d\n", dir);
    return 0;
  }

  if(off!=0) {
    printf("find_next_unallocated_page returned unexpected offset %d\n", off);
    return 0;
  }
  return 1;
}

/*
k_map_if_required should return a new mapping if the given physical block is not present
*/
int test_k_map_if_required_not_present() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) first_entry[1024];

  uint32_t phys_ptr = 0x8A49000;

  memset(fake_directory, 0, 4096);  //each entry is 4 bytes and memset works in bytes
  memset(first_entry, 0, 4096);

  fake_directory[0] = ((uint32_t)&first_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  for(register int i=0; i<421; i++) {
    if(i!=123) first_entry[i] = MP_PRESENT; //should map at the empty page on 123
  }

  uint32_t result = (uint32_t) k_map_if_required(&fake_directory, (void *)phys_ptr, MP_READWRITE);
  if(result!=123*4096) {
    printf("k_map_if_required returned unexpected pointer value %x, expected %x\n", result, 123*4096);
    return 0;
  }

  if(first_entry[123] != (phys_ptr|MP_PRESENT|MP_READWRITE)) {
    printf("k_map_if_required did not insert physical pointer in the right place, got %x expected %x\n", first_entry[123], phys_ptr|MP_PRESENT|MP_READWRITE);
    return 0;
  }

  return 1;
}

/*
k_map_if_required should return the existing mapping if the given physical block is present
*/
int test_k_map_if_required_present() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) first_entry[1024];

  uint32_t phys_ptr = 0x8A49000;

  memset(fake_directory, 0, 4096);  //each entry is 4 bytes and memset works in bytes
  memset(first_entry, 0, 4096);

  fake_directory[0] = ((uint32_t)&first_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  for(register int i=0; i<421; i++) {
    if(i==123){
      first_entry[i] = phys_ptr|MP_PRESENT|MP_READWRITE;
    } else if(i<40 || i>50) { //make sure a blank hole is left to ensure that a map has not taken place.
      first_entry[i] = MP_PRESENT;
    }
  }

  uint32_t result = (uint32_t) k_map_if_required(&fake_directory, (void *)phys_ptr, MP_READWRITE);
  if(result!=123*4096) {
    printf("k_map_if_required returned unexpected pointer value 0x%x, expected 0x%x\n", result, 123*4096);
    return 0;
  }

  if(first_entry[123] != (phys_ptr|MP_PRESENT|MP_READWRITE)) {
    printf("k_map_if_required corrupted the memory map, got %x expected %x\n", first_entry[123], phys_ptr|MP_PRESENT|MP_READWRITE);
    return 0;
  }

  return 1;
}

/*
vm_map_next_unallocated_pages should map the first available continuous block
to the given list of pointers
*/
int test_vm_map_next_unallocated_pages() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) pages[1024][1024];

  vaddr ptr_list[255];
  for(register int i=0; i<255; i++) {
    ptr_list[i] = (vaddr) (0xFF000000 | (i*2*0x1000));  //set up a discontinuous set of physical pointers
  }

  memset(fake_directory, 0, 4096);

  for(register int i=0; i<1024; i++) {
    fake_directory[i] = ((vaddr)&pages[i] & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  }

  //fill most of the directory
  for(register int i=0; i<1024; i++) {
    for(register int j=0; j<1024; j++) {
      if(i!=j) pages[i][j]= MP_PRESENT | MP_READWRITE;  //put some empty blocks in if i=j
    }
  }

  const int target_page=482;
  const int target_start_entry=86;

  for(register int i=0; i<258; i++) {
    pages[target_page][i+target_start_entry] = 0;     //empty a block of 255 pages from entry 86 of dir 482
  }

  vaddr returned_ptr = (vaddr) vm_map_next_unallocated_pages(&fake_directory, MP_READWRITE, (void **)&ptr_list, 255);
  vaddr expected_ptr = (target_page * 0x400000) + (target_start_entry * 0x1000);

  if(returned_ptr != expected_ptr) {
    printf("vm_map_next_unallocated_pages returned unexpected pointer value, expected 0x%x got 0x%x\n", expected_ptr, returned_ptr);
    return 0;
  }

  uint8_t did_fail=0;
  for(register int i=0; i<255; i++) {
    if( (pages[target_page][i+target_start_entry] & MP_ADDRESS_MASK) != ptr_list[i]) {
      printf("vm_map_next_unallocated_pages mapped page %d incorrectly, expected address 0x%x got 0x%x\n", i, ptr_list[i], (pages[target_page][i+target_start_entry] & MP_ADDRESS_MASK) );
      did_fail = 1;
    }
    if( ! (pages[target_page][i+target_start_entry] & MP_PRESENT)) {
      printf("vm_map_next_unallocated_pages mapped page %d incorrectly, 'present' flag not set\n", i);
      did_fail = 1;
    }
    if( ! (pages[target_page][i+target_start_entry] & MP_READWRITE)) {
      printf("vm_map_next_unallocated_pages mapped page %d incorrectly, 'read-write' flag not set\n", i);
      did_fail = 1;
    }
    if(did_fail) break;
  }

  return !did_fail;
}

/*
vm_map_next_unallocated_pages should return null of not enough pages could be found
to the given list of pointers
*/
int test_vm_map_next_unallocated_pages_nomem() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) pages[1024][1024];

  vaddr ptr_list[255];
  for(register int i=0; i<255; i++) {
    ptr_list[i] = (vaddr) (0xFF000000 | (i*2*0x1000));  //set up a discontinuous set of physical pointers
  }

  memset(fake_directory, 0, 4096);

  for(register int i=0; i<1024; i++) {
    fake_directory[i] = ((vaddr)&pages[i] & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  }

  //fill most of the directory
  for(register int i=0; i<1024; i++) {
    for(register int j=0; j<1024; j++) {
      if(i!=j) pages[i][j]= MP_PRESENT | MP_READWRITE;  //put some empty blocks in if i=j
    }
  }

  const int target_page=482;
  const int target_start_entry=86;

  vaddr returned_ptr = (vaddr) vm_map_next_unallocated_pages(&fake_directory, MP_READWRITE, (void **)&ptr_list, 255);
  vaddr expected_ptr = 0;

  if(returned_ptr != expected_ptr) {
    printf("vm_map_next_unallocated_pages returned unexpected pointer value, expected 0x%x got 0x%x\n", expected_ptr, returned_ptr);
    return 0;
  }

  return 1;
}

/*
vm_map_next_unallocated_pages should return null if pages=0
*/
int test_vm_map_next_unallocated_pages_inval() {
  uint32_t __attribute__ ((aligned(4096))) fake_directory[1024];
  uint32_t __attribute__ ((aligned(4096))) pages[1024][1024];

  vaddr ptr_list[255];
  for(register int i=0; i<255; i++) {
    ptr_list[i] = (vaddr) (0xFF000000 | (i*2*0x1000));  //set up a discontinuous set of physical pointers
  }

  memset(fake_directory, 0, 4096);

  for(register int i=0; i<1024; i++) {
    fake_directory[i] = ((vaddr)&pages[i] & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;
  }

  //fill most of the directory
  for(register int i=0; i<1024; i++) {
    for(register int j=0; j<1024; j++) {
      if(i!=j) pages[i][j]= MP_PRESENT | MP_READWRITE;  //put some empty blocks in if i=j
    }
  }

  const int target_page=482;
  const int target_start_entry=86;

  for(register int i=0; i<258; i++) {
    pages[target_page][i+target_start_entry] = 0;     //empty a block of 255 pages from entry 86 of dir 482
  }

  vaddr returned_ptr = (vaddr) vm_map_next_unallocated_pages(&fake_directory, MP_READWRITE, (void **)&ptr_list, 0);
  vaddr expected_ptr = 0;

  if(returned_ptr != expected_ptr) {
    printf("vm_map_next_unallocated_pages returned unexpected pointer value, expected 0x%x got 0x%x\n", expected_ptr, returned_ptr);
    return 0;
  }

  return 1;
}

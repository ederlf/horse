/* Simple public domain implementation of the standard CRC32 checksum.
 * Outputs the checksum for each file given as a command line argument.
 * Invalid file names and files that cause errors are silently skipped.
 * The program reads from stdin if it is called with no arguments. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t crc32_for_byte(uint32_t r) {
  int j;
  for(j = 0; j < 8; ++j)
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
  static uint32_t table[0x100];
  size_t i;
  if(!*table)
    for(i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for(i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

uint32_t calculate_hash(void){
  uint8_t buf[14];
  uint32_t crc = 0;
  memset(buf, 0x1, 14);
  crc32(buf, 14, &crc);
  return crc;
}

int main(int ac, char** av) {
  
  uint32_t crc =  calculate_hash();
  printf("%x\n", crc);
  return 0;
}

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#define MAX_SIZE 10000000

struct message
{
	uint16_t op;
	uint16_t chksum;
	char keyword[4];
	uint64_t length;
	char data[MAX_SIZE-16];
};

uint16_t ip_checksum(char* vdata,size_t length, int mode) {
    
  //Initialise the accumulator.
  uint32_t acc=0x0000;

  // Handle complete 16-bit blocks.
  for (size_t i=0;i+1<length;i+=2) {
      uint16_t word;
      memcpy(&word, vdata+i,2);
      acc+=word;
      if (acc>0xffff) {
          acc-=0xffff;
      }
  }

  // Handle any partial block at the end of the data.
  if (length&1) {
      uint16_t word=0;
      memcpy(&word,vdata+length-1,1);
      acc+=word;
      if (acc>0xffff) {
          acc-=0xffff;
      }
  }

  if(mode == 0)
    return (uint16_t)~acc;
  else
    return (uint16_t)acc;
};

#endif

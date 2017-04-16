#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int windowSize = 4096;

static int match(char* windowStart, char* windowEnd, char* input, char* inputEnd, char** matchPos) {
  int bestLength = 0;

  while(windowStart < windowEnd) {
    int currentMatch = 0; 

    while(windowStart+currentMatch < windowEnd && input+currentMatch < inputEnd &&
      windowStart[currentMatch] == input[currentMatch] ) {
      currentMatch++;
    }

    if (currentMatch >= bestLength) {
      bestLength = currentMatch;
      *matchPos = windowStart;
    }
    windowStart++;
  }

  return bestLength;
}

int compress(char* inputStart, int inputLength, char* outputStart, int outputLength) {
  char* input = inputStart;
  char* inputEnd = input + inputLength;
  char* output = outputStart;
  *(int*)output = inputLength;
  output += sizeof(int);
  
  while(input < inputEnd) {
    char* windowStart = input-windowSize < inputStart ? inputStart : input-windowSize;
    char* matchPos = NULL;
    int matchLength = match(windowStart, input, input, inputEnd, &matchPos);

    if (matchLength > 1) {
      *output++ = matchLength;
      *(short*)output = (short)(input-matchPos);
      output += sizeof(short);
      input += matchLength;
    } else {
      *output++ = 0;
      *output++ = *input;
      input++;
    }
  }
  return (int)(output-outputStart);
}

int decompress(char* input, int inputLength, char** outputRef) {
  char* inputEnd = input + inputLength;
  int uncompressedLength = *(int*)input;
  input += sizeof(int);
  *outputRef = malloc(uncompressedLength);
  char* output = *outputRef;

  while(input < inputEnd) {
    if (*input == 0) { 
      *output = input[1];
      output++;
      input += 2;
    } else {
      char len = *input++;
      short offset = *(short*)input;
      input += sizeof(short);
      char* start = output - offset;

      while(len > 0) {
        *output++ = *start++;
        len--;
      }
    }
  }

  return 0;
}

int main(int argc, char** argv) {
  char* buf = malloc(4096);
  char* str = "AAABABCABCDEABCDEFGH";
  printf("%s\n", str);
  int result = compress(str, strlen(str)+1, buf, 4096);
  printf("%d\n", result);
  for(int i = 0; i<result; i++) {
    if (buf[i] < 32) {
      printf("%02d ", buf[i]);
    } else {
      printf("%c ", buf[i]);
    }
  }
  printf("\n");
  char* decompressed = NULL;
  int decompressedLength = decompress(buf, result, &decompressed);
  printf("%s\n", decompressed);
  return 0;
}

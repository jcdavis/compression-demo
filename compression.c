#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int windowSize = 4096;
static int minMatchLength = 4;
static int maxMatchLength = 0x7f;
static int maxLiteralChainLength = 0x7f;

static int match(char* windowStart, char* windowEnd, char* input, char* inputEnd, char** matchPos) {
  int bestLength = 0;

  while(windowStart < windowEnd) {
    int currentMatch = 0; 

    while(windowStart+currentMatch < windowEnd && input+currentMatch < inputEnd &&
      windowStart[currentMatch] == input[currentMatch] && currentMatch < maxMatchLength) {
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
  char* outputEnd = output + outputLength;
  char* activeLiteral = NULL;

  *(int*)output = inputLength;
  output += sizeof(int);
  
  while(input < inputEnd) {
    char* windowStart = input-windowSize < inputStart ? inputStart : input-windowSize;
    char* matchPos = NULL;
    int matchLength = match(windowStart, input, input, inputEnd, &matchPos);
    
    if (matchLength >= minMatchLength) {
      activeLiteral = NULL;
      *output++ = matchLength | 0x80;
      *(unsigned short*)output = (unsigned short)(input-matchPos);
      output += sizeof(unsigned short);
      input += matchLength;
    } else {
      if (activeLiteral == NULL || *activeLiteral == maxLiteralChainLength) {
        activeLiteral = output;
        *output++ = 0;
      }
      (*activeLiteral)++;
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
    if ((*input & 0x80) == 0) {
      unsigned char length = *input++;
      while (length > 0) {
        *output++ = *input++;
        length--;
      }
    } else {
      unsigned char length = *input++ & 0x7f;
      unsigned short offset = *(unsigned short*)input;
      input += sizeof(unsigned short);
      char* start = output - offset;

      while(length > 0) {
        *output++ = *start++;
        length--;
      }
    }
  }
  return (int)(output-*outputRef);
}

void roundtrip(char* string) {
  char* compressed = malloc(4096);
  char* decompressed = NULL;
  int compressedLength = compress(string, strlen(string)+1, compressed, 4096);
  decompress(compressed, compressedLength, &decompressed);

  printf("%s\n", string);
  printf("%d\n", compressedLength);
  for(int i = 0; i < compressedLength; i++) {
    if (compressed[i] < 32) {
      printf("%02d ", compressed[i]);
    } else {
      printf("%c ", compressed[i]);
    }
  }
  printf("\n");
  printf("%s\n", decompressed);
  free(compressed);
  free(decompressed);
}

int main(int argc, char** argv) {
  if (argc != 4 || (strcmp(argv[1],"-c") != 0 && strcmp(argv[1], "-d") != 0)) {
    printf("Unexpected arguments. Format is:\ncompression-demo -c/-d inputfile outputfile\n");
    return 1;
  }
  int bufferSize = 640000;
  FILE* inputfp = fopen(argv[2], "r");
  FILE* outputfp = fopen(argv[3], "w");
  char* input = malloc(bufferSize);
  int inputLength = fread(input, 1, bufferSize, inputfp);
  char* output = NULL;
  int outputLength;
  printf("Before %d\n", inputLength);

  if (strcmp(argv[1], "-c") == 0) {
    output = malloc(bufferSize);
    outputLength = compress(input, inputLength, output, bufferSize);
  } else {
    outputLength = decompress(input, inputLength, &output);
  }

  fwrite(output, 1, outputLength, outputfp);
  printf("After %d\n", outputLength);

  fclose(inputfp);
  fclose(outputfp);
  free(input);
  free(output);
  return 0;
}

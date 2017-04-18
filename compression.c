#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int windowSize = 4*1024;
static const int minMatchLength = 4;
static const int maxMatchLength = 0x3f;
static const int maxLiteralChainLength = 0x7f;

/* Look for the longest match from input in the window, storing the pointer in matchPos and
 * returning its length
 */
static int match(char* windowStart, char* input, char* inputEnd, char** matchPos) {
  int bestLength = 0;

  while(windowStart < input) {
    int currentMatch = 0; 
    while(input+currentMatch < inputEnd &&
      windowStart[currentMatch] == input[currentMatch] && currentMatch < maxMatchLength) {
      currentMatch++;
    }
    // Prefer a later match of the same length - smaller offset
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
  char* activeLiteral = NULL;
  // Store the uncompressed length at the start
  *(int*)output = inputLength;
  output += sizeof(int);
  
  while(input < inputEnd) {
    char* windowStart = input-windowSize < inputStart ? inputStart : input-windowSize;
    char* matchPos = NULL;
    int matchLength = match(windowStart, input, inputEnd, &matchPos);
    
    if (matchLength >= minMatchLength) {
      // We found a good match. Reset the literal header pointer and emit the match data
      activeLiteral = NULL;
      unsigned short offset = (unsigned short)(input-matchPos);
      if (offset <= 0xff) {
        *output++ = matchLength | 0x80;
        *output++ = (unsigned char)offset;
      } else {
        *output++ = matchLength | 0xc0;
        *(unsigned short*)output = offset;
        output += sizeof(unsigned short);
      }
      input += matchLength;
    } else {
      // No match. If we are already in a literal chain just update the header's count,
      // otherwise emit a new header. Make sure not to increment a header past its max value.
      if (activeLiteral == NULL || *activeLiteral == maxLiteralChainLength) {
        activeLiteral = output++;
        *activeLiteral = 0;
      }
      (*activeLiteral)++;
      *output++ = *input++;
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
      // Highest bit is 0 - write out the literal chain
      unsigned char length = *input++;
      while (length > 0) {
        *output++ = *input++;
        length--;
      }
    } else {
      // Highest bit is 1 - write out the contents of the match by looking back into output
      unsigned char firstByte = *input++;
      unsigned char length = firstByte & 0x3f;
      unsigned short offset;
      if ((firstByte & 0x40) == 0) {
        offset = *(unsigned char*)input++;
      } else {
        offset = *(unsigned short*)input;
        input += sizeof(unsigned short);
      }
      char* start = output - offset;

      while(length > 0) {
        *output++ = *start++;
        length--;
      }
    }
  }
  // In any valid situation this should be the same as uncompressedLength, but for debugging
  // returning the generated length is helpful. 
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

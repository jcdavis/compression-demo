#include <inttypes.h>
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
static int match(uint8_t* windowStart, uint8_t* input, uint8_t* inputEnd, uint8_t** matchPos) {
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

int compress(uint8_t* inputStart, int inputLength, uint8_t* outputStart, int outputLength) {
  uint8_t* input = inputStart;
  uint8_t* inputEnd = input + inputLength;
  uint8_t* output = outputStart;
  uint8_t* activeLiteral = NULL;
  // Store the uncompressed length at the start
  *(uint32_t*)output = inputLength;
  output += sizeof(uint32_t);
  
  while(input < inputEnd) {
    uint8_t* windowStart = input-windowSize < inputStart ? inputStart : input-windowSize;
    uint8_t* matchPos = NULL;
    int matchLength = match(windowStart, input, inputEnd, &matchPos);
    
    if (matchLength >= minMatchLength) {
      // We found a good match. Reset the literal header pointer and emit the match data
      activeLiteral = NULL;
      uint16_t offset = (uint16_t)(input-matchPos);
      if (offset <= 0xff) {
        *output++ = 0x80 | matchLength;
        *output++ = (uint8_t)offset;
      } else {
        *output++ = 0x80 | 0x40 | matchLength;
        *(uint16_t*)output = offset;
        output += sizeof(uint16_t);
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

int decompress(uint8_t* input, int inputLength, uint8_t** outputRef) {
  uint8_t* inputEnd = input + inputLength;
  int uncompressedLength = *(uint32_t*)input;
  input += sizeof(uint32_t);
  *outputRef = malloc(uncompressedLength);
  uint8_t* output = *outputRef;

  while(input < inputEnd) {
    if ((*input & 0x80) == 0) {
      // Highest bit is 0 - write out the literal chain
      uint8_t length = *input++;
      while (length > 0) {
        *output++ = *input++;
        length--;
      }
    } else {
      // Highest bit is 1 - write out the contents of the match by looking back into output
      uint8_t firstByte = *input++;
      uint8_t length = firstByte & 0x3f;
      uint16_t offset;
      if ((firstByte & 0x40) == 0) {
        offset = *input++;
      } else {
        offset = *(uint16_t*)input;
        input += sizeof(uint16_t);
      }
      uint8_t* start = output - offset;

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

int main(int argc, char** argv) {
  if (argc != 4 || (strcmp(argv[1],"-c") != 0 && strcmp(argv[1], "-d") != 0)) {
    printf("Unexpected arguments. Format is:\ncompression-demo -c/-d inputfile outputfile\n");
    return 1;
  }
  int bufferSize = 640000;
  FILE* inputfp = fopen(argv[2], "r");
  FILE* outputfp = fopen(argv[3], "w");
  uint8_t* input = malloc(bufferSize);
  int inputLength = fread(input, 1, bufferSize, inputfp);
  uint8_t* output = NULL;
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

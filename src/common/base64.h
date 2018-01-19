/**
 * base64.h
 */
#ifndef ___BASE64_H___
#define ___BASE64_H___

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BASE64_DEF_LINE_SIZE
#  define BASE64_DEF_LINE_SIZE   72
#endif

#ifndef BASE64_MIN_LINE_SIZE
#  define BASE64_MIN_LINE_SIZE    4
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

void base64_encode_file( FILE *infile, FILE *outfile, int linesize );
void base64_decode_file( FILE *infile, FILE *outfile );
int  base64_encode_string( const unsigned char *instr, int inlen, unsigned char* outstr, int linesize );
int  base64_decode_string( const unsigned char *instr, int inlen, unsigned char *outstr );
int  base64_decode_wstring( const wchar_t *instr, int inlen, unsigned char *outstr );

#if defined(__cplusplus)
}
#endif

#endif

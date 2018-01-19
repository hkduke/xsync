/**
 * base64.c
 */
#include <assert.h>
#include "base64.h"		/* first  include */
#include "b64.h"		  /* second include */

void base64_encode_file( FILE *infile, FILE *outfile, int linesize )
{
	b64_encode_stream( infile, outfile, linesize );
}

void base64_decode_file( FILE *infile, FILE *outfile )
{
	b64_decode_stream( infile, outfile );
}

int  base64_encode_string( const unsigned char *instr, int inlen, unsigned char* outstr, int linesize )
{
	return b64_encode_string( instr, inlen, outstr, linesize );
}

int  base64_decode_string( const unsigned char *instr, int inlen, unsigned char *outstr )
{
	return b64_decode_string( instr, inlen, outstr );
}

int  base64_decode_wstring( const wchar_t *instr, int inlen, unsigned char *outstr )
{
	return b64_decode_wstring( instr, inlen, outstr );
}
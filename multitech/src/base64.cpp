#include "base64.h"

Base64::Base64() {
}

Base64::~Base64() {
}

size_t Base64::encoded_size( size_t inlen ) {
	size_t ret;

	ret = inlen;
	if ( inlen % 3 != 0 ) {
		ret += 3 - ( inlen % 3 );
	}
	ret /= 3;
	ret *= 4;

	return ret;
}

bool Base64::encode( unsigned char* in, char* out, size_t len ) {

	size_t  elen;
	size_t  i;
	size_t  j;
	size_t  v;

	if ( in == NULL || len == 0 ) {
		return false;
	}

	elen = encoded_size( len );
	out[elen] = '\0';

	for ( i=0, j=0; i<len; i+=3, j+=4 ) {
		v = in[i];
		v = i+1 < len ? v << 8 | in[i+1] : v << 8;
		v = i+2 < len ? v << 8 | in[i+2] : v << 8;

		out[j]   = b64chars[( v >> 18 ) & 0x3F];
		out[j+1] = b64chars[( v >> 12 ) & 0x3F];
		if ( i+1 < len ) {
			out[j+2] = b64chars[( v >> 6 ) & 0x3F];
		} else {
			out[j+2] = '=';
		}
		if ( i+2 < len ) {
			out[j+3] = b64chars[v & 0x3F];
		} else {
			out[j+3] = '=';
		}
	}

	return true;
}

size_t Base64::decoded_size( const char* in ) {
	size_t len;
	size_t ret;
	size_t i;

	if ( in == NULL ) {
		return 0;
	}

	len = strlen( in );
	ret = len / 4 * 3;

	for ( i=len; i-->0; ) {
		if ( in[i] == '=' ) {
			ret--;
		} else {
			break;
		}
	}

	return ret;
}

bool Base64::isvalidchar( char c ) {
	if ( c >= '0' && c <= '9' ) {
		return true;
	}
	if ( c >= 'A' && c <= 'Z' ) {
		return true;
	}
	if ( c >= 'a' && c <= 'z' ) {
		return true;
	}
	if ( c == '+' || c == '/' || c == '=' ) {
		return true;
	}
	return false;
}

bool Base64::decode( const char* in, unsigned char* out, size_t outlen ) {
	size_t len;
	size_t i;
	size_t j;
	int    v;

	if ( in == NULL || out == NULL ) {
		return false;
    }

	len = strlen( in );
	if ( outlen < decoded_size( in ) || len % 4 != 0 ) {
		return false;
	}

	for ( i=0; i<len; i++ ) {
		if ( !isvalidchar( in[i] ) ) {
			return false;
		}
	}

	for ( i=0, j=0; i<len; i+=4, j+=3 ) {
		v = b64invs[in[i]-43];
		v = ( v << 6 ) | b64invs[in[i+1]-43];
		v = in[i+2]=='=' ? v << 6 : ( v << 6 ) | b64invs[in[i+2]-43];
		v = in[i+3]=='=' ? v << 6 : ( v << 6 ) | b64invs[in[i+3]-43];

		out[j] = ( v >> 16 ) & 0xFF;
		if ( in[i+2] != '=' ) {
			out[j+1] = ( v >> 8 ) & 0xFF;
		}
		if ( in[i+3] != '=' ) {
			out[j+2] = v & 0xFF;
		}
	}

	return true;
}
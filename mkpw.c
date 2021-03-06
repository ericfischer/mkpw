#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE_CC__
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#define HAS_PBCOPY
#else
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

void out(unsigned char *md_value, int md_len, int alpha, int maxlen, int verbose) {
	/* like base64, but heavier on punctuation */
	char *alphabet =
		"AaBbCcDd"
		"EeFfGgHh"
		"IiJjKkLL"
		"MmNnOoPp"
		"QqRrSs<>"
		"01234567"
		"`~!@#$%^"
		"&*()-_=+"
		"[]{};:,.";

	if (alpha) {
		alphabet =
		"AaBbCcDd"
		"EeFfGgHh"
		"IiJjKkLL"
		"MmNnOoPp"
		"QqRrSsTt"
		"UuVvWwXx"
		"YyZz0123"
		"45678901"
		"23456789";
	}

#ifdef HAS_PBCOPY
	int pbcopy = 1;
#else
	int pbcopy = 0;
#endif

	if (verbose) {
		pbcopy = 0;
	}

	FILE *out;

	if (pbcopy) {
		out = popen("pbcopy", "w");
		if (out == NULL) {
			perror("pbcopy");
			exit(EXIT_FAILURE);
		}
	} else {
		out = stdout;
	}

	int i;
	int len = 0;
	for (i = 0; i / 8 < md_len; i += 6) {
		int v = md_value[i / 8];
		if (i / 8 + 1 < md_len) {
			v |= md_value[i / 8 + 1] << 8;
		}

		v >>= i % 8;

		if (len++ < maxlen) {
			putc(alphabet[v % 64], out);
		} else {
			break;
		}
	}

	if (pbcopy) {
		pclose(out);
	} else {
		printf("\n");
	}
}

void usage(char **argv) {
	fprintf(stderr, "Usage: %s [-a] [-l maxlen] service\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int i;
	int alpha = 0;
	int maxlen = INT_MAX;
	int verbose = 0;

	while ((i = getopt(argc, argv, "al:v")) != -1) {
		switch (i) {
		case 'a':
			alpha = 1;
			break;

		case 'l':
			maxlen = atoi(optarg);
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			usage(argv);
		}
	}

	if (argc - optind != 1) {
		usage(argv);
	}

	char *service = argv[optind];
	char *p = "Master password for ";
	char prompt[strlen(service) + strlen(p) + 3];
	sprintf(prompt, "%s%s: ", p, service);

	char *pw = getpass(prompt);

#ifdef __APPLE_CC__
	unsigned char md_value[CC_SHA1_DIGEST_LENGTH];
	int md_len = CC_SHA1_DIGEST_LENGTH;

	CCHmac(kCCHmacAlgSHA1, pw, strlen(pw), service, strlen(service), md_value);
#else
	unsigned char md_value[SHA_DIGEST_LENGTH];
	int md_len = SHA_DIGEST_LENGTH;

	HMAC(EVP_sha1(), pw, strlen(pw), service, strlen(service), md_value, &md_len);
#endif

	out(md_value, md_len, alpha, maxlen, verbose);
}

#include <cstdlib>
#include <cstdio>
#include <getopt.h>

#include <tuple>

#include "transcode.h"


static const struct option long_options[] = {
	{"input",		required_argument, NULL, 'i'},
	{"output",		required_argument, NULL, 'o'},
	{"seek",		required_argument, NULL, 's'},
	{"duration",	required_argument, NULL, 'd'},
	{"crop",		required_argument, NULL, 'c'},
	{"rescale",		required_argument, NULL, 'r'},
	
	//TODO: no-audio
	{0,				0,                 NULL, 0}
};
static const char *help =
"usage:\n"
"  -i, --input filename                      \n"
"  -o, --output filename                     \n"
"  -s, --seek min:sec                        \n"
"  -d, --duration sec                        \n"
"  -c, --crop widthxheight+offsetx,offsety   \n"
"  -r, --rescale widthxheight                \n"
;

void usage(){
	printf("%s", help);
	exit(0);
}

int main(int argc, char *argv[]){
	char *fname_input=NULL, *fname_output=NULL;
	int crop_w=0, crop_h=0, crop_x=0, crop_y=0;
	int rescale_w=0, rescale_h=0;
	if(argc < 2){
		usage();
	}
	int config, option_idx;
	char *ptr;
	for(;;){
		config = getopt_long(argc, argv, "i:o:s:d:c:r:", long_options, &option_idx);
		if(config == -1) break;
		switch(config){
			case 'i':
				fname_input = optarg;
				break;
			case 'o':
				fname_output = optarg;
				break;
			case 's':
				//TODO
				break;
			case 'd':
				//TODO
				break;
			case 'c':
				crop_w = atoi(optarg);
				ptr = optarg;
				while(*ptr && *ptr!='x') ++ptr;
				if(*ptr){
					crop_h = atoi(++ptr);
					while(*ptr && *ptr!='+') ++ptr;
					if(*ptr){
						crop_x = atoi(++ptr);
						while(*ptr && *ptr!=',') ++ptr;
						if(*ptr){
							crop_y = atoi(++ptr);
							break;
						}
					}
				}
				usage(); //no return
			case 'r':
				rescale_w = atoi(optarg);
				ptr = optarg;
				while(*ptr && *ptr!='x') ++ptr;
				if(*ptr){
					rescale_h = atoi(++ptr);
				}else{
					usage();
				}
				break;

			default:
				usage();
		}
	}
	
	printf("filter: crop=%i:%i:%i:%i,scale=%i:%i\n", crop_w, crop_h, crop_x, crop_y, rescale_w, rescale_h);
	
	FILE *f = fopen(fname_input, "rb");
	fseek(f, 0L, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);
	uint8_t *buf = new uint8_t[fsize];
	fread(buf, 1, fsize, f);
	fclose(f);
	uint8_t *outbuf;
	size_t outsize;
	std::tie(outbuf,outsize) = transcode(buf, fsize);
	
	f = fopen(fname_output, "wb");
	fwrite(outbuf, 1, outsize, f);
	fclose(f);
	
	delete[] buf;
	free_buffer(outbuf);
	
	return 0;
}

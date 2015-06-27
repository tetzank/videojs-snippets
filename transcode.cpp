#include "transcode.h"

#include <cstdlib>
#include <cstdio>

extern "C"{
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}


struct buffer_data{
	uint8_t *start;
	size_t size;
	uint8_t *pos;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size){
#ifndef NDEBUG
	printf("read: size %i\n", buf_size);
#endif
	struct buffer_data *bd = (struct buffer_data*)opaque;
	buf_size = FFMIN(buf_size, bd->size - (bd->pos - bd->start));
	
	memcpy(buf, bd->pos, buf_size);
	bd->pos  += buf_size;

	return buf_size;
}

#if 0
static int write_packet(void *opaque, uint8_t *buf, int buf_size){
#ifndef NDEBUG
	printf("write: size %i\n", buf_size);
#endif
	struct buffer_data *bd = (struct buffer_data*)opaque;
	if(bd->start){
		// check if it fits in
		int free = bd->size - (bd->pos - bd->start);
		if(free < buf_size){
			//FIXME: this reallocs many times if small packets get written
			int pos = bd->pos - bd->start;
			bd->size = buf_size - free;
			bd->start = (uint8_t*)realloc(bd->start, bd->size);
			bd->pos = bd->start + pos;
			memcpy(bd->pos, buf, buf_size);
			bd->pos += buf_size;
			
			//FIXME: maybe better using vector, but how to get raw data out of it?
		}
	}else{
		// first time -> alloc and copy everything
		bd->start = (uint8_t*)malloc(buf_size);
		memcpy(bd->start, buf, buf_size);
		bd->size = buf_size;
		bd->pos = bd->start + buf_size;
	}
	return buf_size;
}
#endif

static int64_t seek(void *opaque, int64_t offset, int whence){
#ifndef NDEBUG
	printf("seek: offset %li, whence %i\n", offset, whence);
#endif
	struct buffer_data *bd = (struct buffer_data*)opaque;
	switch(whence){
		case SEEK_SET:
			bd->pos = bd->start + offset;
			break;
		case SEEK_CUR:
			bd->pos += offset;
			break;
		case SEEK_END:
			bd->pos = bd->start + bd->size + offset;
			break;

		case AVSEEK_SIZE:
			return bd->size;

		default:
			return -1;
	}
	return 0;
}

#define ERROR(string) error(string, __FILE__, __LINE__)
static void error(const char *message, const char *file, int line){
	fprintf(stderr, message);
	fprintf(stderr, ": %s:%i\n", file, line);
	exit(-1);
}

std::pair<uint8_t*,size_t> transcode(uint8_t *buf, size_t buf_size){
	constexpr int avio_ctx_buffer_size = 4 * (1<<20);
	uint8_t *avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
// 	uint8_t *avio_ctx_buffer_out = (uint8_t*)av_malloc(avio_ctx_buffer_size);
	
	struct buffer_data bdin = {buf, buf_size, buf};
// 	struct buffer_data bdout = {nullptr, 0, nullptr};
	
	av_register_all();
	
	AVIOContext *avio_ctx = avio_alloc_context(
		avio_ctx_buffer, avio_ctx_buffer_size, 0,
		&bdin, &read_packet, NULL, &seek
	);
	if(!avio_ctx) ERROR("can't allocate avio context");
// 	AVIOContext *avio_ctx_out = avio_alloc_context(
// 		avio_ctx_buffer_out, avio_ctx_buffer_size, 0,
// 		&bdout, &read_packet, &write_packet, &seek
// 	);
	
	AVFormatContext *fmt_ctx = avformat_alloc_context();
	if(!fmt_ctx) ERROR("can't allocate format context");
	fmt_ctx->pb = avio_ctx;
	int ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	if(ret < 0) ERROR("can't open format input");
	
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if(ret < 0) ERROR("couldn't find stream info");
	av_dump_format(fmt_ctx, 0, "bla", 0);
	
	AVCodecContext *codec_ctx=NULL, *codec_ctx_orig=NULL;
	int video_idx;
	for(unsigned int i=0; i<fmt_ctx->nb_streams; ++i){
		if(fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			codec_ctx_orig = fmt_ctx->streams[i]->codec;
			video_idx = i;
			break;
		}
	}
	if(codec_ctx_orig == NULL) ERROR("no video stream found");

	AVCodec *codec = NULL;
	codec = avcodec_find_decoder(codec_ctx_orig->codec_id);
	if(codec==NULL) ERROR("unsupported codec");

	codec_ctx = avcodec_alloc_context3(codec);
	if(avcodec_copy_context(codec_ctx, codec_ctx_orig) != 0) ERROR("couldn't copy codec context");

	if(avcodec_open2(codec_ctx, codec, NULL) < 0) ERROR("couldn't open codec");
	
	//output
	AVFormatContext *fmt_outctx = NULL;
	avformat_alloc_output_context2(&fmt_outctx, NULL, "webm", NULL);
	if(!fmt_outctx) ERROR("couldn't create output format context");
// 	fmt_outctx->pb = avio_ctx_out;
	AVCodec *codec_out = avcodec_find_encoder(AV_CODEC_ID_VP9);
	if(!codec_out) ERROR("couldn't find vp9 encoder");
	AVStream *out_stream = avformat_new_stream(fmt_outctx, codec_out);
	if(!out_stream) ERROR("can't create new format stream");
	AVCodecContext *codec_outctx = out_stream->codec;
	codec_outctx->codec_id = AV_CODEC_ID_VP9;
	codec_outctx->bit_rate = codec_ctx->bit_rate;//200000;
// 	codec_outctx->global_quality = 1;
	codec_outctx->width = codec_ctx->width;//960;
	codec_outctx->height = codec_ctx->height;//540;
	codec_outctx->sample_aspect_ratio = codec_ctx->sample_aspect_ratio;
	out_stream->time_base = codec_ctx->time_base;//(AVRational){ 1, 25/*30*/ };
	codec_outctx->time_base = codec_ctx->time_base;//out_stream->time_base;
	codec_outctx->pix_fmt = AV_PIX_FMT_YUV420P;
	if(fmt_outctx->oformat->flags & AVFMT_GLOBALHEADER){
		codec_outctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

// 	av_opt_set(codec_outctx->priv_data, "crf", "33", 0);
// 	av_opt_set(codec_outctx->priv_data, "speed", "6", 0);

	if(avcodec_open2(codec_outctx, codec_out, NULL) < 0) ERROR("couldn't open output codec");

// 	avio_open(&format_outctx->pb, argv[2], AVIO_FLAG_WRITE);
	if(avio_open_dyn_buf(&fmt_outctx->pb) != 0) ERROR("can't open dynamic buffer");
// 	av_dump_format(fmt_outctx, 0, "bla", 0);
	if(avformat_write_header(fmt_outctx, NULL) < 0) ERROR("writing format header failed");

	AVFrame *frame=NULL;
	frame = av_frame_alloc();
	if(frame==NULL){
		exit(-1);
	}

	int /*count=0, */frameFinished=0;
	AVPacket packet;
// 	int done=0;
// 	av_seek_frame(fmt_ctx, video_idx, 25, AVSEEK_FLAG_ANY);
	while(av_read_frame(fmt_ctx, &packet) >= 0 /*&& !done*/){
		// is this packet from a video stream
		if(packet.stream_index == video_idx){
// 			frame = av_frame_alloc();
// 			if(!frame) ERROR("frame allocation failed");

			av_packet_rescale_ts(&packet,
				fmt_ctx->streams[video_idx]->time_base,
				fmt_ctx->streams[video_idx]->codec->time_base
			);
			// decode frame
			if(avcodec_decode_video2(codec_ctx, frame, &frameFinished, &packet) < 0) ERROR("decode failed");

			if(frameFinished){
				frame->pts = av_frame_get_best_effort_timestamp(frame);
				//TODO: filter
				int outFrameFinished=0;
				AVPacket out_packet;
				out_packet.data = NULL;
				out_packet.size = 0;
				av_init_packet(&out_packet);
				if(avcodec_encode_video2(codec_outctx, &out_packet, frame, &outFrameFinished) < 0) ERROR("ecnode failed");
				if(outFrameFinished){
					out_packet.stream_index = video_idx;
					av_packet_rescale_ts(&out_packet,
						fmt_outctx->streams[video_idx]->codec->time_base,
						fmt_outctx->streams[video_idx]->time_base
					);
					if(av_interleaved_write_frame(fmt_outctx, &out_packet) < 0) ERROR("writing frame failed");
				}
			}
// 			av_frame_free(&frame);
		}
		// av_read_frame allocated stuff -> free it
		av_free_packet(&packet);
	}
	av_write_trailer(fmt_outctx);
	
	av_free_packet(&packet);

	av_frame_free(&frame);

// 	avio_closep(&format_outctx->pb);
	uint8_t *outbuf;
	int outsize = avio_close_dyn_buf(fmt_outctx->pb, &outbuf);
	avcodec_close(codec_outctx);
// 	avcodec_free_context(&codec_outctx);
	avformat_free_context(fmt_outctx);
	
	avcodec_close(codec_ctx);
	avcodec_free_context(&codec_ctx);
	avcodec_close(codec_ctx_orig);
	
	avformat_close_input(&fmt_ctx);
	if (avio_ctx) {
		av_freep(&avio_ctx->buffer);
		av_freep(&avio_ctx);
	}
// 	if(avio_ctx_out){
// 		av_freep(&avio_ctx_out->buffer);
// 		av_freep(&avio_ctx_out);
// 	}

// 	return std::make_pair(bdout.start, bdout.size);
	return std::make_pair(outbuf, outsize);
}

void free_buffer(uint8_t *buf){
	av_free(buf);
}




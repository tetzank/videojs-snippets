#ifndef TRANSCODE_H_
#define TRANSCODE_H_

#include <utility>
#include <cstdlib>
#include <cstdint>


std::pair<uint8_t*,size_t> transcode(uint8_t *buf, size_t buf_size);

void free_buffer(uint8_t *buf);


#endif

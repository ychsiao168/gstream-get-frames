#ifndef __LIB_GETFRAME_H_INCLUDE__
#define __LIB_GETFRAME_H_INCLUDE__

typedef void (*fp_frame_cb)(unsigned char *frame, unsigned int frame_size);

int fourd_init_frame(const char* url, fp_frame_cb cb);
void fourd_start_frame_loop(void);
void fourd_deinit_frame(void);

#endif
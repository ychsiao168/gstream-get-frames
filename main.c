#include <stdio.h>
#include "libgetframe.h"

void get_frame_cb(unsigned char *frame, unsigned int frame_size)
{
    static unsigned int num_samples = 0;

    num_samples++;
    printf("%8u | frame_size=%6u, data=0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X\n",
            num_samples,
            frame_size,
            *(unsigned short *)&frame[0], *(unsigned short *)&frame[1], *(unsigned short *)&frame[2], *(unsigned short *)&frame[3],
            *(unsigned short *)&frame[4], *(unsigned short *)&frame[5], *(unsigned short *)&frame[6], *(unsigned short *)&frame[7]
    );
}

int main(int argc, char* argv[])
{
    // url examples:
    // file:///path/to/test.mkv
    // https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4
    // https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
    // rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mp4
    // rtsp://176.122.164.163:8554/jellyfish-5-mbps-hd-h264.mkv
    // rtsp://username:password@192.168.1.145/stream-2.sdp

    fourd_init_frame("https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4", get_frame_cb);
    fourd_start_frame_loop();   // block until end of stream.
    fourd_deinit_frame();
    return 0;
}

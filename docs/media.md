### KVS SDK handling of the media

Kinesis Video Streams end-to-end streaming is mostly content type agnostic. It handles any time encoded series, similar to audio and video. Some applications use GPS coordinates, others use Lidar or Radar streams. These applications integrate with the SDK via Producer interface which accepts a Frame structure that can represent any time-encoded datum. The SDK as well as the KVS service overall have a few exceptions where the content type matters.

* H264/H265 video frames can be adapted from Annex-B format to AvCC and vice-versa.
* KVS SDK can auto-extract H264/H265 CPD (Codec Private Data) from an Annex-B Idr frames if the CPD has not been already specified by the application and NAL_ADAPTATION_ANNEXB_CPD_NALS flags have been specified in https://github.com/awslabs/amazon-kinesis-video-streams-pic/blob/master/src/client/include/com/amazonaws/kinesis/video/client/Include.h#L916. Many encoders that produce Annex-B format elementary streams include SPS/PPS (and in case of H265 VPS) NALs prepended to the Idr frames.
* KVS SDK can adapt CPD from Annex-B format to AvCC format for H264 and H265
* KVS SDK will automatically attempt to extract video pixel width and height (for a number of formats) and include PixelWidth and PixelHeight elements in the generated MKV (https://www.matroska.org/technical/elements.html) as some downstream consumers require it for video.
* KVS SDK will automatically attempt to extract and set Audio specific elements in the MKV for audio streams as those can be required by the downstream consumers.
* KVS SDK has a set of APIs to generate audio specific CPD.
* KVS HLS/MPEG-DASH/Console playback and clip generation require specific elementary stream and packaging formats - for example, the console playback requires H264/H265 elementary stream with CPD in AvCC format. More information on the supported formats and limitations can be found in AWS documentation.




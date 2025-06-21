---
name: HLS/DASH/Console Playback Failure
about: Are you experiencing a failure when playing your streamed media?
title: "[Playback Failure]"
labels: Playback
assignees: ''

---

**NOTE:** This template is for failure to playback issues of HLS, DASH, and AWS console players. If the playback issue is related to GetMedia playback, please submit an issue on the [KVS Java Parser Library](https://github.com/aws/amazon-kinesis-video-streams-parser-library) repository. If the issue is related to HLS/DASH/Console, but the issue is not regarding a failure to playback (such as a latency issue or general question), please submit using the general question template.

### Brief one-liner description of the issue:
...

### Please include the following details:
- Problematic playback methods (HLS, DASH, and/or AWS Console), operating systems, browsers:
  - ...
- Working playback methods (were you able to playback the stream using using any methods?), operating systems, browsers:
  - ...
- Is the stream's data retention setting greater than 0?
- Does your stream meet the [KVS video playback track requirements](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/video-playback-requirements.html)?
- If there are fragment decoding errors:
  - Are fragment timestamps accurate, in the correct order, and have no duplicates? ([ListFragments](https://docs.aws.amazon.com/kinesisvideostreams/latest/dg/API_reader_ListFragments.html) can be used to retrieve fragment information for a stream)
  - Is your application encoding the frame data using the H.264 format?
  - Does the resolution of the frames match the resolution specified in the Codec Private Data?
  - Does the H.264 profile and level of the encoded frames match the profile and level specified in the Codec Private Data?
  - Does the browser/OS support the profile/level combination?
- If there are HLS playback failures:
  - Is the fragment duration less than 1 second? If yes, does issue persist with fragments longer than 1 second?
  - Is each HLS streaming session URL being used by only one player at a time?
  - Does each fragment have a consistent number of tracks, is not changing between having both an audio and video track and only a video track, and has consistent encoder settings (resolution and frame rate not changing between fragments in each track)?
  - Does issue persist after fetching using GetHLSStreamingSessionURL with the ContainerFormat and DiscontinuityMode parameters set to different values?


### Logging
Add any relevant SDK and player logs.  IMPORTANT NOTE:  Please make sure to NOT share AWS access credentials under any circumstance!  Please make sure they are not in the logs.

** If you would not like to open an issue to discuss your solution in open-platform, please email your question to kinesis-video-support@amazon.com **

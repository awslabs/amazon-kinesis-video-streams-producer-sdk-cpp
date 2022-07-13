package com.amazon.kinesis.video.canary.consumer;

import com.amazonaws.kinesisvideo.parser.mkv.Frame;
import com.amazonaws.kinesisvideo.parser.mkv.FrameProcessException;
import com.amazonaws.kinesisvideo.parser.utilities.FragmentMetadata;
import com.amazonaws.kinesisvideo.parser.utilities.FragmentMetadataVisitor;
import com.amazonaws.kinesisvideo.parser.utilities.FrameVisitor;
import com.amazonaws.kinesisvideo.parser.utilities.MkvTrackMetadata;
import com.amazonaws.services.cloudwatch.AmazonCloudWatchAsync;
import com.amazonaws.services.cloudwatch.model.Dimension;
import com.amazonaws.services.cloudwatch.model.MetricDatum;
import com.amazonaws.services.cloudwatch.model.PutMetricDataRequest;
import com.amazonaws.services.cloudwatch.model.StandardUnit;
import com.google.common.primitives.Ints;
import com.google.common.primitives.Longs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.zip.CRC32;

public class CanaryFrameProcessor implements FrameVisitor.FrameProcessor {
    int lastFrameIndex = -1;
    final AmazonCloudWatchAsync cwClient;
    final Dimension dimensionPerStream;
    final Dimension aggregatedDimension;

    public CanaryFrameProcessor(AmazonCloudWatchAsync cwClient, String streamName, String canaryLabel) {
        this.cwClient = cwClient;
        dimensionPerStream = new Dimension()
                .withName("ProducerSDKCanaryStreamName")
                .withValue(streamName);
        aggregatedDimension = new Dimension()
                .withName("ProducerSDKCanaryType")
                .withValue(canaryLabel);
    }

    @Override
    public void process(Frame frame, MkvTrackMetadata trackMetadata, Optional<FragmentMetadata> fragmentMetadata, Optional<FragmentMetadataVisitor.MkvTagProcessor> tagProcessor) throws FrameProcessException {
        int frameTimeDelta = frame.getTimeCode();
        long fragmentStartTime = fragmentMetadata.get().getProducerSideTimestampMillis();
        byte[] data = new byte[frame.getFrameData().remaining()];
        int offset = 0;
        frame.getFrameData().get(data);
        byte[] timeData = new byte[Long.BYTES];

        System.arraycopy(data, offset, timeData, 0, timeData.length);
        offset += timeData.length;
        long frameTimeInsideData = Longs.fromByteArray(timeData);

        byte[] indexData = new byte[Integer.BYTES];
        System.arraycopy(data, offset, indexData, 0, indexData.length);
        offset += indexData.length;
        int frameIndex = Ints.fromByteArray(indexData);

        byte[] sizeData = new byte[Integer.BYTES];
        System.arraycopy(data, offset, sizeData, 0, sizeData.length);
        offset += sizeData.length;
        int frameSize = Ints.fromByteArray(sizeData);

        List<MetricDatum> datumList = new ArrayList<>();
        // frameSize == buffer size - extra canary metadata size
        MetricDatum datum = new MetricDatum()
                .withMetricName("FrameSizeMatch")
                .withUnit(StandardUnit.None)
                .withValue(frameSize == data.length ? 1.0 : 0)
                .withDimensions(dimensionPerStream);
        datumList.add(datum);
        MetricDatum aggDatum = new MetricDatum()
                .withMetricName("FrameSizeMatch")
                .withUnit(StandardUnit.None)
                .withValue(frameSize == data.length ? 1.0 : 0)
                .withDimensions(aggregatedDimension);
        datumList.add(aggDatum);

        byte[] crcData = new byte[Long.BYTES];
        System.arraycopy(data, offset, crcData, 0, crcData.length);
        Arrays.fill(data, offset, offset + crcData.length, (byte) 0);
        offset += crcData.length;
        long crcValue = Longs.fromByteArray(crcData);
        CRC32 crc32 = new CRC32();
        crc32.update(data);

        datum = new MetricDatum()
                .withMetricName("FrameDataMatches")
                .withUnit(StandardUnit.None)
                .withValue(crc32.getValue() == crcValue ? 1.0 : 0)
                .withDimensions(dimensionPerStream);
        datumList.add(datum);
        aggDatum = new MetricDatum()
                .withMetricName("FrameDataMatches")
                .withUnit(StandardUnit.None)
                .withValue(crc32.getValue() == crcValue ? 1.0 : 0)
                .withDimensions(aggregatedDimension);
        datumList.add(aggDatum);


        // frameTimestampInsideData == producerTimestamp + frame timecode
        datum = new MetricDatum()
                .withMetricName("FrameTimeMatchesProducerTimestamp")
                .withUnit(StandardUnit.None)
                .withValue(frameTimeInsideData == fragmentStartTime + frameTimeDelta ? 1.0 : 0)
                .withDimensions(dimensionPerStream);
        datumList.add(datum);
        aggDatum = new MetricDatum()
                .withMetricName("FrameTimeMatchesProducerTimestamp")
                .withUnit(StandardUnit.None)
                .withValue(frameTimeInsideData == fragmentStartTime + frameTimeDelta ? 1.0 : 0)
                .withDimensions(aggregatedDimension);
        datumList.add(aggDatum);

        // frameIndex == lastFrameIndex + 1 except lastFrameIndex is not initialized
        if (lastFrameIndex >= 0) {
            datum = new MetricDatum()
                    .withMetricName("FrameDropped")
                    .withUnit(StandardUnit.None)
                    .withValue(frameIndex != lastFrameIndex + 1 ? 1.0 : 0)
                    .withDimensions(dimensionPerStream);
            datumList.add(datum);
            aggDatum = new MetricDatum()
                    .withMetricName("FrameDropped")
                    .withUnit(StandardUnit.None)
                    .withValue(frameIndex != lastFrameIndex + 1 ? 1.0 : 0)
                    .withDimensions(aggregatedDimension);
            datumList.add(aggDatum);
        }
        lastFrameIndex = frameIndex;

        // E2E frame latency = currentTime - frameTimeInsideData
        datum = new MetricDatum()
                .withMetricName("EndToEndFrameLatency")
                .withUnit(StandardUnit.Milliseconds)
                .withValue((double) System.currentTimeMillis() - frameTimeInsideData)
                .withDimensions(dimensionPerStream);
        datumList.add(datum);
        aggDatum = new MetricDatum()
                .withMetricName("EndToEndFrameLatency")
                .withUnit(StandardUnit.Milliseconds)
                .withValue((double) System.currentTimeMillis() - frameTimeInsideData)
                .withDimensions(aggregatedDimension);
        datumList.add(aggDatum);

        sendMetrics(datumList);
    }

    @Override
    public void close() {

    }

    private void sendMetrics(List<MetricDatum> datumList) {
        PutMetricDataRequest request = new PutMetricDataRequest()
                .withNamespace("KinesisVideoSDKCanary")
                .withMetricData(datumList);
        cwClient.putMetricDataAsync(request);
        //FIXME verify result of putting to cloudwatch
    }
}

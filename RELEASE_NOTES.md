Release 3.1.1 of the Amazon Kinesis Video C++ Producer SDK

Platforms tested on

Linux
Windows
MacOS

Release tagged at: 2d653a29451bce60741a4aeebb5825518345d25f

Whats new:

- Introduced a new kvssink property `disable-buffer-clipping` to cater to  some src/mux elements that produce non-standard segment start times
- Added unit test for resetting and re-creating the stream
- Added overloaded constructors for supplying the caching mode

Bug fixes:

- Appropriately set frame order mode for intermittent producer scenario in multi track case

Changes in Producer C: f2a97fe6eaf78cbffd46ccfa5994bee2bebf99bf

- Resetting stream will not purge the API caching entries

PIC Commit: df42dddc1d421d1e6bc47d5ebf7cd085446cbb69
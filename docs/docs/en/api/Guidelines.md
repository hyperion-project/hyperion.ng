# Guidelines
Improve the experience with Hyperion by following a rule set

[[toc]]

## Priority Guidelines
The possibilities of priorities mixing and using is endless. But where are a lot of possibilities you require also some guidelines to reduce confusion on user and developer side and to get the best matching experience possible.

The user expects, that an Effect or Color should be higher in priority than capturing as you usually run them from remotes and not all time.
While we have different capture/input methods, we follow also a specific priority chain to make sure that it fit's the most use cases out of the box.

|          Type           | Priority/Range | Recommended |                  Comment                   |
| :---------------------: | :------------: | :---------: | :----------------------------------------: |
|    Boot Effect/Color    |       0        |      -      |                  Blocked                   |
|    Web Configuration    |       1        |      -      |                                            |
|   **Remote Control**    |    **2-99**    |   **50**    |       Set effect/color/single image        |
|   **Image Streaming**   |  **100-199**   |   **150**   | For image streams (Flatbuffer/Protobuffer) |
|        Boblight         |      201       |      -      |                                            |
|       USB Capture       |      240       |      -      |                                            |
|    Platform Capture     |      250       |      -      |                                            |
| Background Effect/Color |      254       |      -      |                                            |
|        Reserved         |      255       |      -      |                                            |
 

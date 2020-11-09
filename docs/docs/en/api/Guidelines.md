# Guidelines
Improve the user experience with Hyperion by following these guidelines.

[[toc]]

## Priority Guidelines
Please adhere to the following priority guidelines to avoid user confusion and ensure
the best user experience possible:

The user expects that an effect or color should be higher in priority (lower in value)
than capturing, as colors/effects are usually run intermittently.

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
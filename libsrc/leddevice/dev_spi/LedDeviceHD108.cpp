#include "LedDeviceHD108.h"

/**
 * @brief Constructor for the HD108 LED device.
 *
 * @param deviceConfig JSON configuration object for this device.
 */
LedDeviceHD108::LedDeviceHD108(const QJsonObject &deviceConfig)
    : ProviderSpi(deviceConfig)
{
    // By default, set the global brightness register to full (16-bit max)
    _global_brightness = 0xFFFF;
}

/**
 * @brief Factory method: creates an instance of LedDeviceHD108.
 *
 * @param deviceConfig The JSON configuration for the device.
 * @return A pointer to the newly constructed LedDeviceHD108 instance.
 */
LedDevice* LedDeviceHD108::construct(const QJsonObject &deviceConfig)
{
    return new LedDeviceHD108(deviceConfig);
}

/**
 * @brief Initializes the HD108 device using the given JSON configuration.
 *
 * This reads certain device-specific parameters, such as the maximum brightness
 * level, and configures the global brightness register accordingly.
 *
 * @param deviceConfig The JSON object containing device parameters.
 * @return True if initialization succeeded, false otherwise.
 */
bool LedDeviceHD108::init(const QJsonObject &deviceConfig)
{
    bool isInitOK = false;

    // First, let the base SPI provider perform its initialization
    if (ProviderSpi::init(deviceConfig))
    {
        // Read brightnessControlMaxLevel from the config, falling back to a default if absent
        _brightnessControlMaxLevel = deviceConfig["brightnessControlMaxLevel"].toInt(HD108_BRIGHTNESS_MAX_LEVEL);

        // Log the brightness info
        Info(_log,
             "[%s] Setting maximum brightness to [%d] = %d%%",
             QSTRING_CSTR(_activeDeviceType),
             _brightnessControlMaxLevel,
             _brightnessControlMaxLevel * 100 / HD108_BRIGHTNESS_MAX_LEVEL);

        // Combine the brightness levels into the HD108's 16-bit brightness field.
        // According to the HD108 spec, this is composed of a control bit plus
        // the brightness level split into three segments for R, G, B.
        _global_brightness = (1 << 15)
                           | (_brightnessControlMaxLevel << 10)
                           | (_brightnessControlMaxLevel << 5)
                           | _brightnessControlMaxLevel;

        isInitOK = true;
    }

    return isInitOK;
}

/**
 * @brief Writes a vector of RGB colors to the HD108 LEDs.
 *
 * The HD108 protocol requires:
 * - A start frame of 64 bits (8 bytes) all set to 0x00.
 * - For each LED, 64 bits:
 *   - 16 bits of global brightness
 *   - 16 bits for red
 *   - 16 bits for green
 *   - 16 bits for blue
 * - An end frame of at least (ledCount / 16 + 1) bytes of 0xFF.
 *
 * Each 8-bit color value is expanded to 16 bits by copying it into both the high
 * and low byte (e.g. 0x7F -> 0x7F7F). This ensures a correct mapping to the HD108's
 * internal 16-bit color resolution and allows for a true "off" state at 0x0000.
 *
 * @param ledValues A vector of ColorRgb (red, green, blue) structures.
 * @return The result of the SPI write operation (0 for success, or an error code).
 */
int LedDeviceHD108::write(const std::vector<ColorRgb> & ledValues)
{
    // Calculate how much space we need in total:
    //  - 8 bytes for the start frame
    //  - 8 bytes per LED (16 bits global brightness + 16 bits R + G + B)
    //  - end frame: ledCount / 16 + 1 bytes of 0xFF
    const size_t ledCount = ledValues.size();
    const size_t totalSize = 8                        // start frame
                          + (ledCount * 8)            // LED data (8 bytes each)
                          + (ledCount / 16 + 1);      // end frame bytes

    // Reserve enough space to avoid multiple allocations
    std::vector<uint8_t> hd108Data;
    hd108Data.reserve(totalSize);

    // 1) Start frame: 64 bits of 0x00
    hd108Data.insert(hd108Data.end(), 8, 0x00);

    // 2) For each LED, insert 8 bytes: 16 bits brightness, 16 bits R, 16 bits G, 16 bits B
    for (const ColorRgb &color : ledValues)
    {
        // Expand 8-bit color components to 16 bits each
        uint16_t red16   = (static_cast<uint16_t>(color.red)   << 8) | color.red;
        uint16_t green16 = (static_cast<uint16_t>(color.green) << 8) | color.green;
        uint16_t blue16  = (static_cast<uint16_t>(color.blue)  << 8) | color.blue;

        // Global brightness (16 bits)
        hd108Data.push_back(_global_brightness >> 8);
        hd108Data.push_back(_global_brightness & 0xFF);

        // Red (16 bits)
        hd108Data.push_back(red16 >> 8);
        hd108Data.push_back(red16 & 0xFF);

        // Green (16 bits)
        hd108Data.push_back(green16 >> 8);
        hd108Data.push_back(green16 & 0xFF);

        // Blue (16 bits)
        hd108Data.push_back(blue16 >> 8);
        hd108Data.push_back(blue16 & 0xFF);
    }

    // 3) End frame: at least (ledCount / 16 + 1) bytes of 0xFF
    hd108Data.insert(hd108Data.end(), (ledCount / 16) + 1, 0xFF);

    // Finally, transmit the assembled data via SPI
    return writeBytes(hd108Data.size(), hd108Data.data());
}

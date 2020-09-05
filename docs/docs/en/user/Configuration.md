# Configuration
Hyperion is fully configurable via web browser. The interface is fully responsive and created with touch devices in mind.

## Web Configuration
Open the web configuration by typing the IP address of your device and the port 8090 in your browser. The installation script will show you the address, if you don't know it. \
**Example:** `http://192.168.0.20:8090`

### Dashboard
<ImageWrap src="/images/en/user_config_dash.jpg" alt="Hyperion Web Configuration - Dashboard" />

#### Top right navbar
 * **Arrows** - Switch between different LED hardware instances (If multiple are available)
 * **TV** - Live led visualization
 * **Wand/magic stick** - Wizards that guide you through color calibration and more
 * **Wrench** - Settings for language selection, settings level, logout, ...
 
 **Left sidebar**
 * **Dashboard** - Yes, here we are.
 * **Configuration** - All available settings
 * **Remote Control** - Control Hyperion like any other Hyperion remote application
 * **Effects Configurator** - Create new effects based on effect templates
 * **Support** - Where you get support and how to support us (and why)
 * **System** - Inspect your log messages, upload a report for support, credits page, etc

#### Page
  * The **Information** panel shows some important/useful informations. With a small smart access area with important actions.
  * The **Component status** shows always the latest state (enabled/disabled) of the components

::: tip Hashtag navigation
The web configuration supports hashtags for sitenames, so you could directly open a specific page by calling the hashtag. **Example:**`http://192.168.0.20:8090/#remote` - will open the remote control page.
:::

### Configuration
We added additional information(s) to each option. Some topics require additional attention which are covered here. If you need more help or something lacks of infos, just dive into our Forum.

#### Language
By default, the web configuration selects the language based on your browser locale or best matching next to. So no configuration is required. In case you want to start learning Hyperion in another language, select from the provided once. \
Want to contribute a new translation? It's easy! Checkout: [Contribute to Hyperion](https://github.com/hyperion-project/hyperion.ng#contributing).
<ImageWrap src="/images/en/user_config_lang.jpg" alt="Hyperion Web Configuration - Language" />

#### Settings level
Settings level prevents a option flooding for new users. While the **Default** level is for beginners and has the lowest amount of options the **Advanced** is for people that want to or need to dive a little deeper. **Expert** is for experts, you shouldn't need it that often.
<ImageWrap src="/images/en/user_config_access.jpg" alt="Hyperion Web Configuration - Settings level" />

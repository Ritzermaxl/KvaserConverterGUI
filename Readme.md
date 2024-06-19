# KvaserConverterGUI

project to make converting log files from the Kvaser Memorator easier and faster


Template repository: https://github.com/pthom/hello_imgui_template

## Explanations

### Get Hello ImGui with CMake

There are 3 ways to get HelloImGui with CMake. They are documented inside the CMakeLists.txt file.
By default, option 2 below is used: HelloImGui is downloaded and built automatically at configure time.


#### Option 2: automatic download
The [CMakeLists.txt](CMakeLists.txt) file will download and build hello_imgui at configure time, and make the "hello_imgui_add_app" cmake function available, if hello-imgui is not found;

By default, you do not need to add HelloImGui as a dependency to your project, it will be downloaded and built automatically during CMake configure time.
If you wish to use a local copy of HelloImGui, edit CMakeLists.txt and uncomment the `add_subdirectory` line.

*Note: `hello_imgui_add_app` will automatically link your app to hello_imgui, embed the assets folder (for desktop, mobile, and emscripten apps), and the application icon.*

### Assets folder structure

Anything in the assets/ folder located beside the app's CMakeLists will be bundled 
together with the app (for macOS, iOS, Android, Emscripten).
The files in assets/app_settings/ will be used to generate the app icon, 
and the app settings.

```
assets/
├── world.jpg                   # A custom asset. Any file or folder here will be deployed 
│                               # with the app.
├── fonts/
│    ├── DroidSans.ttf           # Default fonts used by HelloImGui
│    └── fontawesome-webfont.ttf # (if not found, the default ImGui font will be used)
│               
└── app_settings/               # Application settings
     └── icon.png               # This will be the app icon, it should be square
                                # and at least 256x256. It will  be converted
                                # to the right format, for each platform (except Android)
 


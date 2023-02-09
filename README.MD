# YARC - Yet Another Reflow Controller

### Setup Instructions

1. Install plugin platformio
2. Create project for your board
3. To match your wiring set the defines the corresponding value in _pindefines.h_
<<<<<<< webinterface
4. TFT_eSPI -> User_Setup.h
5. lvgl -> lv_conf.h (lv use canvas 1!)
6. lv_lib_qrcode -> #include "../lvgl/lvgl.h" (more todo in the .c file see todo.txt)
7. In platformIo Build the Filesystemimage (files to be uploaded to the filesystem are in the data-folder) and upload it to the esp32 if those files are missing, the mcu will crash. You can find this option unter PlatformIo -> Project Tasts -> Build Filesystem Image / Upload Filesystem Image
9. read todo.txt
10. profit???
=======
5. TFT_eSPI -> User_Setup.h
6. lvgl -> lv_conf.h setup from template
7. Change the include path in lv_liv_qrcode from "lvgl/lvgl.h" to "lvgl.h"
>>>>>>> master


## WEBINTERFACE

At this time there are two functions implemented in the webinterface of the controller

1. live monitoring and downloading of the measured datapoints (csv) -> monitoring.html
2. creating/adding and removing profilecurves -> create-profile.html


## TODO / PLANS

Implementing IotWebConf into this project would be a good idea. Then you could add the oven to your local network as http://yarc.local/


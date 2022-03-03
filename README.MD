# YARC - Yet Another Reflow Controller

### Setup Instructions

1. Install plugin pltformio
2. Create project for specific controller
3. To match your wiring set the defines the corresponding value in _pindefines.h_
4. TFT_eSPI -> User_Setup.h
5. lvgl -> lv_conf.h (lv use canvas 1 !!!)
6. lv_lib_qrcode -> #include "../lvgl/lvgl.h" (more todo in the .c file see todo.txt)
7. In platformIo Build the Filesystemimage (data-folder) and upload it to the esp32 if those files are missing, the mcu will crash
8. read todo.txt
9. profit???


# WEBINTERFACE

At this time there are two functions implemented in the webinterface of the controller

1. live monitoring and downloading of the measured datapoints (csv) -> monitoring.html
2. creating/adding and removing profilecurves -> create-profile.html

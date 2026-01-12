# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c "./components/File_Manager.c" "./components/WiFi_Manager.c" "./components/Cloud_Manager.c" "./components/Acquisition_Manager.c" "./components/RTC_Manager.c" "./components/Motorpump_Manager.c" "./components/instruments/ADS1219.c" "./components/instruments/DS18B20.c" "./components/instruments/HX711.c" "./components/instruments/LJ12A3_4_Z.c"
    INCLUDE_DIRS        "./components" "./components/instruments"# optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
    EMBED_TXTFILES		"certs/root_ca.pem"
)

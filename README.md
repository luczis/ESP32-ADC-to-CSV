# ESP32-ADC-to-CSV
Couple of sofwares utilized to read analog data in bulk, and save to csv files

# ADC to file

This is supposed to read the ADC from the ESP32 and output to a .out file. 

There is two slight different algorithms. That's because the write time can take time, and the sampling rate must be slower to catch up.

- Long

The internal buffer is small, so it can create larger files, but with slower sampling rate.

- Fast

All the data is allocated to RAM, before going to the SD card. Sampling rate is faster, but size is limited.

# File to CSV

Due to the size of an ASCII text file, compared to a raw binary, it's better to output raw data from the ESP32. The file is later converted via an output software.

To compile just access ./FiletoCSV folder, and cmake it.

To convert the .out file just use ./FiletoCSV <outfileName>.


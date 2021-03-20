//
// "Fast Shot" version of the software to convert AD input to file
// The Fast Shot version is for faster sampling time
// However the output is a buffer in RAM, which makes the sampling
// set smaller.
// 
// * Colaboradores:
// - Lucas Zischler
//

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "include/reset.h"
#include "include/sd_funk.h"

#define SD_CS		5
#define SPI_MOSI	23
#define SPI_MISO	19
#define SPI_SCK		18

#define ADC_0		33
#define ADC_1		34
#define ADC_2		35
#define ADC_3		36

#define CORE_0 0
#define CORE_1 1

#define PRIORITY_0 0
#define PRIORITY_1 1
#define PRIORITY_2 2
#define PRIORITY_3 3

#define TIMER_0 0
#define TIMER_1 1
#define TIMER_2 2
#define TIMER_3 3

#define DATA_SIZE 6

File output_file;

const char header_text[] = 
"//\n// Results from the 12bit AD\n// Developed by Lucas Zischler\n// In the need of help, contact: https://www.lucas.zischler.nom.br/\n//\n";

// Settings
const char folder_name[] = "/Medicao";		// Full path to output file folder
const char file_name[] = "output";		// Base name for the output file
const uint16_t sample_time = 5;			// Time (in us) between measurements 	!! It must be greater or equal to 5 !!
const uint32_t sample_set = 1000 * DATA_SIZE;	// Amount of data gattered. 		!! It must be a multiple of DATA_SIZE !!

// Tasks and functions declarations
TaskHandle_t SetupCardTask;
void SetupCardTaskFunction(void* parameters);
TaskHandle_t SetupADTask;
void SetupADTaskFunction(void* parameters);
TaskHandle_t ReadADTask;
void ReadADTaskFunction(void* parameters);
TaskHandle_t StoreDataTask;
void StoreDataTaskFunction(void* parameters);

void setup()
{
	// Serial configuration
	Serial.begin(115200);

	// Utilized for debug, show what caused the last system reset
	Serial.println("CPU0 reset reason:");
	print_reset_reason(rtc_get_reset_reason(0));
	verbose_print_reset_reason(rtc_get_reset_reason(0));
	Serial.println("CPU1 reset reason:");
	print_reset_reason(rtc_get_reset_reason(1));
	verbose_print_reset_reason(rtc_get_reset_reason(1));

	// SD card setup task start
	xTaskCreate(SetupCardTaskFunction, "SetupCardTask", 10000, NULL, 0, &SetupCardTask);
}

void SetupCardTaskFunction(void*parameters) {
	// Setup SPI
	pinMode(SD_CS, OUTPUT);
	digitalWrite(SD_CS, LOW);
	SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

	// Setup SD
	if(!SD.begin()){
		Serial.println("Card Mount Failed");
		vTaskDelete(NULL);
	}
	uint8_t cardType = SD.cardType();
	if(cardType == CARD_NONE){
		Serial.println("No SD card attached");
		vTaskDelete(NULL);
	}

	// Get card size
	uint64_t cardSize = SD.cardSize() / (1024 * 1024);
	Serial.printf("SD Card Size: %lluMB\n", cardSize);

	// Create output dir
	SD.mkdir(folder_name);

	// Check existing files
	File root = SD.open(folder_name);
	if(!root.isDirectory()){
        	Serial.println("Not a directory");
		vTaskDelete(NULL);
	}

	bool file_exist = false;
	uint16_t file_number = 0;
	char new_file_name[256];
	do {
		uint8_t pos = 0;
		// Folder
		for(uint8_t i=0; folder_name[i] != '\0'; i++) {
			new_file_name[pos++] = folder_name[i];
		}
		if(new_file_name[pos] != '/')
			new_file_name[pos++] = '/';

		// File
		for(uint8_t i=0; file_name[i] != '\0'; i++) {
			new_file_name[pos++] = file_name[i];
		}

		pos = appendNumberToString(new_file_name, pos, file_number);

		// End file name
		new_file_name[pos++] = '.';
		new_file_name[pos++] = 'o';
		new_file_name[pos++] = 'u';
		new_file_name[pos++] = 't';
		new_file_name[pos++] = '\0';

		// Check all files in output folder
		file_exist = false;
		File sub_file = root.openNextFile();
		while(sub_file) {
			if(!sub_file.isDirectory()) {
				if(compareStrings(sub_file.name(), new_file_name)) {
					file_exist = true;
					file_number ++;
					for(uint8_t i=0; i<pos; i++)
						new_file_name[i] = '\0';
					break;
				}
			}
			sub_file = root.openNextFile();
		}
	} while(file_exist);

	// Put sample time in file
	uint8_t pos = 0;
	char sample_time_header[256];
	sample_time_header[pos++] = 'S';
	sample_time_header[pos++] = 'T';
	sample_time_header[pos++] = ' ';
	pos = appendNumberToString(sample_time_header, pos, sample_time);
	sample_time_header[pos++] = 'u';
	sample_time_header[pos++] = 's';
	sample_time_header[pos++] = '\n';
	sample_time_header[pos] = '\0';

	// Create output file
	Serial.println(new_file_name);
	output_file = SD.open(new_file_name, FILE_WRITE);
	output_file.print(header_text);
	output_file.print(sample_time_header);

	// AD setup task start
	xTaskCreate(SetupADTaskFunction, "SetupADTask", 10000, NULL, 0, &SetupADTask);
	vTaskDelete(NULL);
}

hw_timer_t * timerInt = NULL;
bool reading_ad = false;
void int1ms() {
	if(reading_ad)
		vTaskResume(ReadADTask);
}

void SetupADTaskFunction(void* parameters) {
	//Setup timer
	timerInt = timerBegin(TIMER_0, 80, true);
	timerAttachInterrupt(timerInt, &int1ms, true);
	timerAlarmWrite(timerInt, sample_time, true);
	timerAlarmEnable(timerInt);

	// Loop tasks start
	xTaskCreate(ReadADTaskFunction, "ReadADTask", 1000, NULL, 0, &ReadADTask);
	vTaskDelete(NULL);
}

uint8_t data_buffer[sample_set];
bool current_buffer = 0;
void ReadADTaskFunction(void* parameters) {
	reading_ad = true;

	while(reading_ad) {
		vTaskSuspend(NULL);

		// Read values
		static uint16_t adc0val, adc1val, adc2val, adc3val;
		adc0val = analogRead(ADC_0);
		adc1val = analogRead(ADC_1);
		adc2val = analogRead(ADC_2);
		adc3val = analogRead(ADC_3);

		// Pass data to buffer
		static uint16_t position_count = 0;
		data_buffer[position_count++] = (uint8_t)((adc0val & 0x0ff0)>>4);
		data_buffer[position_count++] = (uint8_t)((adc0val & 0x000f)<<4) | (uint8_t)((adc1val & 0x0f00)>>8);
		data_buffer[position_count++] = (uint8_t)((adc1val & 0x00ff)<<0);
		data_buffer[position_count++] = (uint8_t)((adc2val & 0x0ff0)>>4);
		data_buffer[position_count++] = (uint8_t)((adc2val & 0x000f)<<4) | (uint8_t)((adc3val & 0x0f00)>>8);
		data_buffer[position_count++] = (uint8_t)((adc3val & 0x00ff)<<0);
	
		// Swap buffers, and flush the one filled
		if(position_count >= sample_set) {
			timerAlarmDisable(timerInt);
			reading_ad = false;
			vTaskResume(ReadADTask);
			xTaskCreate(StoreDataTaskFunction, "StoreDataTask", 5000, NULL, 0, &StoreDataTask);
		}
	}

	while(1)
		vTaskDelay(10/portTICK_PERIOD_MS);
	vTaskSuspend(NULL);
}

void StoreDataTaskFunction(void* parameters) {
	// Flush data in buffer
	output_file.write(data_buffer, sample_set);

	output_file.close();
	Serial.println("Done");

	while(1)
		vTaskDelay(10/portTICK_PERIOD_MS);
	vTaskSuspend(NULL);
}

void loop() {};

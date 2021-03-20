#include <iostream>
#include <stdlib.h>
#include <fstream>

const char output_header[] = "Time(us),ADC0,ADC1,ADC2,ADC3\n";

uint32_t stringToUInt(char* s) {
	uint32_t result = 0;
	uint8_t pos = 0;
	for(; s[pos]!='\0'; pos++);
	for(uint8_t i=0; i<pos; i++) {
		uint32_t temp_value = (s[i]&0xf);
		for(uint8_t j=1; j<(pos-i); j++)
			temp_value *= 10;
		result += temp_value;
	}

	return result;
}

uint8_t uintToString(char * s, uint32_t val) {
	if(val == 0) {
		s[0] = '0';
		s[1] = '\0';
		return 2;
	}
	// Get the amount of chars need to add
	uint8_t pos = 0;
	uint8_t char_number = 0;
	uint16_t temp_result = val;
	while(temp_result != 0) {
		char_number++;
		temp_result /= 10;
	}

	// Append char
	uint16_t temp_file_number = val;
	for(uint8_t i=char_number; i>0; i--) {
		temp_result = temp_file_number;
		for(uint8_t j=0; j<(i-1); j++)
			temp_result /= 10;
		s[pos++] = temp_result | 0x30;
		for(uint8_t j=0; j<(i-1); j++)
			temp_result *= 10;
		temp_file_number -= temp_result;
	}

	s[pos++] = '\0';
	return pos;
}

int main(int argc, const char* argv[]) {
	if(argc != 2) {
		printf("Invalid argument. Please provide one file.\n");
		return 1;
	}
	
	// Get input file name
	const char* input_file_path = argv[1];
	uint8_t ifp_size = 0;
	for(; input_file_path[ifp_size] != '\0'; ifp_size++);
	if((input_file_path[ifp_size-4]!='.')|(input_file_path[ifp_size-3]!='o')|(input_file_path[ifp_size-2]!='u')|(input_file_path[ifp_size-1]!='t')) {
		printf("Invalid file\n");
		return 1;
	}

	// Open input file
	std::ifstream input_file(input_file_path, std::ios::binary);
	if(!input_file.is_open()) {
		printf("Failure to open file\n");
		return 1;
	}
	
	// Get output file name
	char output_file_path[ifp_size];
	for(uint8_t i=0; i<ifp_size-3; i++)
		output_file_path[i]=input_file_path[i];
	output_file_path[ifp_size-3] = 'c';
	output_file_path[ifp_size-2] = 's';
	output_file_path[ifp_size-1] = 'v';
	output_file_path[ifp_size] = '\0';

	// Open output file
	std::ofstream output_file(output_file_path);
	if(!input_file.is_open()) {
		printf("Failure to open file\n");
		return 1;
	}
	output_file << output_header;

	uint32_t sample_time;

	uint8_t data_buffer[6];
	while(!input_file.eof()) {
		input_file.read((char*) &data_buffer[0], 2);
		// Comment
		if((data_buffer[0] == '/')&&(data_buffer[1] == '/')) {
			do {
				input_file.read((char*) &data_buffer[0], 1);
			} while(data_buffer[0] != '\n');
			continue;
		}
		// Sample Time
		else if((data_buffer[0] == 'S')&&(data_buffer[1] == 'T')) {
			uint8_t pos = 0;
			char st_text[256];
			do {
				input_file.read((char*) &data_buffer[0], 1);
				if((0x30 <= data_buffer[0]) && (data_buffer[0] <= 0x39))
					st_text[pos++] = (char)data_buffer[0];
				if(data_buffer[0] == 'm') // Add 3 zeros if mili
					for(uint8_t i=0; i<3; i++)
						st_text[pos++] = '0';
			} while(data_buffer[0] != '\n');
			st_text[pos] = '\0';
			sample_time = stringToUInt(st_text);
			break;
		}
	}
	// Data
	uint32_t line_count = 0;
	uint16_t value;
	char value_text[256];
	while(!input_file.eof()) {
		input_file.read((char*) &data_buffer[0], 6);
		// Time
		output_file << (line_count++)*sample_time << ',';
		// ADC0
		value = ((data_buffer[0]&0xff)<<4)|((data_buffer[1]&0xf0)>>4);
		uintToString(value_text, value);
		output_file << value_text << ',';
		// ADC1
		value = ((data_buffer[1]&0x0f)<<8)|((data_buffer[2]&0xff)>>0);
		uintToString(value_text, value);
		output_file << value_text << ',';
		// ADC2
		value = ((data_buffer[3]&0xff)<<4)|((data_buffer[4]&0xf0)>>4);
		uintToString(value_text, value);
		output_file << value_text << ',';
		// ADC3
		value = ((data_buffer[4]&0x0f)<<8)|((data_buffer[5]&0xff)>>0);
		uintToString(value_text, value);
		output_file << value_text << '\n';
	}

	output_file.close();
	input_file.close();
	return 0;
}

//Aasys Sresta
//One wire temp sensor
//DS18B20 

#include <u.h>
#include <libc.h>

#define PRECISION  0.0625

#define SKIP_ROM 0xCC
#define TCONV 0x44
#define READ_SCRATCH 0xBE

#define PIN27_OUT fprint(f_gpio, "function 27 out")
#define PIN27_IN fprint(f_gpio, "function 27 in")
#define PIN27_PULSE fprint(f_gpio, "function 27 pulse")
#define PIN27_LOW fprint(f_gpio, "set 27 0")
#define PIN27_HIGH fprint(f_gpio, "set 27 1")

int f_gpio;
char buf[20];
int ds_buf[9*8];

void open_gpio(void) {
	print("Opening GPIO...\n");

	f_gpio = open("/dev/gpio", ORDWR);

	if (f_gpio < 0) {
		bind("#G", "/dev", MAFTER);
		f_gpio = open("/dev/gpio", ORDWR);
		
		if (f_gpio < 0) {
			print("GPIO open error %r\n");
		}
	}
}

void big_delay(int ncalls) {
	for (int i = 0; i < ncalls; i++) {
		nsec();
		nsec();
	}
}

int read_bus(void) {
	//exec time 70-100us
	read(f_gpio, buf, 16);
	buf[16] = 0;
	int pin27 = strtoull(buf, nil, 16) & (1 << 27);
	return !pin27 == 0;
}

void print_bus(void) {
	print("%d\n", read_bus());
}

void bus_write(char command) {
	for (int i = 0; i < 8; i++) {
		//extract bit
		if ((command >> i) & 0x01) {
			PIN27_LOW;
			PIN27_PULSE;
			
			// ~60us
			big_delay(4);
		} else {
			PIN27_OUT;
			PIN27_LOW;

			// ~60us
			big_delay(4);
			
			PIN27_IN;
			big_delay(1);
		}
	}
}

int extract_bit(char c, int index) {
	return (c >> index) & 0x01 ;
}


void read_scratch(void) {
	print("Reading Scratch...\n");
	bus_write(SKIP_ROM);
	bus_write(READ_SCRATCH);

		
	for (int i = 0; i < 8 * 9; i = i + 8) {
		for (int j = 7; j >= 0; j--) {
			PIN27_PULSE;
			ds_buf[i + j] = read_bus();
		}
	}
	
	for (int i = 0; i < 9; i++) {
		print("%d -> ", i);
		for (int j = 0; j < 8; j++) {
			print("%d", ds_buf[i * 8 + j]);
		}
		print("\n");
	}
	
}

int bus_reset(void) {
	// INITIALIZE
	print("\nReseting...\n");

	//bring it low
	PIN27_OUT;
	PIN27_LOW;

	//sleep ~500us
	big_delay(40);

	PIN27_IN;
	big_delay(3);
	
	//print_bus();
	int presence = read_bus();

	// sleep ~ 500us
	big_delay(30);

	int end_presence = read_bus();
	if (presence == 0 && end_presence == 1 ) {
		print("Presence detected\n\n");
	} else {	
		print("No Presence detected!!\n\n");
	}
	return presence == 0 && end_presence == 1 ;
}


void delay_tester(int x) {
	vlong start = nsec();

	big_delay(x);
	
	vlong stop = nsec();
	print("%d", x);
	print (" -> %d\n", stop - start);
}

int two_power(int n) {
	if (n == 0)
		return 1;
	else
		return 2 * two_power(n - 1);
}

void read_temp() {
	//rearrange 12 bits LSB --> MSB
	int temp[12];
	
	for (int i = 0; i < 4; i++) { 
		temp[11- i ] = ds_buf[i+12];
	}
	for (int i = 0; i < 8; i++) { 
		temp[ 11 - i - 4] = ds_buf[i];
	}

	if (temp[11]==1) {
		//if negative flip all bits

		for (int i = 0; i < 11; i++) { 
			temp[i] = temp[i] == 1 ? 0 : 1;
		}

		//add 1
		for (int i = 0; i < 11; i++) { 
			if (temp[i] == 0) {
				temp[i] = 1;
				break;
			} else {
				temp[i] = 0;
			}
		}
	}

	int d_temp = 0;

	// convert to decimal
	for (int i = 0; i < 11; i++) { 
		if (temp[i] == 1)
			d_temp = d_temp + two_power(i);
	}

	//negative??
	if (temp[11]==1) {
		d_temp = d_temp * -1;
	}

	double actual_temp = PRECISION * d_temp;

	print("Temp (degree C) = %f\n", actual_temp);


}

void main(void) {
	open_gpio();
	
	PIN27_OUT;
	//PIN27_LOW;
	//PIN27_HIGH;

	PIN27_IN;

	if (!read_bus()) {
		print("Error, BUS not active, is the pin connected?\n");
	} else {

		if (bus_reset()) {
			bus_write(SKIP_ROM);
			bus_write(TCONV);
			
			sleep(1000);

			if (bus_reset()) {
				read_scratch();
				read_temp();
			}


		}
	}

}

void main2(void) {
	open_gpio();

	for(int i = 0; i < 50; i++) {
		delay_tester(i);
	}
}

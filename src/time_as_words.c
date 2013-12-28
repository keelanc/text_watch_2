#include "time_as_words.h"


static const char* const ONES[] = {
	"clock",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"ten",
	"eleven",
	"twelve"
};

static const char* const TEENS[] = {
	"",
	"eleven",
	"twelve",
	"thirteen",
	"four",
	"fifteen",
	"sixteen",
	"seven",
	"eight",
	"nine"
};

static const char* const TENS[] = {
	"o'",
	"ten",
	"twenty",
	"thirty",
	"forty",
	"fifty"
};


void time_as_words(int int_hour, int int_min, char* str_hour, char* str_tens, char* str_ones) {
	
	strcpy(str_hour, "");
	strcpy(str_tens, "");
	strcpy(str_ones, "");
	
	//hour
	if (int_hour % 12 == 0) {
		strcat(str_hour, ONES[12]);
	}
	else {
		strcat(str_hour, ONES[int_hour % 12]);
	}
	
	//minute
	if (int_min > 10 && int_min < 20) {
		strcat(str_tens, TEENS[int_min - 10]);
        if (int_min==14 || int_min==17 || int_min==18 || int_min==19) {
            strcat(str_ones, "teen");
        }
        else {
            strcat(str_ones, "");
        }
	}
	else {
		strcat(str_tens, TENS[int_min / 10]);
		if (int_min % 10 != 0 || int_min == 0) {
			strcat(str_ones, ONES[int_min % 10]);
		}
	}
	
}
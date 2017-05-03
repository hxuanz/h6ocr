#pragma once

enum H6OCRERROR 
{ 
	SUCCESS = 0,
	INVILD_URI = 10,
	INVILD_IMAGE,
	INVILD_DICTIONARY,
	perspective_detectLines = 20, 
	cutAndOcr_cut_Vertical = 30,
	cutAndOcr_keys_null,
	cutAndOcr_values_null,
	cutAndOcr_keys_greater,
	cutAndOcr_values_greater,
};

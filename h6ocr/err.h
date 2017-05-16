#pragma once

enum H6OCRERROR 
{ 
	SUCCESS = 0,
	INVILD_URI = 10,
	INVILD_PARAMS,
	INVILD_PARAMS_imageData,
	INVILD_PARAMS_scaleData,
	INVILD_IMAGE,
	INVILD_DICTIONARY,
	perspective_detectLines = 20, 
	ERR_loadDictionary_Json_parse,

	cutAndOcr_cut_Vertical = 30,
	cutAndOcr_keys_null,
	cutAndOcr_values_null,
	cutAndOcr_keys_greater,
	cutAndOcr_values_greater,
};

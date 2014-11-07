#include "util.h"

char config_sdmc_path[0x200];
bool config_has_sdmc = false;
bool config_sdmcwriteable = false;
bool config_slotone = false;
bool config_usesys = false;
bool config_nand_cfg_save = false;
char config_sysdataoutpath[0x200]; //0x200 is MAX file path

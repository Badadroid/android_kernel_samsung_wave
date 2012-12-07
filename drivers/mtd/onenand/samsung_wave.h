/* linux/drivers/mtd/onenand/samsung_wave.h
 *
 * Partition Layouts for Samsung Wave S8500/S8530 Bada 2.0
 *
 */

struct mtd_partition wave_s8500_partition_info[] = {
	{
		.name           = "nv_data",
		.offset         = (1948*SZ_256K), //0x1E700000
		.size           = (20*SZ_256K),
	},
	
	{
		.name           = "fota",
		.offset         = (204*SZ_256K),
		.size           = (12*SZ_256K),
	},
};

struct mtd_partition wave_s8530_partition_info[] = {
	{
		.name           = "nv_data",
		.offset         = (1984*SZ_256K), //0x1F000000
		.size           = (20*SZ_256K),
	},
	{
		.name           = "fw_block",
		.offset         = (16*SZ_256K), //0x00600000
		.size           = (592*SZ_256K),
	},
	{
		.name           = "dbl",
		.offset         = (16*SZ_256K), //0x00400000
		.size           = (8*SZ_256K),
	},		
	{
		.name           = "amss",
		.offset         = (24*SZ_256K), //0x00600000
		.size           = (54*SZ_256K),
	},		
	{
		.name           = "apps",
		.offset         = (76*SZ_256K), //0x01300000
		.size           = (140*SZ_256K),
	},		
	{
		.name           = "rsrc1",
		.offset         = (216*SZ_256K), //0x03600000
		.size           = (260*SZ_256K),
	},		
	{
		.name           = "csc",
		.offset         = (476*SZ_256K), //0x07700000
		.size           = (120*SZ_256K),
	},	
	{
		.name           = "fota",
		.offset         = (596*SZ_256K), //0x09500000
		.size           = (12*SZ_256K),
	},	
	{
		.name           = "stl1",
		.offset         = (608*SZ_256K), //0x09800000
		.size           = (988*SZ_256K),
	},	
	{
		.name           = "stl2",
		.offset         = (1596*SZ_256K), //0x18F00000
		.size           = (372*SZ_256K),
	},	
	{
		.name           = "secdata",
		.offset         = (1968*SZ_256K), //0x1EC00000
		.size           = (16*SZ_256K),
	},
};





zreladdr-y	:= 0x20008000
params_phys-y	:= 0x20000100

# override for Herring
zreladdr-$(CONFIG_MACH_HERRING)		:= 0x30008000
params_phys-$(CONFIG_MACH_HERRING)	:= 0x30000100

# override for Aries
zreladdr-$(CONFIG_MACH_ARIES)		:= 0x30008000
params_phys-$(CONFIG_MACH_ARIES)	:= 0x30000100

# override for P1
zreladdr-$(CONFIG_MACH_P1)		:= 0x30008000
params_phys-$(CONFIG_MACH_P1)		:= 0x30000100

# override for Wave
zreladdr-$(CONFIG_MACH_WAVE)		:= 0x20008000
params_phys-$(CONFIG_MACH_WAVE)		:= 0x20000100

This folder contains bootloader which have been patched so that they do not check the Unique ID of the MK22 MCU and compare it with the signature bytes in the MCU ROM

This bootloader is ONLY intended to be used by developers and anyone who need to reflash their MCU ROM because the signature bytes are corrupted or erased.


On the GD-77 the signature bytes are at 0x7F800 and if these bytes become erased or corrupted the bootloader will not allow updates to the firmware.
In this case the green LED on the top of the radio flashes, rather than remaining permanently on, and as soon as buttons SK1 and SK2 are released the bootloader runs the main firmware.

On the Baofeng DM-1801 the signature bytes appear to be in a different address, and the bootloader is different in other ways and a patched version has not been created yet


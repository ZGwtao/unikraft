
#include <uk/microkit.h>

char microkit_name[MICROKIT_PD_NAME_LENGTH];

seL4_Word microkit_irqs;
seL4_Word microkit_notifications;
seL4_Word microkit_pps;
seL4_Word microkit_ioports;

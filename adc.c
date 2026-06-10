 #include <xc.h>

 #include "adc.h"
 
 void init_adc(void)
 {
     ADFM = 1;          // Right justified
 
     ACQT2 = 0;
     ACQT1 = 1;
     ACQT0 = 0;
 
     ADCS0 = 0;
     ADCS1 = 1;
     ADCS2 = 0;
 
     VCFG0 = 0;
     VCFG1 = 0;
 
     ADON = 1;
 }
 
 unsigned short read_adc(unsigned char channel)
 {
     unsigned int reg_val;
 
     /*select the channel*/
     ADCON0 = (ADCON0 & 0xC3) | (channel << 2);
     
     /* Start the conversion */
     GO = 1;
     while (GO);
     reg_val = (ADRESH << 8) | ADRESL; 
 
     return reg_val;
 }
 
 
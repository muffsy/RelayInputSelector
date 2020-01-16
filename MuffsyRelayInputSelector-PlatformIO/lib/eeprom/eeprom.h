#ifndef __EEPROM__H_
#define __EEPROM__H_

/***********************************************************
Author: Bernard Borredon
Date : 21 decembre 2015
Version: 1.3
  - Correct write(uint32_t address, int8_t data[], uint32_t length) for eeprom >= T24C32.
    Tested with 24C02, 24C08, 24C16, 24C64, 24C256, 24C512, 24C1025 on LPC1768 (mbed online and µVision V5.16a).
  - Correct main test.
    
Date : 12 decembre 2013
Version: 1.2
  - Update api documentation
  
Date: 11 december 2013
Version: 1.1
  - Change address parameter size form uint16_t to uint32_t (error for eeprom > 24C256).
  - Change size parameter size from uint16_t to uint32_t (error for eeprom > 24C256).
    - Add EEPROM name as a private static const char array.
    - Add function getName.
    - Add a test program.

Date: 27 december 2011
Version: 1.0
************************************************************/

// Includes
#include <string> 

#include "mbed.h"

// Example
/*
#include <string>

#include "mbed.h"
#include "eeprom.h"

#define EEPROM_ADDR 0x0   // I2c EEPROM address is 0x00

#define SDA p9            // I2C SDA pin
#define SCL p10           // I2C SCL pin

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

DigitalOut led2(LED2);

typedef struct _MyData {
                         int16_t sdata;
                         int32_t idata;
                         float fdata;
                       } MyData;

static void myerror(std::string msg)
{
  printf("Error %s\n",msg.c_str());
  exit(1);
}

void eeprom_test(void)
{
  EEPROM ep(SDA,SCL,EEPROM_ADDR,EEPROM::T24C64);  // 24C64 eeprom with sda = p9 and scl = p10
  uint8_t data[256],data_r[256];
  int8_t ival;
  uint16_t s;
  int16_t sdata,sdata_r;
  int32_t ldata[1024];
  int32_t eeprom_size,max_size;
  uint32_t addr;
  int32_t idata,idata_r;
  uint32_t i,j,k,l,t,id;
  float fdata,fdata_r;
  MyData md,md_r;
    
  eeprom_size = ep.getSize();
  max_size = MIN(eeprom_size,256);
  
  printf("Test EEPROM I2C model %s of %d bytes\n\n",ep.getName(),eeprom_size);
  
  // Test sequential read byte (max_size first bytes)
  for(i = 0;i < max_size;i++) {
     ep.read(i,ival);
     data_r[i] = ival;
     if(ep.getError() != 0)
       myerror(ep.getErrorMessage());
  }
  
  printf("Test sequential read %d first bytes :\n",max_size);
  for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
    
    // Test sequential read byte (max_size last bytes)
  for(i = 0;i < max_size;i++) {
        addr = eeprom_size - max_size + i;
    ep.read(addr,ival);
    data_r[i] = ival;
    if(ep.getError() != 0)
      myerror(ep.getErrorMessage());
  }
  
  printf("\nTest sequential read %d last bytes :\n",max_size);
  for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
  
  // Test write byte (max_size first bytes)
  for(i = 0;i < max_size;i++)
     data[i] = i;
  
  for(i = 0;i < max_size;i++) {
     ep.write(i,(int8_t)data[i]);
     if(ep.getError() != 0)
       myerror(ep.getErrorMessage());
  }
  
  // Test read byte (max_size first bytes)
  for(i = 0;i < max_size;i++) {
     ep.read(i,(int8_t&)ival);
     data_r[i] = (uint8_t)ival;
     if(ep.getError() != 0)
       myerror(ep.getErrorMessage());
  }
  
  printf("\nTest write and read %d first bytes :\n",max_size);
  for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
  
  // Test current address read byte (max_size first bytes)
  ep.read((uint32_t)0,(int8_t&)ival); // current address is 0
  data_r[0] = (uint8_t)ival;
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  
  for(i = 1;i < max_size;i++) {
     ep.read((int8_t&)ival);
     data_r[i] = (uint8_t)ival;
     if(ep.getError() != 0)
       myerror(ep.getErrorMessage());
  }
  
  printf("\nTest current address read %d first bytes :\n",max_size);
  for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
   
  // Test sequential read byte (first max_size bytes)
  ep.read((uint32_t)0,(int8_t *)data_r,(uint32_t) max_size);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  
  printf("\nTest sequential read %d first bytes :\n",max_size);
  for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
  
  // Test write short, long, float 
  sdata = -15202;
    addr = eeprom_size - 16;
  ep.write(addr,(int16_t)sdata); // short write at address eeprom_size - 16
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  
  idata = 45123;
    addr = eeprom_size - 12;
  ep.write(addr,(int32_t)idata); // long write at address eeprom_size - 12
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
    
  fdata = -12.26;
    addr = eeprom_size - 8;
  ep.write(addr,(float)fdata); // float write at address eeprom_size - 8
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  
  // Test read short, long, float
  printf("\nTest write and read short (%d), long (%d), float (%f) :\n",
           sdata,idata,fdata);  
  
  ep.read((uint32_t)(eeprom_size - 16),(int16_t&)sdata_r);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  printf("sdata %d\n",sdata_r);
  
  ep.read((uint32_t)(eeprom_size - 12),(int32_t&)idata_r);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  printf("idata %d\n",idata_r);
  
  ep.read((uint32_t)(eeprom_size - 8),fdata_r);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  printf("fdata %f\n",fdata_r);
  
  // Test read and write a structure
  md.sdata = -15203;
  md.idata = 45124;
  md.fdata = -12.27;
 
  ep.write((uint32_t)(eeprom_size - 32),(void *)&md,sizeof(md)); // write a structure eeprom_size - 32
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
    
  printf("\nTest write and read a structure (%d %d %f) :\n",md.sdata,md.idata,md.fdata);
  
  ep.read((uint32_t)(eeprom_size - 32),(void *)&md_r,sizeof(md_r));
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
  
  printf("md.sdata %d\n",md_r.sdata);
  printf("md.idata %d\n",md_r.idata);
  printf("md.fdata %f\n",md_r.fdata);
    
    // Test read and write of an array of the first max_size bytes
    for(i = 0;i < max_size;i++)
       data[i] = max_size - i - 1;
    
    ep.write((uint32_t)(0),data,(uint32_t)max_size);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
    
    ep.read((uint32_t)(0),data_r,(uint32_t)max_size);
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
    
    printf("\nTest write and read an array of the first %d bytes :\n",max_size);
    for(i = 0;i < max_size/16;i++) {
     for(j = 0;j < 16;j++) {
        addr = i * 16 + j;
        printf("%3d ",(uint8_t)data_r[addr]);
     }
     printf("\n");
  }
    printf("\n");
  
  // Test write and read an array of int32
  s = eeprom_size / 4;                // size of eeprom in int32
  int ldata_size = sizeof(ldata) / 4; // size of data array in int32
  l = s / ldata_size;                 // loop index
  
  // size of read / write in bytes
  t = eeprom_size;
  if(t > ldata_size * 4)
    t = ldata_size * 4;
  
  printf("Test write and read an array of %d int32 (write entire memory) :\n",t/4);

  // Write entire eeprom
    if(l) {
    for(k = 0;k < l;k++) {
       for(i = 0;i < ldata_size;i++)
          ldata[i] = ldata_size * k + i;
        
       addr = k * ldata_size * 4;
       ep.write(addr,(void *)ldata,t);
       if(ep.getError() != 0)
         myerror(ep.getErrorMessage());
    }  
    
      printf("Write OK\n");
    
    // Read entire eeprom
      id = 0;
    for(k = 0;k < l;k++) {
       addr = k * ldata_size * 4;
       ep.read(addr,(void *)ldata,t);
       if(ep.getError() != 0)
         myerror(ep.getErrorMessage());
  
       // format outputs with 8 words rows
       for(i = 0;i < ldata_size / 8;i++) {
                id++;
          printf("%4d ",id);
          for(j = 0;j < 8;j++) {
             addr = i * 8 + j;
             printf("%5d ",ldata[addr]);
          }
          printf("\n");
       }
    }
  }
    else {
        for(i = 0;i < s;i++)
       ldata[i] = i;
        
    addr = 0;
    ep.write(addr,(void *)ldata,t);
    if(ep.getError() != 0)
      myerror(ep.getErrorMessage());
        
        printf("Write OK\n");
    
    // Read entire eeprom
      id = 0;
    
    addr = 0;
    ep.read(addr,(void *)ldata,t);
    if(ep.getError() != 0)
      myerror(ep.getErrorMessage());
  
    // format outputs with 8 words rows
    for(i = 0;i < s / 8;i++) {
             id++;
       printf("%4d ",id);
       for(j = 0;j < 8;j++) {
          addr = i * 8 + j;
          printf("%5d ",ldata[addr]);
       }
       printf("\n");
    }
    }
  
  // clear eeprom
  printf("\nClear eeprom\n");

  ep.clear();
  if(ep.getError() != 0)
    myerror(ep.getErrorMessage());
    
  printf("End\n");  
    
}

int main() 
{

  eeprom_test();
    
  return(0);
}
*/

// Defines
#define EEPROM_Address     0xa0

#define EEPROM_NoError     0x00
#define EEPROM_BadAddress  0x01
#define EEPROM_I2cError    0x02
#define EEPROM_ParamError  0x03
#define EEPROM_OutOfRange  0x04
#define EEPROM_MallocError 0x05

#define EEPROM_MaxError       6

static std::string _ErrorMessageEEPROM[EEPROM_MaxError] = {
                                                            "",
                                                            "Bad chip address",
                                                            "I2C error (nack)",
                                                            "Invalid parameter",
                                                            "Data address out of range",
                                                            "Memory allocation error"
                                                          };

/** EEPROM Class
*/
class EEPROM {
public:
    enum TypeEeprom {T24C01=128,T24C02=256,T24C04=512,T24C08=1024,T24C16=2048,
                     T24C32=4096,T24C64=8192,T24C128=16384,T24C256=32768,
                     T24C512=65536,T24C1024=131072,T24C1025=131073} Type;
                                         
    /**
     * Constructor, initialize the eeprom on i2c interface.
     * @param sda sda i2c pin (PinName)
     * @param scl scl i2c pin (PinName)
     * @param address eeprom address, according to eeprom type (uint8_t)
     * @param type eeprom type (TypeEeprom) 
     * @return none
    */
    EEPROM(PinName sda, PinName scl, uint8_t address, TypeEeprom type);
    
    /**
     * Random read byte
     * @param address start address (uint32_t)
     * @param data byte to read (int8_t&)
     * @return none
    */
    void read(uint32_t address, int8_t& data);
    
    /**
     * Random read short
     * @param address start address (uint32_t)
     * @param data short to read (int16_t&)
     * @return none
    */
    void read(uint32_t address, int16_t& data);
    
    /**
     * Random read long
     * @param address start address (uint32_t)
     * @param data long to read (int32_t&)
     * @return none
    */
    void read(uint32_t address, int32_t& data);
    
    /**
     * Random read float
     * @param address start address (uint32_t)
     * @param data float to read (float&)
     * @return none
    */
    void read(uint32_t address, float& data);
    
    /**
     * Random read anything
     * @param address start address (uint32_t)
     * @param data data to read (void *)
     * @param size number of bytes to read (uint32_t)
     * @return none
    */
    void read(uint32_t address, void *data, uint32_t size);
    
    /**
     * Current address read byte
     * @param data byte to read (int8_t&)
     * @return none
    */
    void read(int8_t& data);
    
    /**
     * Sequential read byte
     * @param address start address (uint32_t)
     * @param data bytes array to read (int8_t[]&)
     * @param size number of bytes to read (uint32_t)
     * @return none
    */
    void read(uint32_t address, int8_t *data, uint32_t size);
    
    /**
     * Write byte
     * @param address start address (uint32_t)
     * @param data byte to write (int8_t)
     * @return none
    */
    void write(uint32_t address, int8_t data);
    
    /**
     * Write short
     * @param address start address (uint32_t)
     * @param data short to write (int16_t)
     * @return none
    */
    void write(uint32_t address, int16_t data);
    
    /**
     * Write long
     * @param address start address (uint32_t)
     * @param data long to write (int32_t)
     * @return none
    */
    void write(uint32_t address, int32_t data);
    
    /**
     * Write float
     * @param address start address (uint32_t)
     * @param data float to write (float)
     * @return none
    */
    void write(uint32_t address, float data);
    
    /**
     * Write anything (use the page write mode)
     * @param address start address (uint32_t)
     * @param data data to write (void *)
     * @param size number of bytes to write (uint32_t)
     * @return none
    */
    void write(uint32_t address, void *data, uint32_t size);
    
    /**
     * Write array of bytes (use the page mode)
     * @param address start address (uint32_t)
     * @param data bytes array to write (int8_t[])
     * @param size number of bytes to write (uint32_t)
     * @return none
    */
    void write(uint32_t address, int8_t data[], uint32_t size);
    
    /**
     * Wait eeprom ready
     * @param none
     * @return none
    */
    void ready(void);
    
    /**
     * Get eeprom size in bytes
     * @param none
     * @return size in bytes (uint32_t)
    */
    uint32_t getSize(void);
        
    /**
     * Get eeprom name
     * @param none
     * @return name (const char*)
    */
    const char* getName(void);
    
    /**
     * Clear eeprom (write with 0)
     * @param  none
     * @return none
    */
    void clear(void);
    
     /**
     * Get the current error number (EEPROM_NoError if no error)
     * @param  none
     * @return none
    */
    uint8_t getError(void);
    
    /**
     * Get current error message
     * @param  none
     * @return current error message(std::string)
    */
    std::string getErrorMessage(void)
    { 
      return(_ErrorMessageEEPROM[_errnum]);
    }
    
//---------- local variables ----------
private:
    I2C _i2c;              // Local i2c communication interface instance
    int _address;          // Local i2c address
    uint8_t _errnum;       // Error number
    TypeEeprom _type;      // EEPROM type
    uint8_t _page_write;   // Page write size
    uint8_t _page_number;  // Number of page
    uint32_t _size;        // Size in bytes
    bool checkAddress(uint32_t address); // Check address range
    static const char * const _name[]; // eeprom name
//-------------------------------------
};
#endif

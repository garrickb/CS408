#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "libusb.h"

unsigned char* padIClickerBaseCommand(char* unpadded, int length)
{
  if(length < 0 || length > 64)
  {
    return NULL;
  }
  // allocated memory for command
  unsigned char * ret = (unsigned char*)malloc(64*sizeof(char));
  int i;
  //copies original command over to the new command
  for(i=0; i<length; i++) {
    ret[i] = unpadded[i];
  }
  //pads it to 64 bytes
  for(i=i; i<64; i++) {
    ret[i] = 0x0;
  }
  return ret;
}

typedef struct
{
  // libusb session
  libusb_context *ctx;
  // handle for base
  libusb_device_handle *base;
  // frequency for iClickerBase
  char frequency[2];
} iClickerBase;

iClickerBase* getIClickerBase()
{
  iClickerBase* iBase = (iClickerBase*)malloc(sizeof(iClickerBase));
  iBase->ctx = NULL;
  iBase->base = NULL;
  int rc = libusb_init (&iBase->ctx);
  if (rc < 0)
  {
    return iBase;
  }
  // sets to recommended debug level
  libusb_set_debug(iBase->ctx, 3);

  int VENDOR_ID = 0x1881;
  int PRODUCT_ID = 0x0150;
  iBase->base = libusb_open_device_with_vid_pid(iBase->ctx, VENDOR_ID, PRODUCT_ID); //these are vendorID and productID I found for my usb device
  if(iBase->base != NULL)
  {
    //find out if kernel driver is attached
    if(libusb_kernel_driver_active(iBase->base, 0)) {
      //detach it
      libusb_detach_kernel_driver(iBase->base, 0);
    }
  }
  return iBase;
}

void sendIClickerBaseControlTransfer(iClickerBase *iBase, char* commandstring, int length)
{
  // pads the command string
  unsigned char* paddedcommand = padIClickerBaseCommand(commandstring, length);
  // sends the command to the base
  libusb_control_transfer(iBase->base,0x21,0x09,0x0200,0x0000,paddedcommand,64,1000);
  // waits 2ms for command to be processed. this is just an arbitrary time
  usleep(2000);
  free(paddedcommand);
}

void setIClickerBaseDisplay(iClickerBase *iBase, char* displayString, int length, int line)
{
  // check if the length is between 0 and 16
  if(length < 0 || length > 16) {
    return;
  }
  // check if line number is valid
  if(line < 0 || line > 1) {
    return;
  }
  char* command = (char*)malloc(18*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x13 + line;
  int i;
  for(i=0; i<length; i++)
  {
    command[2+i] = displayString[i];
  }
  for(i=i; i<16; i++)
  {
    command[2+i] = 0x20;
  }
  sendIClickerBaseControlTransfer(iBase, command, 18);
  free(command);
}

void setIClickerBaseFrequency(iClickerBase* iBase, char first, char second)
{
  if(first < 'a' || first > 'd' || second < 'a' || second > 'd')
  {
    return;
  }
  iBase->frequency[0] = first;
  iBase->frequency[1] = second;
  char* command = (char*)malloc(4*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x10;
  command[2] = 0x21 + first - 'a';
  command[3] = 0x41 + second - 'a';
  sendIClickerBaseControlTransfer(iBase, command, 4);
  command[1] = 0x16;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  free(command);
}

void setIClickerBaseVersion2(iClickerBase* iBase)
{
  char* command = (char*)malloc(2*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x2d;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  free(command);
}

void initIClickerBase(iClickerBase* iBase)
{
  setIClickerBaseFrequency(iBase, 'a', 'a');

  char* command = (char*)malloc(9*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x2a;
  command[2] = 0x21;
  command[3] = 0x41;
  command[3] = 0x05;
  sendIClickerBaseControlTransfer(iBase, command, 5);
  command[1] = 0x12;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  command[1] = 0x15;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  command[1] = 0x16;
  sendIClickerBaseControlTransfer(iBase, command, 2);

  setIClickerBaseVersion2(iBase);

  command[1] = 0x29;
  command[2] = 0xa1;
  command[3] = 0x8f;
  command[4] = 0x96;
  command[5] = 0x8d;
  command[6] = 0x99;
  command[7] = 0x97;
  command[8] = 0x8f;
  sendIClickerBaseControlTransfer(iBase, command, 9);
  command[1] = 0x17;
  command[2] = 0x04;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x17;
  command[2] = 0x03;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x16;
  sendIClickerBaseControlTransfer(iBase, command, 2);
}

void startIClickerBasePoll(iClickerBase* iBase)
{
  char* command = (char*)malloc(3*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x17;
  command[2] = 0x03;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x17;
  command[2] = 0x05;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x11;
  sendIClickerBaseControlTransfer(iBase, command, 2);
}

void stopIClickerBasePoll(iClickerBase* iBase)
{
  char* command = (char*)malloc(3*sizeof(char));
  command[0] = 0x01;
  command[1] = 0x12;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  command[1] = 0x16;
  sendIClickerBaseControlTransfer(iBase, command, 2);
  command[1] = 0x17;
  command[2] = 0x01;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x17;
  command[2] = 0x03;
  sendIClickerBaseControlTransfer(iBase, command, 3);
  command[1] = 0x17;
  command[2] = 0x04;
  sendIClickerBaseControlTransfer(iBase, command, 3);

}

void closeIClickerBase(iClickerBase* iBase)
{
  libusb_close(iBase->base); //close the device we opened
  libusb_exit(iBase->ctx); //needs to be called to end
}

int main()
{
  iClickerBase* iBase = getIClickerBase();

  initIClickerBase(iBase);
  setIClickerBaseFrequency(iBase, 'c', 'd');

  setIClickerBaseDisplay(iBase, "Randy is", 8, 0);
  setIClickerBaseDisplay(iBase, "  ", 2, 1);
  startIClickerBasePoll(iBase);
  //stopIClickerBasePoll(iBase);

  //closeIClickerBase(iBase);
  return 0;
}

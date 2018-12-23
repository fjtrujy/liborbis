/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev/liborbis
 */
#include <kernel.h>
#include <ps4link.h>
#include <debugnet.h>
#include <sys/fcntl.h>
#include <string.h>

#define HOST0 "host0:"
#define ORBISFILE_MAX_OPEN_DESCRIPTORS 10
int orbisFileRemoteOpenDescriptors[ORBISFILE_MAX_OPEN_DESCRIPTORS];
int orbisfile_initialized=0;

int orbisCheckSlotByFd(int fd)
{
	if(fd<0)
	{
		return -1;
	}
	int i = 0;
	int slot = -1;

	// search slot
	for(i=0;i<ORBISFILE_MAX_OPEN_DESCRIPTORS;i++) 
	{ 
		if(orbisFileRemoteOpenDescriptors[i]==fd) 
		{ 
			slot=i; 
			break; 	
		}
	}
	return slot;
}
int orbisCheckFreeRemoteDescriptor()
{
	int i=0;
	int slot=-1;
	// search slot
	for (i=0;i<ORBISFILE_MAX_OPEN_DESCRIPTORS;i++) 
	{ 
		if(orbisFileRemoteOpenDescriptors[i]==-1) 
		{ 
				slot = i; 
				break; 	
		}
	}
	return slot;
}
int orbisOpen(const char *file, int flags, int mode)
{
	int ret;
	int slot;
	if(!file)
	{
		return 0x80020016;
	}
	if (strncmp(file,HOST0,sizeof(HOST0)-1)==0) 
	{
		
		slot=orbisCheckFreeRemoteDescriptor();
		if(slot>=0)
		{
			ret=ps4LinkOpen(file,flags,mode);
			if(ret>0)
			{
				orbisFileRemoteOpenDescriptors[slot]=ret;
				debugNetPrintf(DEBUG,"[ORBISFILE] slot=%d fd=%d\n",slot,ret);
			}
			return ret;
		}
		debugNetPrintf(DEBUG,"[ORBISFILE] no free slot available\n");
		return 0x80020016;
	}
	else
	{
		return sceKernelOpen(file,flags,mode);
	}
}
int orbisClose(int fd)
{
	int ret;
	int slot=-1;
	if(!fd)
	{
		return 0x80020009;
	}
	slot=orbisCheckSlotByFd(fd);
	if(slot>=0)
	{
		
		ret=ps4LinkClose(fd);
		orbisFileRemoteOpenDescriptors[slot]=-1;
		return ret;
	}
	return sceKernelClose(fd);
}
int orbisRead(int fd, void *data, size_t size)
{
	int ret;
	int slot=-1;
	if(!fd)
	{
		return 0x80020009;
	}
	if(!data)
	{
		return 0x8002000e;
	}
	slot=orbisCheckSlotByFd(fd);
	if(slot>=0)
	{	
		ret=ps4LinkRead(fd,data,size);
		return ret;
	}
	return sceKernelRead(fd,data,size);
}
int orbisWrite(int fd, const void *data, size_t size)
{
	int ret;
	int slot=-1;
	if(!fd)
	{
		return 0x80020009;
	}
	if(!data)
	{
		return 0x8002000e;
	}
	slot=orbisCheckSlotByFd(fd);
	if(slot>=0)
	{
		ret=ps4LinkWrite(fd,data,size);
		return ret;
	}
	return sceKernelWrite(fd,data,size);
}
int orbisLseek(int fd, int offset, int whence)
{
	int ret;
	int slot=-1;
	if(!fd)
	{
		return 0x80020009;
	}
	slot=orbisCheckSlotByFd(fd);
	if(slot>=0)
	{
		ret=ps4LinkLseek(fd,offset,whence);
		return ret;
	}
	return sceKernelLseek(fd,offset,whence);
}
void orbisFileFinish()
{
	int i;
	for(i=0;i<ORBISFILE_MAX_OPEN_DESCRIPTORS;i++)
	{
		
		if(orbisFileRemoteOpenDescriptors[i]>0)
		{
			ps4LinkClose(orbisFileRemoteOpenDescriptors[i]);
		}
		orbisFileRemoteOpenDescriptors[i]=-1;
	}
	orbisfile_initialized=0;
}
int orbisFileInit()
{
	int i;
	if(orbisfile_initialized)
	{
		debugNetPrintf(DEBUG,"[ORBISFILE] liborbisFile already initialized\n");
		return orbisfile_initialized;
	}
	for(i=0;i<ORBISFILE_MAX_OPEN_DESCRIPTORS;i++)
	{
		orbisFileRemoteOpenDescriptors[i]=-1;
	}
	debugNetPrintf(DEBUG,"[ORBISFILE] liborbisFile initialized\n");
	
	orbisfile_initialized=1;
	return orbisfile_initialized;
}
	
	
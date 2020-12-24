#ifndef CRC32_H
#define CRC32_H
int Crc32_ComputeFile( FILE *file, u32 *outCrc32 );
u32 Crc32_ComputeBuf( u32 inCrc32, const void *buf, size_t bufLen );
#endif

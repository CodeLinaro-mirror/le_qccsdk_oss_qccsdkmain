/*========================================================================
*
* @file binary_desriptor.h
* @brief Contains the structure for the binary description
*========================================================================*/
#ifndef _BINARY_DESCRIPTOR_
#define _BINARY_DESCRIPTOR_
typedef struct binary_desriptor
{
	uint32_t version;
	uint32_t reserved[15];
}binary_desriptor_t;

#endif /* _BINARY_DESCRIPTOR_ */

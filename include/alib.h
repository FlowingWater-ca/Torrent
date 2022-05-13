/**
 * @file alib.h
 * @brief Functions that deal with the initialization of
 * the block structure
 */
#ifndef _ALIB_H
#define _ALIB_H

#include <cstddef>
#include "atype.h"

/**
 * @brief Packs uint64_t into uint8_t buffer little endian style
 */
uint32_t u64Packer(uint8_t *buf, uint64_t data);
uint32_t u64Unpack(uint8_t *buf, uint64_t *data);
size_t strlen(const uint8_t *ptr);

/**
 * @brief This function will print the relative information of a block
 * 
 * it will print the:
 *	time (when the block was created)
 *	key (Unique identifier)
 *	number of payloads
 */
void printBlock(block *target);

/**
 * @brief Create a new pack (magnet link info)
 * 
 * This fn will allocate a pack struct, and all of its parameters
 * @return True if success
 */
bool newPack(pack *px,
             uint8_t xt[MAGNET_XT_LEN],//!< exact topic (file hash)
             uint64_t xl,//!< Exact length (size in bytez)
             char *dn,   //!< Display name
             uint8_t *tr,  //!< tracker url
			 char *kt[MAGNET_KT_COUNT]);

bool newTran(tran *tx,
			 uint64_t time,
			 uint64_t id,
			 uint64_t amount,
			 uint64_t src,
			 uint64_t dest);

/**
 * @brief THIS IS NOT THREAD SAFE.
 */
bool newBlock(chain *ch);

bool insertBlock(chain *ch,
				 uint64_t n,
				 uint64_t time,
				 uint32_t n_packs,//package count
				 pack *packs,     //package array
				 uint32_t n_trans,//transaction count
				 tran *trans,     //transaction array
				 uint8_t crc[SHA512_LEN] = NULL,
				 uint8_t key[SHA512_LEN] = NULL);

bool trimBlock(chain *ch);

void deletePack(pack *target);
void deleteTran(tran *target);
void deleteChain(chain *target);

bool enqueuePack(pack *target);
bool enqueueTran(tran *target);
bool dequeuePack(pack *target);
bool dequeueTran(tran *target);

/**
 * @brief Generate a block checksum or validate an existing checksum
 */
bool checkBlock(block *bx, bool modify = false);

/**
 * @brief Validate content against internal checksums
 */
bool auditChain(chain *ch);

/**
 * @brief Compare two chains
 *
 * @return The index of the first disagreement
 */
uint64_t compareChain(chain *left, chain *right);

#endif //_ALIB_H

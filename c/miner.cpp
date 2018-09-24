/**
 * 	TrustNote Equihash PoW Miner
 *
 *
 *	Copyright @ TrustNote.org
 *
 * 	@history
 *	Created by Liu QiXing on 2018/8/22.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include "bitcoin-consensus-params.h"
#include "uint256.h"
#include "arith_uint256.h"
#include "equihash_avx2.h"
#include "crypto/blake2b.h"

#include "utility.h"
#include "miner.h"


#ifdef WIN32
	#ifndef strncasecmp
		#define strncasecmp strnicmp
	#endif
#endif



/**
 *	main
 *
 *	@return	{int}
 */
int main( void )
{
	uint8_t utInputHeader[ 140 ];
	arith_uint256 bn256DifficultyDefault	= UintToArith256( uint256S( TRUSTNOTE_MINER_POW_LIMIT ) );
	arith_uint256 bn256DifficultyMax	= UintToArith256( uint256S( TRUSTNOTE_MINER_POW_MAX ) );
	arith_uint256 bn256DifficultyMin	= UintToArith256( uint256S( TRUSTNOTE_MINER_POW_MIN ) );

	uint32_t uDifficulty		= bn256DifficultyDefault.GetCompact();
	uint32_t uNonceStart		= 0;
	uint32_t uCalcTimes		= 30000;
	uint32_t uNonce;
	char szHexHash[ 64 ];
	int nCheckPoW;

	#ifdef _DEBUG
		printf( "))) : difficulty def : %u\n", bn256DifficultyDefault.GetCompact() );
		printf( "))) : difficulty max : %u\n", bn256DifficultyMax.GetCompact() );
		printf( "))) : difficulty min : %u\n", bn256DifficultyMin.GetCompact() );
	#endif

	//	...
	uint32_t uNewDifficulty1	= calculateNextDifficulty( 528482303, 3194, 2400 );
	uNewDifficulty1			= calculateNextDifficulty( 528482303, 2294, 2400 );
	uNewDifficulty1			= calculateNextDifficulty( 528482303, 1194, 2400 );
	uint32_t uNewDifficulty2	= calculateNextDifficulty( uDifficulty, 30000, 15000 );
	uint32_t uNewDifficulty3	= calculateNextDifficulty( uDifficulty, 10000, 15000 );
	uint32_t uNewDifficulty4	= calculateNextDifficulty( uDifficulty, 40000, 15000 );
	uint32_t uNewDifficulty5	= calculateNextDifficulty( uDifficulty, 50000, 15000 );
	uint32_t uNewDifficulty6	= calculateNextDifficulty( uDifficulty, 60000, 15000 );
	uint32_t uNewDifficulty7	= calculateNextDifficulty( uDifficulty, 70000, 15000 );
	uint32_t uNewDifficulty8	= calculateNextDifficulty( uDifficulty, 80000, 15000 );
	uint32_t uNewDifficulty9	= calculateNextDifficulty( uDifficulty, 90000, 15000 );

	#ifdef _DEBUG
		printf( "### : will start mining with difficulty : %u\n", uDifficulty );
	#endif

	//	...
	memset( utInputHeader, 0, sizeof( utInputHeader ) );
	for ( int i = 0; i < 140; i ++ )
	{
		utInputHeader[ i ] = i;
	}
	startMining( utInputHeader, uDifficulty, uNonceStart, uCalcTimes, &uNonce, szHexHash, sizeof( szHexHash ) );
	#ifdef _DEBUG
		printf( "### : %u\t : %.*s\n", uNonce, 64, szHexHash );
	#endif

	//	...
	nCheckPoW = checkProofOfWork( utInputHeader, uDifficulty, uNonce, szHexHash );
	#ifdef _DEBUG
		printf( "checkProofOfWork return %d : %s\n\n\n\n",
			nCheckPoW,
			0 == nCheckPoW ? "Winer Winner Chicken Dinner!" : "Not Okay!" );
	#endif

	//	...
	return 0;
}




/**
 *	start mining
 *
 *	@param	{uint8_t*}	pcutInputHeader		140 bytes header
 *	@param	{uint32_t}	uDifficulty
 *	@param	{uint32_t}	uNonceStart
 *	@param	{uint32_t}	uCalcTimes
 *	@param	{uint32_t *}	puNonce			OUT 4 bytes
 *	@param	{char *}	pszHashHex		OUT 64 bytes
*	@param	{uint32_t}	uHashHexLength
 *	@return	{uint32_t}
 */
int startMining(
	const uint8_t * pcutInputHeader,
	const uint32_t uDifficulty,
	const uint32_t uNonceStart,
	const uint32_t uCalcTimes,
	OUT uint32_t * puNonce,
	OUT char * pszHashHex,
	uint32_t uHashHexLength )
{
	int nRet;
	uint8_t utOutContext[ 32 ];
	char szHexHash[ 128 ];

	if ( NULL == pcutInputHeader )
	{
		return -1000;
	}
	if ( NULL == puNonce )
	{
		return -1002;
	}
	if ( NULL == pszHashHex )
	{
		return -1003;
	}
	if ( 64 != uHashHexLength )
	{
		return -1004;
	}

	//	...
	nRet = -1;

	//	...
	void * pvContextAlloc	= malloc( TRUSTNOTE_MINER_CONTEXT_SIZE + 4096 );
	void * pvContext	= (void*)( ( (long)pvContextAlloc + 4095 ) & -4096 );
	void * pvContextEnd	= (char*)pvContext + TRUSTNOTE_MINER_CONTEXT_SIZE;

	//	...
	uint32_t uNonce		= uNonceStart;
	uint32_t nNonceEnd	= uCalcTimes > 0
					? ( uNonceStart + uCalcTimes ) >= UINT32_MAX
						? ( UINT32_MAX - uNonceStart - 1 )
						: ( uNonceStart + uCalcTimes )
					: ( UINT32_MAX - uNonceStart - 1 );
	for ( ; uNonce < nNonceEnd; uNonce ++ )
	{
		//
		//	calculate ...
		//
		EhPrepare( pvContext, (void *)pcutInputHeader );
		int32_t nSolutionCount = std::min( 3, EhSolver( pvContext, uNonce ) );

		for ( int n = 0; n < nSolutionCount; n ++ )
		{
			//
			//	blake2b	512/256
			//	blake2s	256/128
			//
			blake2b( (uint8_t *)utOutContext, (uint8_t *)pvContext + n * 1344, NULL, sizeof(utOutContext), 1344, 0 );
			bytesToHexString( utOutContext, 32, szHexHash );

			#ifdef _DEBUG
				printf( "Nonce: %u\t : %.*s\n", uNonce, 64, szHexHash );
			#endif

			//	filter
			int nCheck = filterDifficulty( uDifficulty, szHexHash );
			if ( 0 == nCheck )
			{
				#ifdef _DEBUG
					printf( ">>> Done!\n" );
				#endif

				//	printf( "[%d] - nonce: %d\t buff: %.*s\n", nCheck, uNonce, 64, szHexHash );
				nRet = 0;
				*puNonce = uNonce;
				memcpy( pszHashHex, szHexHash, std::min( (uint32_t)64, uHashHexLength ) );

				//	...
				break;
			}
		}

		#ifdef _DEBUG
			printf( "\n" );
		#endif

		if ( 0 == nRet )
		{
			//	done
			break;
		}
	}

	//	...
	free( pvContextAlloc );
	pvContextAlloc = NULL;

	//	...
	return nRet;
}


/**
 *	stop mining
 *	@return	{int}
 */
int stopMining()
{
	return 0;
}


/**
 *	check proof of work
 *
 *	@param	{uint8_t *}	pcutInputHeader
 *	@param	{uint32_t}	uDifficulty
 *	@param	{uint32_t}	uNonce
 *	@param	{const char *}	pcszHash		with length 64
 *	@return	{int}
 *		0	- okay
 */
int checkProofOfWork(
	const uint8_t * pcutInputHeader,
	const uint32_t uDifficulty,
	const uint32_t uNonce,
	const char * pcszHashHex )
{
	int nRet;

	if ( NULL == pcutInputHeader )
	{
		return -1000;
	}
	if ( NULL == pcszHashHex )
	{
		return -1001;
	}
	if ( strlen( pcszHashHex ) < 64 )
	{
		#ifdef _DEBUG
			printf( "checkProofOfWork :: pcszHashHex : %s, length : %zu\n", pcszHashHex, strlen( pcszHashHex ) );
		#endif
		return -1002;
	}

	uint8_t utOutContext[ 32 ];
	char szHashHex[ 128 ];

	//	...
	nRet = -1;

	//	...
	void * pvContextAlloc	= malloc( TRUSTNOTE_MINER_CONTEXT_SIZE + 4096 );
	void * pvContext	= (void*)( ( (long)pvContextAlloc + 4095 ) & -4096 );
	void * pvContextEnd	= (char*)pvContext + TRUSTNOTE_MINER_CONTEXT_SIZE;

	//
	//	calculate ...
	//
	EhPrepare( pvContext, (void *)pcutInputHeader );
	int32_t nSolutionCount = EhSolver( pvContext, uNonce );

	for ( int n = 0; n < nSolutionCount; n ++ )
	{
		//
		//	blake2b	512/256
		//	blake2s	256/128
		//
		blake2b( (uint8_t *)utOutContext, (uint8_t *)pvContext + n * 1344, NULL, sizeof( utOutContext ), 1344, 0 );
		bytesToHexString( utOutContext, 32, szHashHex );

		if ( 0 == strncasecmp( szHashHex, pcszHashHex, 64 ) )
		{
			//
			//	hex value matched
			//
			int nCheck = filterDifficulty( uDifficulty, szHashHex );
			if ( 0 == nCheck )
			{
				//
				//	difficulty filtered as okay
				//
				nRet = 0;
				break;
			}
		}
	}

	//	...
	free( pvContextAlloc );
	pvContextAlloc = NULL;

	return nRet;
}


/**
 *	convert 256 bits string to uint32_t
 *
 *	@param 	{const char *}	pcszDifficultyHex	"00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
 *	@return	{uint32_t}
 */
uint32_t difficulty256HexToUInt32( const char * pcszDifficultyHex )
{
	if ( NULL == pcszDifficultyHex )
	{
		return 0;
	}
	if ( strlen( pcszDifficultyHex ) < 64 )
	{
		return 0;
	}

	char szDifficultyHex[ 65 ];
	uint256 un256Difficulty;
	arith_uint256 bn256Difficulty;

	memset( szDifficultyHex, 0, sizeof(szDifficultyHex) );
	memcpy( szDifficultyHex, pcszDifficultyHex, 64 );

	un256Difficulty	= uint256S( szDifficultyHex );
	bn256Difficulty	= UintToArith256( un256Difficulty );

	//	...
	return bn256Difficulty.GetCompact();
}


/**
 *	filter difficulty
 *	check proof of work
 *
 *	@param	{uint32_t}	uDifficulty
 *	@param	{const char *}	pcszHashHex
 *	@return	{uint32_t}
 */
int filterDifficulty(
	const uint32_t uDifficulty,
	const char * pcszHashHex )
{
	bool fNegative;
	bool fOverflow;
	arith_uint256 bnTarget;
	char szHashHexCalc[ 128 ];

	if ( NULL == pcszHashHex )
	{
		return -1000;
	}
	if ( strlen( pcszHashHex ) < 64 )
	{
		#ifdef _DEBUG
			printf( "filterDifficulty :: pcszHashHex : %s, length : %zu\n", pcszHashHex, strlen( pcszHashHex ) );
		#endif
		return -1002;
	}

	//	...
	memcpy( szHashHexCalc, pcszHashHex, 64 );
	szHashHexCalc[ 64 ]	= 0;

	//	...
	bnTarget.SetCompact( uDifficulty, & fNegative, & fOverflow );

	//	check range
	if ( fNegative || 0 == bnTarget || fOverflow || bnTarget > UintToArith256( uint256S( TRUSTNOTE_MINER_POW_LIMIT ) ) )
	{
		//	printf("nBits below minimum work.\n");
		return -10;
	}

	//	check proof of work matches claimed amount
	if ( UintToArith256( uint256S( szHashHexCalc ) ) > bnTarget )
	{
		//	printf("hash doesn't match nBits.\n");
		return -20;
	}

	return 0;
}


/**
 *	calculate next difficulty
 *
 *	@param	{uint32_t}	uPreviousAverageDifficulty
 *	@param	{uint32_t}	uTimeUsed
 *	@param	{uint32_t}	uTimeStandard
 *	@return	{uint32_t}
 */
uint32_t calculateNextDifficulty(
	const uint32_t uPreviousAverageDifficulty,
	const uint32_t uTimeUsed,
	const uint32_t uTimeStandard )
{
	uint32_t uRet = 0;

	//	...
	const arith_uint256 bnPowMaxUInt256	= UintToArith256( uint256S( TRUSTNOTE_MINER_POW_MAX ) );
	const arith_uint256 bnPowLimitUInt256	= UintToArith256( uint256S( TRUSTNOTE_MINER_POW_LIMIT ) );

	//	3/4 uTimeStandard + 1/4 uTimeUsed
	int64_t n64ActualTimeSpan		= ( uTimeStandard * 3 + uTimeUsed ) / 4;
	int64_t n64MinActualTimeSpan		= ( uTimeStandard * ( 100 - 16 ) ) / 100;
	int64_t n64MaxActualTimeSpan		= ( uTimeStandard * ( 100 + 32 ) ) / 100;

	//	...
	//assert( bnPowMaxUInt256 / bnPowLimitUInt256 >= consensus.nPowAveragingWindow );

	#ifdef _DEBUG
		printf( "### uPreviousAverageDifficulty\t: %u\n", uPreviousAverageDifficulty );
		printf( "### uTimeUsed\t: %u\n", uTimeUsed );
		printf( "### uTimeStandard\t: %u\n", uTimeStandard );

		printf( "### bnPowMaxUInt256\t: %u\n", bnPowMaxUInt256.GetCompact() );
		printf( "### bnPowLimitUInt256\t: %u\n", bnPowLimitUInt256.GetCompact() );

		printf( "### n64ActualTimeSpan\t: %u\n", n64ActualTimeSpan );
		printf( "### n64MinActualTimeSpan\t: %u\n", n64MinActualTimeSpan );
		printf( "### n64MaxActualTimeSpan\t: %u\n", n64MaxActualTimeSpan );
	#endif

	if ( n64ActualTimeSpan < n64MinActualTimeSpan )
	{
		//	84% adjustment up
		#ifdef _DEBUG
			printf( "*** 84%% adjustment up, span less then min: %lld, reset as min\n", n64MinActualTimeSpan );
		#endif
		n64ActualTimeSpan = n64MinActualTimeSpan;
	}
	if ( n64ActualTimeSpan > n64MaxActualTimeSpan )
	{
		// 	132% adjustment down
		#ifdef _DEBUG
			printf( "*** 132%% adjustment down, span greater than max: %lld, reset as max\n", n64MaxActualTimeSpan );
		#endif
		n64ActualTimeSpan = n64MaxActualTimeSpan;
	}

	//
	//	retarget
	//
	//printf("bnPowLimit: %d\n", bnPowLimit.GetCompact());

	//	...
	arith_uint256 bnNewUInt256;
	bnNewUInt256.SetCompact( uPreviousAverageDifficulty );

	#ifdef _DEBUG
		printf( "*** before\t\t: %u\n", bnNewUInt256.GetCompact() );
	#endif

	//	...
	bnNewUInt256	/= uTimeStandard;
	bnNewUInt256	*= n64ActualTimeSpan;

	#ifdef _DEBUG
		printf( "*** after\t\t: %u [ ( %u / %u ) * %u ]\n", bnNewUInt256.GetCompact(),
			uPreviousAverageDifficulty, uTimeStandard, n64ActualTimeSpan );
	#endif

	//
	//	set as the limited lowest difficulty
	//
	if ( bnNewUInt256 > bnPowLimitUInt256 )
	{
		#ifdef _DEBUG
			printf( "*** bnNewUInt256(%u) > bnPowLimitUInt256(%u)\n", bnNewUInt256.GetCompact(), bnPowLimitUInt256.GetCompact() );
		#endif
		bnNewUInt256 = bnPowLimitUInt256;
	}

	//	...
	uRet = bnNewUInt256.GetCompact();
	#ifdef _DEBUG
		printf( "calculateNextDifficulty :: uPreviousAverageDifficulty : %u, uTimeUsed : %u, uTimeStandard : %u, return : %u\n",
			uPreviousAverageDifficulty, uTimeUsed, uTimeStandard, uRet );
		printf( "\n\n\n" );
	#endif

	return uRet;
}


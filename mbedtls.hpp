#pragma once


// #include <mbedtls/ecp.h>
// #include <mbedtls/ecdh.h>
// #include <mbedtls/entropy.h>
// #include <mbedtls/ctr_drbg.h>

// static mbedtls_ctr_drbg_context drbg;

// int init_mbedtls()
// {
//     mbedtls_entropy_context entropy;
	
// 	mbedtls_entropy_init(&entropy);
// 	mbedtls_ctr_drbg_init(&drbg);
// 	int ret = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, NULL, 0);
// 	if (ret) {
// 		//std::cerr << "mbedtls_ctr_drbg_seed fail:" << ret << "\n";
// 		return -1;
// 	}
//     return 0;
// }



# Files and Subdirectories in `sw/device/lib/crypto`

The `sw/device/lib/crypto` directory contains OpenTitan's cryptography library implementation. Here's a comprehensive listing organized by subdirectory:

## Directory Structure Overview

The crypto library is organized into four main subdirectories:
- **data/** - Test plans and documentation
- **drivers/** - Hardware driver implementations for crypto accelerators
- **impl/** - Core cryptographic algorithm implementations
- **include/** - Public API header files

---

## 1. `data/` Subdirectory

### `crypto_testplan.hjson`
Test plan configuration file that defines test points for the crypto library, including stress tests for AES, ECC, hash functions, HMAC, KMAC, RSA, SHAKE, SPHINCS+, and entropy generation. 

---

## 2. `drivers/` Subdirectory

This directory contains low-level hardware drivers for OpenTitan's cryptographic accelerators:

### AES Hardware Driver
- **`aes.h`** / **`aes.c`** - Driver for the AES hardware accelerator, supporting ECB, CBC, CFB, OFB, and CTR modes with both software-provided and sideloaded keys.
- **`aes_test.c`** - Test file for the AES driver

### Entropy and Random Number Generation
- **`entropy.h`** / **`entropy.c`** - Driver for the entropy complex (CSRNG), providing access to hardware random number generation. The seed size is 384 bits (256-bit key + 128-bit block for CTR-DRBG with AES-256).  
- **`entropy_kat.h`** / **`entropy_kat.c`** - Known answer tests for entropy functionality
- **`entropy_test.c`** - Test file for entropy driver
- **`mock_entropy.cc`** - Mock implementation for testing entropy functionality

### HMAC Hardware Driver
- **`hmac.h`** / **`hmac.c`** - Driver for the HMAC hardware accelerator

### Key Manager
- **`keymgr.h`** / **`keymgr.c`** - Driver for the key manager hardware, which derives keys from the root of trust. Supports hardware-backed key generation with diversification using salt and version parameters. 
- **`keymgr_test.c`** - Test file for key manager driver

### KMAC Hardware Driver
- **`kmac.h`** / **`kmac.c`** - Driver for the KMAC (Keccak Message Authentication Code) hardware accelerator

### OTBN (OpenTitan Big Number) Accelerator
- **`otbn.h`** / **`otbn.c`** - Driver for OTBN, a specialized coprocessor for big number operations used in public-key cryptography (RSA, ECC). Handles 256-bit wide words. 

### RISC-V Core Interface
- **`rv_core_ibex.h`** / **`rv_core_ibex.c`** - Interface to RISC-V core specific features
- **`rv_core_ibex_test.c`** - Test file for RISC-V core interface
- **`mock_rv_core_ibex.cc`** - Mock implementation for testing

---

## 3. `impl/` Subdirectory

This directory contains the core cryptographic algorithm implementations:

### Main Implementation Files

#### AES Implementations
- **`aes.c`** - AES block cipher implementation
- **`aes_gcm.c`** - AES-GCM (Galois/Counter Mode) authenticated encryption implementation

#### Key Derivation Functions
- **`drbg.c`** - Deterministic Random Bit Generator implementation 
- **`hkdf.c`** - HMAC-based Key Derivation Function (HKDF) implementation
- **`kdf_ctr.c`** - Counter-mode Key Derivation Function implementation
- **`kmac_kdf.c`** - KMAC-based Key Derivation Function implementation

#### Message Authentication and Hashing
- **`hmac.c`** - HMAC (Hash-based Message Authentication Code) implementation
- **`kmac.c`** - KMAC implementation
- **`sha2.c`** - SHA-2 family hash functions (SHA-256, SHA-384, SHA-512) implementation
- **`sha3.c`** - SHA-3 family hash functions implementation

#### Elliptic Curve Cryptography
- **`ecc_curve25519.c`** - Curve25519 elliptic curve operations implementation
- **`ecc_p256.c`** - P-256 (NIST secp256r1) elliptic curve operations implementation
- **`ecc_p384.c`** - P-384 (NIST secp384r1) elliptic curve operations implementation
- **`x25519.c`** - X25519 key exchange implementation

#### RSA
- **`rsa.c`** - RSA public-key cryptography implementation

#### Key Management
- **`key_transport.c`** - Key import/export functionality for translating to/from the crypto library's key representations 
- **`keyblob.h`** / **`keyblob.c`** - Key blob management for handling blinded keys with two shares. Provides utilities for constructing and deconstructing key blobs. 
- **`keyblob_unittest.cc`** - Unit tests for keyblob functionality
- **`key_transport_unittest.cc`** - Unit tests for key transport

#### Integrity and Status
- **`integrity.h`** / **`integrity.c`** - Key integrity checking through checksums for both blinded and unblinded keys
- **`status.h`** - Status code definitions and error handling macros for the crypto library 
- **`status_unittest.cc`** - Unit tests for status handling
- **`status_debug_unittest.cc`** - Debug-specific status unit tests
- **`status_functest.c`** - Functional tests for status handling

#### Security Configuration
- **`security_config.c`** - Security configuration management

### Subdirectories under `impl/`

#### `impl/aes_gcm/`
- **`aes_gcm.h`** / **`aes_gcm.c`** - Internal AES-GCM implementation details
- **`ghash.h`** / **`ghash.c`** - GHASH algorithm for GCM mode, implementing Galois field multiplication with precomputed tables for blinded operations
- **`ghash_unittest.cc`** - Unit tests for GHASH

#### `impl/aes_kwp/`
- **`aes_kwp.h`** / **`aes_kwp.c`** - AES Key Wrap with Padding (AES-KWP) implementation for secure key transport

#### `impl/ecc/`
- **`curve25519.h`** / **`curve25519.c`** - Low-level Curve25519 operations
- **`p256.h`** / **`p256.c`** - Low-level P-256 curve operations
- **`p384.h`** / **`p384.c`** - Low-level P-384 curve operations

#### `impl/rsa/`
- **`rsa_datatypes.h`** - Data type definitions for RSA operations (2048, 3072, 4096-bit integers) 
- **`rsa_encryption.h`** / **`rsa_encryption.c`** - RSA encryption and decryption operations
- **`rsa_signature.h`** / **`rsa_signature.c`** - RSA signature generation and verification
- **`rsa_padding.h`** / **`rsa_padding.c`** - RSA padding schemes (PKCS#1 v1.5 and PSS)
- **`run_rsa.h`** / **`run_rsa.c`** - RSA operation execution on OTBN
- **`run_rsa_key_from_cofactor.h`** / **`run_rsa_key_from_cofactor.c`** - RSA key generation from cofactor

#### `impl/sha2/`
- **`sha256.h`** / **`sha256.c`** - SHA-256 specific implementation
- **`sha512.h`** / **`sha512.c`** - SHA-512 and SHA-384 implementation

---

## 4. `include/` Subdirectory

This directory contains the public API header files for the crypto library:

### Main API Headers

- **`otcrypto.h`** - Unified header file that includes the full crypto library API. This is the main entry point for users of the library. 

- **`datatypes.h`** - Shared data types including status codes, byte buffer representations, and key representations used across all crypto algorithms 

### Algorithm-Specific Headers

#### Symmetric Cryptography
- **`aes.h`** - AES block cipher API with support for ECB, CBC, CFB, OFB, and CTR modes 
- **`aes_gcm.h`** - AES-GCM authenticated encryption API

#### Hashing and Message Authentication
- **`sha2.h`** - SHA-2 hash functions API (SHA-256, SHA-384, SHA-512) 
- **`sha3.h`** - SHA-3 hash functions API
- **`hmac.h`** - HMAC message authentication code API
- **`kmac.h`** - KMAC message authentication code API supporting KMAC-128 and KMAC-256  

#### Key Derivation
- **`hkdf.h`** - HKDF (HMAC-based Key Derivation Function) API
- **`kdf_ctr.h`** - Counter-mode KDF API
- **`kmac_kdf.h`** - KMAC-based KDF API
- **`drbg.h`** - Deterministic Random Bit Generator API for instantiating, reseeding, and generating random data

#### Asymmetric Cryptography
- **`rsa.h`** - RSA signature and encryption API supporting 2048, 3072, and 4096-bit keys with PKCS#1 and PSS padding 
- **`ecc_p256.h`** - P-256 elliptic curve operations API (ECDSA and ECDH) 
- **`ecc_p384.h`** - P-384 elliptic curve operations API (ECDSA and ECDH)
- **`ecc_curve25519.h`** - Curve25519 operations API (Ed25519 signatures)
- **`x25519.h`** - X25519 key exchange API

#### Key Management
- **`key_transport.h`** - Key import/export API for symmetric key generation and key format conversion
- **`security_config.h`** - Security configuration API

### Subdirectory under `include/`

#### `include/freestanding/`
These headers provide basic types that can be used in freestanding environments (without full standard library):

- **`hardened.h`** - Hardened boolean type with high Hamming distance between true (0x739) and false (0x1d4) values for fault attack resistance 
- **`absl_status.h`** - Abseil-compatible status codes
- **`defs.h`** - Basic definitions for freestanding environments

---

## Notes

The OpenTitan crypto library is designed with security as a primary concern, featuring:
- **Hardened values** with high Hamming distances to resist fault injection attacks
- **Blinded key operations** using two-share masking to protect against side-channel attacks
- **Hardware acceleration** for performance-critical operations through dedicated crypto accelerators (AES, HMAC, KMAC, OTBN)
- **Comprehensive testing** with unit tests, functional tests, and Known Answer Tests (KATs) against NIST test vectors

The library follows a layered architecture where the `drivers/` directory interfaces with hardware, `impl/` provides the algorithm logic, and `include/` exposes the public API to applications.
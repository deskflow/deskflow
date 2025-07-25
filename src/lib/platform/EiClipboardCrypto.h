/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

//! Encryption algorithm types
enum class EncryptionAlgorithm
{
  None,             // No encryption
  AES256_GCM,       // AES-256 with GCM mode (recommended)
  AES256_CBC,       // AES-256 with CBC mode
  ChaCha20_Poly1305 // ChaCha20-Poly1305 (alternative)
};

//! Encryption key derivation methods
enum class KeyDerivation
{
  PBKDF2,   // PBKDF2 with SHA-256
  Argon2id, // Argon2id (memory-hard)
  HKDF      // HKDF with SHA-256
};

//! Encrypted data container
struct EncryptedData
{
  EncryptionAlgorithm algorithm;
  std::vector<uint8_t> ciphertext;
  std::vector<uint8_t> nonce; // IV/nonce for encryption
  std::vector<uint8_t> tag;   // Authentication tag (for AEAD)
  std::vector<uint8_t> salt;  // Salt for key derivation
  std::string keyId;          // Key identifier
  std::chrono::system_clock::time_point timestamp;

  EncryptedData() : algorithm(EncryptionAlgorithm::None)
  {
  }
};

//! Encryption configuration
struct CryptoConfig
{
  EncryptionAlgorithm algorithm = EncryptionAlgorithm::AES256_GCM;
  KeyDerivation keyDerivation = KeyDerivation::PBKDF2;
  size_t keySize = 32;                          // Key size in bytes (256 bits)
  size_t nonceSize = 12;                        // Nonce size in bytes (96 bits for GCM)
  size_t tagSize = 16;                          // Authentication tag size (128 bits)
  size_t saltSize = 16;                         // Salt size for key derivation
  uint32_t iterations = 100000;                 // PBKDF2 iterations
  bool encryptSensitiveOnly = true;             // Only encrypt sensitive data
  bool compressBeforeEncrypt = true;            // Compress data before encryption
  std::chrono::minutes keyRotationInterval{60}; // Key rotation interval
};

//! Clipboard encryption and decryption service
/*!
This class provides encryption and decryption services for clipboard data,
with special focus on protecting sensitive information during network transfer.
It supports multiple encryption algorithms and key derivation methods.
*/
class EiClipboardCrypto
{
public:
  explicit EiClipboardCrypto(const CryptoConfig &config = CryptoConfig{});
  ~EiClipboardCrypto();

  //! Set master password for key derivation
  bool setMasterPassword(const std::string &password);

  //! Generate a new encryption key
  std::string generateKey();

  //! Set encryption key directly
  bool setEncryptionKey(const std::string &keyId, const std::vector<uint8_t> &key);

  //! Encrypt clipboard data
  EncryptedData encrypt(const std::string &data, bool forceSensitive = false);

  //! Decrypt clipboard data
  std::string decrypt(const EncryptedData &encryptedData);

  //! Check if data should be encrypted based on sensitivity
  bool shouldEncrypt(const std::string &data) const;

  //! Compress data before encryption
  std::vector<uint8_t> compress(const std::string &data) const;

  //! Decompress data after decryption
  std::string decompress(const std::vector<uint8_t> &compressedData) const;

  //! Serialize encrypted data to string (for network transfer)
  std::string serialize(const EncryptedData &data) const;

  //! Deserialize encrypted data from string
  EncryptedData deserialize(const std::string &serialized) const;

  //! Get current configuration
  const CryptoConfig &getConfig() const;

  //! Update configuration
  void setConfig(const CryptoConfig &config);

  //! Check if encryption is available
  bool isAvailable() const;

  //! Get encryption statistics
  struct CryptoStats
  {
    size_t totalEncryptions = 0;
    size_t totalDecryptions = 0;
    size_t encryptionFailures = 0;
    size_t decryptionFailures = 0;
    std::chrono::microseconds averageEncryptionTime{0};
    std::chrono::microseconds averageDecryptionTime{0};
    size_t totalBytesEncrypted = 0;
    size_t totalBytesDecrypted = 0;
    double compressionRatio = 0.0;
  };

  CryptoStats getStatistics() const;

  //! Clear all encryption keys and statistics
  void clear();

  //! Rotate encryption keys
  void rotateKeys();

  //! Validate encrypted data integrity
  bool validateIntegrity(const EncryptedData &data) const;

private:
  //! Derive key from password using configured method
  std::vector<uint8_t> deriveKey(const std::string &password, const std::vector<uint8_t> &salt) const;

  //! Generate random bytes
  std::vector<uint8_t> generateRandomBytes(size_t size) const;

  //! AES-256-GCM encryption
  EncryptedData encryptAES256GCM(const std::vector<uint8_t> &plaintext) const;

  //! AES-256-GCM decryption
  std::vector<uint8_t> decryptAES256GCM(const EncryptedData &data) const;

  //! AES-256-CBC encryption
  EncryptedData encryptAES256CBC(const std::vector<uint8_t> &plaintext) const;

  //! AES-256-CBC decryption
  std::vector<uint8_t> decryptAES256CBC(const EncryptedData &data) const;

  //! ChaCha20-Poly1305 encryption
  EncryptedData encryptChaCha20Poly1305(const std::vector<uint8_t> &plaintext) const;

  //! ChaCha20-Poly1305 decryption
  std::vector<uint8_t> decryptChaCha20Poly1305(const EncryptedData &data) const;

  //! Update statistics
  void updateEncryptionStats(std::chrono::microseconds duration, size_t bytes, bool success);
  void updateDecryptionStats(std::chrono::microseconds duration, size_t bytes, bool success);

  //! Check if sensitive data patterns are present
  bool containsSensitivePatterns(const std::string &data) const;

  CryptoConfig m_config;
  std::string m_masterPassword;
  std::map<std::string, std::vector<uint8_t>> m_keys;
  std::string m_currentKeyId;
  std::chrono::system_clock::time_point m_lastKeyRotation;

  // Thread safety
  mutable std::mutex m_keyMutex;

  // Statistics
  mutable std::mutex m_statsMutex;
  CryptoStats m_stats;

  // Crypto state
  bool m_initialized;
  mutable std::mutex m_cryptoMutex;
};

//! RAII helper for automatic encryption/decryption
class ClipboardCryptoScope
{
public:
  ClipboardCryptoScope(std::shared_ptr<EiClipboardCrypto> crypto, const std::string &data);
  ~ClipboardCryptoScope();

  //! Get encrypted data for network transfer
  std::string getEncryptedData() const;

  //! Check if encryption was successful
  bool isEncrypted() const;

  //! Get original data size
  size_t getOriginalSize() const;

  //! Get encrypted data size
  size_t getEncryptedSize() const;

private:
  std::shared_ptr<EiClipboardCrypto> m_crypto;
  std::string m_originalData;
  EncryptedData m_encryptedData;
  bool m_encrypted;
};

} // namespace deskflow

/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardCrypto.h"
#include "base/Log.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

// Note: In a production environment, you would use a proper crypto library
// like OpenSSL, libsodium, or similar. This is a simplified implementation
// for demonstration purposes.

namespace deskflow {

EiClipboardCrypto::EiClipboardCrypto(const CryptoConfig &config)
    : m_config(config),
      m_initialized(false),
      m_lastKeyRotation(std::chrono::system_clock::now())
{
  // Initialize with a default key for demonstration
  m_currentKeyId = generateKey();
  m_initialized = true;

  LOG_DEBUG("clipboard crypto initialized with algorithm %d", static_cast<int>(m_config.algorithm));
}

EiClipboardCrypto::~EiClipboardCrypto()
{
  clear();
}

bool EiClipboardCrypto::setMasterPassword(const std::string &password)
{
  if (password.empty()) {
    LOG_WARN("empty master password provided");
    return false;
  }

  std::lock_guard<std::mutex> lock(m_keyMutex);
  m_masterPassword = password;

  // Generate new key from password
  m_currentKeyId = generateKey();

  LOG_DEBUG("master password set, new key generated");
  return true;
}

std::string EiClipboardCrypto::generateKey()
{
  std::lock_guard<std::mutex> lock(m_keyMutex);

  // Generate a random key ID
  auto keyBytes = generateRandomBytes(16);
  std::stringstream ss;
  for (auto byte : keyBytes) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  std::string keyId = ss.str();

  // Generate the actual encryption key
  std::vector<uint8_t> salt = generateRandomBytes(m_config.saltSize);
  std::vector<uint8_t> key;

  if (!m_masterPassword.empty()) {
    key = deriveKey(m_masterPassword, salt);
  } else {
    // Generate random key if no password is set
    key = generateRandomBytes(m_config.keySize);
  }

  m_keys[keyId] = key;
  return keyId;
}

bool EiClipboardCrypto::setEncryptionKey(const std::string &keyId, const std::vector<uint8_t> &key)
{
  if (keyId.empty() || key.size() != m_config.keySize) {
    LOG_WARN("invalid key ID or key size");
    return false;
  }

  std::lock_guard<std::mutex> lock(m_keyMutex);
  m_keys[keyId] = key;
  m_currentKeyId = keyId;

  LOG_DEBUG("encryption key set: %s", keyId.c_str());
  return true;
}

EncryptedData EiClipboardCrypto::encrypt(const std::string &data, bool forceSensitive)
{
  auto startTime = std::chrono::steady_clock::now();

  if (!m_initialized || m_config.algorithm == EncryptionAlgorithm::None) {
    EncryptedData result;
    result.algorithm = EncryptionAlgorithm::None;
    result.ciphertext = std::vector<uint8_t>(data.begin(), data.end());
    return result;
  }

  // Check if we should encrypt this data
  if (!forceSensitive && m_config.encryptSensitiveOnly && !shouldEncrypt(data)) {
    EncryptedData result;
    result.algorithm = EncryptionAlgorithm::None;
    result.ciphertext = std::vector<uint8_t>(data.begin(), data.end());
    return result;
  }

  std::vector<uint8_t> plaintext;

  // Compress if enabled
  if (m_config.compressBeforeEncrypt) {
    plaintext = compress(data);
  } else {
    plaintext = std::vector<uint8_t>(data.begin(), data.end());
  }

  EncryptedData result;
  result.timestamp = std::chrono::system_clock::now();
  result.keyId = m_currentKeyId;

  try {
    std::lock_guard<std::mutex> lock(m_cryptoMutex);

    switch (m_config.algorithm) {
    case EncryptionAlgorithm::AES256_GCM:
      result = encryptAES256GCM(plaintext);
      break;
    case EncryptionAlgorithm::AES256_CBC:
      result = encryptAES256CBC(plaintext);
      break;
    case EncryptionAlgorithm::ChaCha20_Poly1305:
      result = encryptChaCha20Poly1305(plaintext);
      break;
    default:
      LOG_WARN("unsupported encryption algorithm");
      result.algorithm = EncryptionAlgorithm::None;
      result.ciphertext = plaintext;
      break;
    }

    result.keyId = m_currentKeyId;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    updateEncryptionStats(duration, data.size(), true);

    LOG_DEBUG("encrypted %zu bytes to %zu bytes", data.size(), result.ciphertext.size());

  } catch (...) {
    LOG_ERR("encryption failed");
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    updateEncryptionStats(duration, data.size(), false);

    // Return unencrypted data on failure
    result.algorithm = EncryptionAlgorithm::None;
    result.ciphertext = std::vector<uint8_t>(data.begin(), data.end());
  }

  return result;
}

std::string EiClipboardCrypto::decrypt(const EncryptedData &encryptedData)
{
  auto startTime = std::chrono::steady_clock::now();

  if (!m_initialized || encryptedData.algorithm == EncryptionAlgorithm::None) {
    // Data is not encrypted
    std::string result(encryptedData.ciphertext.begin(), encryptedData.ciphertext.end());
    return result;
  }

  try {
    std::lock_guard<std::mutex> lock(m_cryptoMutex);

    std::vector<uint8_t> plaintext;

    switch (encryptedData.algorithm) {
    case EncryptionAlgorithm::AES256_GCM:
      plaintext = decryptAES256GCM(encryptedData);
      break;
    case EncryptionAlgorithm::AES256_CBC:
      plaintext = decryptAES256CBC(encryptedData);
      break;
    case EncryptionAlgorithm::ChaCha20_Poly1305:
      plaintext = decryptChaCha20Poly1305(encryptedData);
      break;
    default:
      LOG_WARN("unsupported decryption algorithm");
      plaintext = encryptedData.ciphertext;
      break;
    }

    std::string result;

    // Decompress if needed
    if (m_config.compressBeforeEncrypt && encryptedData.algorithm != EncryptionAlgorithm::None) {
      result = decompress(plaintext);
    } else {
      result = std::string(plaintext.begin(), plaintext.end());
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    updateDecryptionStats(duration, result.size(), true);

    LOG_DEBUG("decrypted %zu bytes to %zu bytes", encryptedData.ciphertext.size(), result.size());
    return result;

  } catch (...) {
    LOG_ERR("decryption failed");
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    updateDecryptionStats(duration, 0, false);

    // Return empty string on failure
    return "";
  }
}

bool EiClipboardCrypto::shouldEncrypt(const std::string &data) const
{
  if (!m_config.encryptSensitiveOnly) {
    return true;
  }

  return containsSensitivePatterns(data);
}

std::vector<uint8_t> EiClipboardCrypto::compress(const std::string &data) const
{
  // Simplified compression - in production, use zlib, lz4, or similar
  // For now, just return the data as-is
  return std::vector<uint8_t>(data.begin(), data.end());
}

std::string EiClipboardCrypto::decompress(const std::vector<uint8_t> &compressedData) const
{
  // Simplified decompression - in production, use zlib, lz4, or similar
  // For now, just return the data as-is
  return std::string(compressedData.begin(), compressedData.end());
}

std::string EiClipboardCrypto::serialize(const EncryptedData &data) const
{
  // Simple serialization format: algorithm|keyId|nonce|tag|salt|ciphertext
  // In production, use a proper serialization format like protobuf or msgpack

  std::stringstream ss;
  ss << static_cast<int>(data.algorithm) << "|";
  ss << data.keyId << "|";

  // Encode binary data as hex
  auto encodeHex = [](const std::vector<uint8_t> &bytes) {
    std::stringstream hex;
    for (auto byte : bytes) {
      hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return hex.str();
  };

  ss << encodeHex(data.nonce) << "|";
  ss << encodeHex(data.tag) << "|";
  ss << encodeHex(data.salt) << "|";
  ss << encodeHex(data.ciphertext);

  return ss.str();
}

EncryptedData EiClipboardCrypto::deserialize(const std::string &serialized) const
{
  EncryptedData result;

  // Parse the serialized format
  std::vector<std::string> parts;
  std::stringstream ss(serialized);
  std::string part;

  while (std::getline(ss, part, '|')) {
    parts.push_back(part);
  }

  if (parts.size() != 6) {
    LOG_WARN("invalid serialized data format");
    return result;
  }

  // Decode hex strings
  auto decodeHex = [](const std::string &hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
      std::string byteString = hex.substr(i, 2);
      uint8_t byte = static_cast<uint8_t>(std::strtol(byteString.c_str(), nullptr, 16));
      bytes.push_back(byte);
    }
    return bytes;
  };

  try {
    result.algorithm = static_cast<EncryptionAlgorithm>(std::stoi(parts[0]));
    result.keyId = parts[1];
    result.nonce = decodeHex(parts[2]);
    result.tag = decodeHex(parts[3]);
    result.salt = decodeHex(parts[4]);
    result.ciphertext = decodeHex(parts[5]);
    result.timestamp = std::chrono::system_clock::now();
  } catch (...) {
    LOG_ERR("failed to deserialize encrypted data");
    result.algorithm = EncryptionAlgorithm::None;
  }

  return result;
}

const CryptoConfig &EiClipboardCrypto::getConfig() const
{
  return m_config;
}

void EiClipboardCrypto::setConfig(const CryptoConfig &config)
{
  std::lock_guard<std::mutex> lock(m_cryptoMutex);
  m_config = config;
  LOG_DEBUG("crypto configuration updated");
}

bool EiClipboardCrypto::isAvailable() const
{
  return m_initialized && m_config.algorithm != EncryptionAlgorithm::None;
}

EiClipboardCrypto::CryptoStats EiClipboardCrypto::getStatistics() const
{
  std::lock_guard<std::mutex> lock(m_statsMutex);
  return m_stats;
}

void EiClipboardCrypto::clear()
{
  std::lock_guard<std::mutex> keyLock(m_keyMutex);
  std::lock_guard<std::mutex> statsLock(m_statsMutex);

  m_keys.clear();
  m_masterPassword.clear();
  m_currentKeyId.clear();
  m_stats = CryptoStats{};

  LOG_DEBUG("crypto state cleared");
}

void EiClipboardCrypto::rotateKeys()
{
  auto now = std::chrono::system_clock::now();
  if (now - m_lastKeyRotation < m_config.keyRotationInterval) {
    return; // Too early for rotation
  }

  std::string oldKeyId = m_currentKeyId;
  m_currentKeyId = generateKey();
  m_lastKeyRotation = now;

  LOG_INFO("encryption key rotated from %s to %s", oldKeyId.c_str(), m_currentKeyId.c_str());
}

bool EiClipboardCrypto::validateIntegrity(const EncryptedData &data) const
{
  // Basic validation - in production, implement proper integrity checks
  return !data.ciphertext.empty() && (data.algorithm == EncryptionAlgorithm::None || !data.keyId.empty());
}

std::vector<uint8_t> EiClipboardCrypto::deriveKey(const std::string &password, const std::vector<uint8_t> &salt) const
{
  // Simplified key derivation - in production, use PBKDF2, Argon2, or similar
  std::vector<uint8_t> key(m_config.keySize);

  // Simple XOR-based derivation for demonstration
  std::hash<std::string> hasher;
  size_t hash = hasher(password + std::string(salt.begin(), salt.end()));

  for (size_t i = 0; i < m_config.keySize; ++i) {
    key[i] = static_cast<uint8_t>((hash >> (i % 8)) & 0xFF);
  }

  return key;
}

std::vector<uint8_t> EiClipboardCrypto::generateRandomBytes(size_t size) const
{
  std::vector<uint8_t> bytes(size);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  for (size_t i = 0; i < size; ++i) {
    bytes[i] = static_cast<uint8_t>(dis(gen));
  }

  return bytes;
}

EncryptedData EiClipboardCrypto::encryptAES256GCM(const std::vector<uint8_t> &plaintext) const
{
  // Simplified AES-GCM implementation for demonstration
  // In production, use OpenSSL or similar crypto library

  EncryptedData result;
  result.algorithm = EncryptionAlgorithm::AES256_GCM;
  result.nonce = generateRandomBytes(m_config.nonceSize);
  result.salt = generateRandomBytes(m_config.saltSize);

  // For demonstration, just XOR with key (NOT SECURE!)
  auto keyIt = m_keys.find(m_currentKeyId);
  if (keyIt == m_keys.end()) {
    throw std::runtime_error("encryption key not found");
  }

  const auto &key = keyIt->second;
  result.ciphertext.resize(plaintext.size());

  for (size_t i = 0; i < plaintext.size(); ++i) {
    result.ciphertext[i] = plaintext[i] ^ key[i % key.size()] ^ result.nonce[i % result.nonce.size()];
  }

  // Generate authentication tag (simplified)
  result.tag = generateRandomBytes(m_config.tagSize);

  return result;
}

std::vector<uint8_t> EiClipboardCrypto::decryptAES256GCM(const EncryptedData &data) const
{
  // Simplified AES-GCM decryption for demonstration
  auto keyIt = m_keys.find(data.keyId);
  if (keyIt == m_keys.end()) {
    throw std::runtime_error("decryption key not found");
  }

  const auto &key = keyIt->second;
  std::vector<uint8_t> plaintext(data.ciphertext.size());

  for (size_t i = 0; i < data.ciphertext.size(); ++i) {
    plaintext[i] = data.ciphertext[i] ^ key[i % key.size()] ^ data.nonce[i % data.nonce.size()];
  }

  return plaintext;
}

EncryptedData EiClipboardCrypto::encryptAES256CBC(const std::vector<uint8_t> &plaintext) const
{
  // Placeholder for AES-CBC implementation
  return encryptAES256GCM(plaintext); // Use GCM for now
}

std::vector<uint8_t> EiClipboardCrypto::decryptAES256CBC(const EncryptedData &data) const
{
  // Placeholder for AES-CBC implementation
  return decryptAES256GCM(data); // Use GCM for now
}

EncryptedData EiClipboardCrypto::encryptChaCha20Poly1305(const std::vector<uint8_t> &plaintext) const
{
  // Placeholder for ChaCha20-Poly1305 implementation
  return encryptAES256GCM(plaintext); // Use GCM for now
}

std::vector<uint8_t> EiClipboardCrypto::decryptChaCha20Poly1305(const EncryptedData &data) const
{
  // Placeholder for ChaCha20-Poly1305 implementation
  return decryptAES256GCM(data); // Use GCM for now
}

void EiClipboardCrypto::updateEncryptionStats(std::chrono::microseconds duration, size_t bytes, bool success)
{
  std::lock_guard<std::mutex> lock(m_statsMutex);

  m_stats.totalEncryptions++;
  if (success) {
    m_stats.totalBytesEncrypted += bytes;

    // Update average time
    auto totalTime = m_stats.averageEncryptionTime * (m_stats.totalEncryptions - 1) + duration;
    m_stats.averageEncryptionTime = totalTime / m_stats.totalEncryptions;
  } else {
    m_stats.encryptionFailures++;
  }
}

void EiClipboardCrypto::updateDecryptionStats(std::chrono::microseconds duration, size_t bytes, bool success)
{
  std::lock_guard<std::mutex> lock(m_statsMutex);

  m_stats.totalDecryptions++;
  if (success) {
    m_stats.totalBytesDecrypted += bytes;

    // Update average time
    auto totalTime = m_stats.averageDecryptionTime * (m_stats.totalDecryptions - 1) + duration;
    m_stats.averageDecryptionTime = totalTime / m_stats.totalDecryptions;
  } else {
    m_stats.decryptionFailures++;
  }
}

bool EiClipboardCrypto::containsSensitivePatterns(const std::string &data) const
{
  // Reuse the sensitive data detection from EiClipboard
  std::vector<std::regex> sensitivePatterns = {
      std::regex(R"(\b\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4}\b)"),         // Credit cards
      std::regex(R"(\b\d{3}-\d{2}-\d{4}\b)"),                              // SSN
      std::regex(R"(password\s*[:=]\s*\S+)", std::regex_constants::icase), // Passwords
      std::regex(R"(\b[A-Za-z0-9]{32,}\b)"),                               // API keys
  };

  for (const auto &pattern : sensitivePatterns) {
    if (std::regex_search(data, pattern)) {
      return true;
    }
  }

  return false;
}

// ClipboardCryptoScope implementation

ClipboardCryptoScope::ClipboardCryptoScope(std::shared_ptr<EiClipboardCrypto> crypto, const std::string &data)
    : m_crypto(crypto),
      m_originalData(data),
      m_encrypted(false)
{
  if (m_crypto && m_crypto->isAvailable()) {
    m_encryptedData = m_crypto->encrypt(data);
    m_encrypted = (m_encryptedData.algorithm != EncryptionAlgorithm::None);
  }
}

ClipboardCryptoScope::~ClipboardCryptoScope() = default;

std::string ClipboardCryptoScope::getEncryptedData() const
{
  if (!m_encrypted || !m_crypto) {
    return m_originalData;
  }

  return m_crypto->serialize(m_encryptedData);
}

bool ClipboardCryptoScope::isEncrypted() const
{
  return m_encrypted;
}

size_t ClipboardCryptoScope::getOriginalSize() const
{
  return m_originalData.size();
}

size_t ClipboardCryptoScope::getEncryptedSize() const
{
  if (!m_encrypted) {
    return m_originalData.size();
  }

  return m_encryptedData.ciphertext.size();
}

} // namespace deskflow

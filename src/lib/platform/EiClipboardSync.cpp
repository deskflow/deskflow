/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboardSync.h"
#include "base/Log.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>

// Note: In a production environment, you would use proper compression libraries
// like LZ4, ZSTD, zlib, etc. This is a simplified implementation for demonstration.

namespace deskflow {

EiClipboardSync::EiClipboardSync(const SyncConfig &config)
    : m_config(config),
      m_lastNetworkUpdate(std::chrono::steady_clock::now())
{
  LOG_DEBUG(
      "clipboard sync initialized with compression %d, delta mode %d", static_cast<int>(m_config.compression),
      static_cast<int>(m_config.deltaMode)
  );
}

EiClipboardSync::~EiClipboardSync()
{
  clear();
}

SyncPacket EiClipboardSync::prepareSync(
    IClipboard::EFormat format, const std::string &mimeType, const std::string &data, const std::string &baseHash
)
{
  auto startTime = std::chrono::steady_clock::now();

  SyncPacket packet;
  packet.id = generatePacketId();
  packet.format = format;
  packet.mimeType = mimeType;
  packet.originalSize = data.size();
  packet.timestamp = std::chrono::system_clock::now();

  std::vector<uint8_t> processedData(data.begin(), data.end());

  // Try delta compression if base data is available
  bool usedDelta = false;
  if (!baseHash.empty() && data.size() >= m_config.deltaThreshold) {
    auto baseData = getFromHistory(baseHash);
    if (!baseData.empty()) {
      double similarity = calculateSimilarity(baseData, processedData);
      if (similarity >= m_config.deltaSimilarityThreshold) {
        DeltaMode deltaMode = m_config.deltaMode;
        if (deltaMode == DeltaMode::Adaptive) {
          deltaMode = chooseDeltaMode(baseData, processedData);
        }

        if (deltaMode != DeltaMode::None) {
          auto deltaData = createDelta(baseData, processedData, deltaMode);
          if (deltaData.size() < processedData.size() * 0.8) { // Only use if significant savings
            processedData = deltaData;
            packet.deltaMode = deltaMode;
            packet.baseHash = baseHash;
            usedDelta = true;
            LOG_DEBUG(
                "using delta compression: %zu -> %zu bytes (%.1f%% savings)", data.size(), deltaData.size(),
                (1.0 - static_cast<double>(deltaData.size()) / data.size()) * 100
            );
          }
        }
      }
    }
  }

  // Apply compression if beneficial
  if (processedData.size() >= m_config.compressionThreshold) {
    CompressionAlgorithm compression = m_config.compression;
    if (m_config.enableAdaptiveCompression) {
      compression = chooseCompression(processedData);
    }

    if (compression != CompressionAlgorithm::None) {
      auto compressedData = compress(processedData, compression);
      if (compressedData.size() < processedData.size() * 0.9) { // Only use if beneficial
        packet.data = compressedData;
        packet.compression = compression;
        packet.compressedSize = compressedData.size();
        LOG_DEBUG(
            "compressed data: %zu -> %zu bytes (%.1f%% savings)", processedData.size(), compressedData.size(),
            (1.0 - static_cast<double>(compressedData.size()) / processedData.size()) * 100
        );
      } else {
        packet.data = processedData;
        packet.compressedSize = processedData.size();
      }
    } else {
      packet.data = processedData;
      packet.compressedSize = processedData.size();
    }
  } else {
    packet.data = processedData;
    packet.compressedSize = processedData.size();
  }

  // Add to history for future delta operations
  std::string dataHash = calculateHash(std::vector<uint8_t>(data.begin(), data.end()));
  addToHistory(dataHash, std::vector<uint8_t>(data.begin(), data.end()));

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

  // Update statistics
  {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats.totalBytesOriginal += data.size();
    m_stats.totalBytesTransferred += packet.data.size();
    m_stats.totalPackets++;

    if (packet.compression != CompressionAlgorithm::None) {
      m_stats.compressionUsage[packet.compression]++;
    }
    if (usedDelta) {
      m_stats.deltaUsage[packet.deltaMode]++;
    }
  }

  LOG_DEBUG(
      "prepared sync packet: %s, %zu -> %zu bytes, delta: %s, compression: %d", packet.id.c_str(), packet.originalSize,
      packet.data.size(), usedDelta ? "yes" : "no", static_cast<int>(packet.compression)
  );

  return packet;
}

std::string EiClipboardSync::applySync(const SyncPacket &packet, const std::string &baseData)
{
  auto startTime = std::chrono::steady_clock::now();

  std::vector<uint8_t> processedData = packet.data;

  // Decompress if needed
  if (packet.compression != CompressionAlgorithm::None) {
    processedData = decompress(processedData, packet.compression);
    LOG_DEBUG("decompressed data: %zu -> %zu bytes", packet.data.size(), processedData.size());
  }

  // Apply delta if needed
  if (packet.deltaMode != DeltaMode::None && !packet.baseHash.empty()) {
    std::vector<uint8_t> base;
    if (!baseData.empty()) {
      base = std::vector<uint8_t>(baseData.begin(), baseData.end());
    } else {
      base = getFromHistory(packet.baseHash);
    }

    if (!base.empty()) {
      processedData = applyDelta(base, processedData, packet.deltaMode);
      LOG_DEBUG("applied delta: %zu + %zu -> %zu bytes", base.size(), packet.data.size(), processedData.size());
    } else {
      LOG_WARN("base data not found for delta application");
      return ""; // Cannot apply delta without base
    }
  }

  std::string result(processedData.begin(), processedData.end());

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

  LOG_DEBUG("applied sync packet: %s, %zu -> %zu bytes", packet.id.c_str(), packet.data.size(), result.size());

  return result;
}

std::vector<uint8_t> EiClipboardSync::compress(const std::vector<uint8_t> &data, CompressionAlgorithm algorithm) const
{
  auto startTime = std::chrono::steady_clock::now();

  std::vector<uint8_t> result;

  try {
    switch (algorithm) {
    case CompressionAlgorithm::LZ4:
      result = compressLZ4(data);
      break;
    case CompressionAlgorithm::ZSTD:
      result = compressZSTD(data);
      break;
    case CompressionAlgorithm::GZIP:
      result = compressGZIP(data);
      break;
    case CompressionAlgorithm::BROTLI:
      // Fallback to GZIP for now
      result = compressGZIP(data);
      break;
    default:
      result = data;
      break;
    }
  } catch (...) {
    LOG_ERR("compression failed, using uncompressed data");
    result = data;
  }

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

  // Update statistics (const_cast for statistics update)
  const_cast<EiClipboardSync *>(this)->updateCompressionStats(algorithm, data.size(), result.size(), duration);

  return result;
}

std::vector<uint8_t>
EiClipboardSync::decompress(const std::vector<uint8_t> &compressedData, CompressionAlgorithm algorithm) const
{
  auto startTime = std::chrono::steady_clock::now();

  std::vector<uint8_t> result;

  try {
    switch (algorithm) {
    case CompressionAlgorithm::LZ4:
      result = decompressLZ4(compressedData);
      break;
    case CompressionAlgorithm::ZSTD:
      result = decompressZSTD(compressedData);
      break;
    case CompressionAlgorithm::GZIP:
      result = decompressGZIP(compressedData);
      break;
    case CompressionAlgorithm::BROTLI:
      // Fallback to GZIP for now
      result = decompressGZIP(compressedData);
      break;
    default:
      result = compressedData;
      break;
    }
  } catch (...) {
    LOG_ERR("decompression failed");
    throw;
  }

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

  return result;
}

std::vector<uint8_t> EiClipboardSync::createDelta(
    const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData, DeltaMode mode
) const
{
  auto startTime = std::chrono::steady_clock::now();

  std::vector<uint8_t> result;

  try {
    switch (mode) {
    case DeltaMode::Binary:
      result = createBinaryDelta(oldData, newData);
      break;
    case DeltaMode::Text:
      result = createTextDelta(oldData, newData);
      break;
    case DeltaMode::Adaptive:
      // Choose based on data characteristics
      if (analyzeData(newData).isText) {
        result = createTextDelta(oldData, newData);
      } else {
        result = createBinaryDelta(oldData, newData);
      }
      break;
    default:
      result = newData;
      break;
    }
  } catch (...) {
    LOG_ERR("delta creation failed, using full data");
    result = newData;
  }

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

  // Update statistics
  const_cast<EiClipboardSync *>(this)->updateDeltaStats(mode, newData.size(), result.size(), duration);

  return result;
}

std::vector<uint8_t> EiClipboardSync::applyDelta(
    const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta, DeltaMode mode
) const
{
  try {
    switch (mode) {
    case DeltaMode::Binary:
      return applyBinaryDelta(baseData, delta);
    case DeltaMode::Text:
      return applyTextDelta(baseData, delta);
    case DeltaMode::Adaptive:
      // Try binary first, then text
      try {
        return applyBinaryDelta(baseData, delta);
      } catch (...) {
        return applyTextDelta(baseData, delta);
      }
    default:
      return delta;
    }
  } catch (...) {
    LOG_ERR("delta application failed");
    throw;
  }
}

double EiClipboardSync::calculateSimilarity(const std::vector<uint8_t> &data1, const std::vector<uint8_t> &data2) const
{
  if (data1.empty() || data2.empty()) {
    return 0.0;
  }

  // Simple similarity calculation using longest common subsequence
  size_t maxLen = std::max(data1.size(), data2.size());
  size_t minLen = std::min(data1.size(), data2.size());

  // Count matching bytes at the beginning
  size_t commonPrefix = 0;
  for (size_t i = 0; i < minLen && data1[i] == data2[i]; ++i) {
    commonPrefix++;
  }

  // Count matching bytes at the end
  size_t commonSuffix = 0;
  for (size_t i = 1; i <= minLen - commonPrefix && data1[data1.size() - i] == data2[data2.size() - i]; ++i) {
    commonSuffix++;
  }

  size_t commonBytes = commonPrefix + commonSuffix;
  return static_cast<double>(commonBytes) / maxLen;
}

CompressionAlgorithm EiClipboardSync::chooseCompression(const std::vector<uint8_t> &data) const
{
  auto characteristics = analyzeData(data);

  if (!characteristics.isCompressible) {
    return CompressionAlgorithm::None;
  }

  // Choose based on data characteristics and size
  if (data.size() < 1024) {
    return CompressionAlgorithm::LZ4; // Fast for small data
  } else if (characteristics.isText) {
    return CompressionAlgorithm::GZIP; // Good for text
  } else if (characteristics.repetitionFactor > 2.0) {
    return CompressionAlgorithm::ZSTD; // Good for repetitive data
  } else {
    return CompressionAlgorithm::LZ4; // Fast general purpose
  }
}

DeltaMode
EiClipboardSync::chooseDeltaMode(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const
{
  auto oldChar = analyzeData(oldData);
  auto newChar = analyzeData(newData);

  if (oldChar.isText && newChar.isText) {
    return DeltaMode::Text;
  } else {
    return DeltaMode::Binary;
  }
}

std::string EiClipboardSync::generatePacketId() const
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);

  std::stringstream ss;
  for (int i = 0; i < 16; ++i) {
    ss << std::hex << dis(gen);
  }
  return ss.str();
}

void EiClipboardSync::addToHistory(const std::string &hash, const std::vector<uint8_t> &data)
{
  std::lock_guard<std::mutex> lock(m_historyMutex);

  // Add to history
  m_dataHistory[hash] = data;
  m_historyOrder.push_back(hash);

  // Cleanup old entries
  while (m_historyOrder.size() > m_config.maxHistorySize) {
    std::string oldHash = m_historyOrder.front();
    m_historyOrder.pop_front();
    m_dataHistory.erase(oldHash);
  }
}

std::vector<uint8_t> EiClipboardSync::getFromHistory(const std::string &hash) const
{
  std::lock_guard<std::mutex> lock(m_historyMutex);

  auto it = m_dataHistory.find(hash);
  return (it != m_dataHistory.end()) ? it->second : std::vector<uint8_t>{};
}

std::string EiClipboardSync::calculateHash(const std::vector<uint8_t> &data) const
{
  // Simple hash calculation - in production, use SHA-256 or similar
  std::hash<std::string> hasher;
  std::string dataStr(data.begin(), data.end());
  size_t hashValue = hasher(dataStr);

  std::stringstream ss;
  ss << std::hex << hashValue;
  return ss.str();
}

const SyncConfig &EiClipboardSync::getConfig() const
{
  return m_config;
}

void EiClipboardSync::setConfig(const SyncConfig &config)
{
  m_config = config;
  LOG_DEBUG("sync configuration updated");
}

BandwidthStats EiClipboardSync::getStatistics() const
{
  std::lock_guard<std::mutex> lock(m_statsMutex);

  BandwidthStats stats = m_stats;

  // Calculate derived statistics
  if (stats.totalBytesOriginal > 0) {
    stats.compressionRatio = 1.0 - static_cast<double>(stats.totalBytesTransferred) / stats.totalBytesOriginal;
  }

  return stats;
}

void EiClipboardSync::clear()
{
  std::lock_guard<std::mutex> historyLock(m_historyMutex);
  std::lock_guard<std::mutex> statsLock(m_statsMutex);

  m_dataHistory.clear();
  m_historyOrder.clear();
  m_stats = BandwidthStats{};

  LOG_DEBUG("sync state cleared");
}

// Simplified compression implementations (for demonstration)
// In production, use proper compression libraries

std::vector<uint8_t> EiClipboardSync::compressLZ4(const std::vector<uint8_t> &data) const
{
  // Simplified LZ4-style compression
  // In production, use the actual LZ4 library

  if (data.size() < 16) {
    return data; // Too small to compress effectively
  }

  std::vector<uint8_t> compressed;
  compressed.reserve(data.size());

  // Simple run-length encoding as a placeholder
  for (size_t i = 0; i < data.size();) {
    uint8_t byte = data[i];
    size_t count = 1;

    while (i + count < data.size() && data[i + count] == byte && count < 255) {
      count++;
    }

    if (count > 3) {
      // Encode as run
      compressed.push_back(0xFF); // Escape byte
      compressed.push_back(static_cast<uint8_t>(count));
      compressed.push_back(byte);
    } else {
      // Encode literally
      for (size_t j = 0; j < count; ++j) {
        compressed.push_back(byte);
      }
    }

    i += count;
  }

  return compressed;
}

std::vector<uint8_t> EiClipboardSync::decompressLZ4(const std::vector<uint8_t> &compressed) const
{
  std::vector<uint8_t> decompressed;
  decompressed.reserve(compressed.size() * 2);

  for (size_t i = 0; i < compressed.size();) {
    if (compressed[i] == 0xFF && i + 2 < compressed.size()) {
      // Run-length encoded
      uint8_t count = compressed[i + 1];
      uint8_t byte = compressed[i + 2];

      for (uint8_t j = 0; j < count; ++j) {
        decompressed.push_back(byte);
      }

      i += 3;
    } else {
      // Literal byte
      decompressed.push_back(compressed[i]);
      i++;
    }
  }

  return decompressed;
}

std::vector<uint8_t> EiClipboardSync::compressZSTD(const std::vector<uint8_t> &data) const
{
  // Placeholder - use LZ4 for now
  return compressLZ4(data);
}

std::vector<uint8_t> EiClipboardSync::decompressZSTD(const std::vector<uint8_t> &compressed) const
{
  // Placeholder - use LZ4 for now
  return decompressLZ4(compressed);
}

std::vector<uint8_t> EiClipboardSync::compressGZIP(const std::vector<uint8_t> &data) const
{
  // Placeholder - use LZ4 for now
  return compressLZ4(data);
}

std::vector<uint8_t> EiClipboardSync::decompressGZIP(const std::vector<uint8_t> &compressed) const
{
  // Placeholder - use LZ4 for now
  return decompressLZ4(compressed);
}

std::vector<uint8_t>
EiClipboardSync::createBinaryDelta(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const
{
  // Simplified binary delta - in production, use bsdiff or similar
  std::vector<uint8_t> delta;

  // Simple approach: store operations as (operation, offset, length, data)
  // Operation: 0 = copy from old, 1 = insert new data

  size_t oldPos = 0, newPos = 0;

  while (newPos < newData.size()) {
    // Find longest match in old data
    size_t bestOldPos = 0, bestLength = 0;

    for (size_t searchPos = 0; searchPos <= oldData.size(); ++searchPos) {
      size_t length = 0;
      while (searchPos + length < oldData.size() && newPos + length < newData.size() &&
             oldData[searchPos + length] == newData[newPos + length]) {
        length++;
      }

      if (length > bestLength) {
        bestLength = length;
        bestOldPos = searchPos;
      }
    }

    if (bestLength >= 4) {
      // Copy operation
      delta.push_back(0); // Copy operation
      delta.push_back(static_cast<uint8_t>(bestOldPos & 0xFF));
      delta.push_back(static_cast<uint8_t>((bestOldPos >> 8) & 0xFF));
      delta.push_back(static_cast<uint8_t>(bestLength));
      newPos += bestLength;
    } else {
      // Insert operation
      delta.push_back(1); // Insert operation
      delta.push_back(newData[newPos]);
      newPos++;
    }
  }

  return delta;
}

std::vector<uint8_t>
EiClipboardSync::applyBinaryDelta(const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta) const
{
  std::vector<uint8_t> result;

  for (size_t i = 0; i < delta.size();) {
    uint8_t operation = delta[i++];

    if (operation == 0) {
      // Copy operation
      if (i + 3 >= delta.size())
        break;

      size_t offset = delta[i] | (delta[i + 1] << 8);
      size_t length = delta[i + 2];
      i += 3;

      for (size_t j = 0; j < length && offset + j < baseData.size(); ++j) {
        result.push_back(baseData[offset + j]);
      }
    } else if (operation == 1) {
      // Insert operation
      if (i >= delta.size())
        break;
      result.push_back(delta[i++]);
    }
  }

  return result;
}

std::vector<uint8_t>
EiClipboardSync::createTextDelta(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const
{
  // Simplified text delta - in production, use proper diff algorithms
  // For now, use binary delta
  return createBinaryDelta(oldData, newData);
}

std::vector<uint8_t>
EiClipboardSync::applyTextDelta(const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta) const
{
  // Simplified text delta application
  return applyBinaryDelta(baseData, delta);
}

void EiClipboardSync::updateCompressionStats(
    CompressionAlgorithm algorithm, size_t originalSize, size_t compressedSize, std::chrono::microseconds duration
)
{
  std::lock_guard<std::mutex> lock(m_statsMutex);

  // Update compression time average
  static size_t compressionCount = 0;
  compressionCount++;

  auto totalTime = m_stats.averageCompressionTime * (compressionCount - 1) + duration;
  m_stats.averageCompressionTime = totalTime / compressionCount;
}

void EiClipboardSync::updateDeltaStats(
    DeltaMode mode, size_t originalSize, size_t deltaSize, std::chrono::microseconds duration
)
{
  std::lock_guard<std::mutex> lock(m_statsMutex);

  // Update delta time average
  static size_t deltaCount = 0;
  deltaCount++;

  auto totalTime = m_stats.averageDeltaTime * (deltaCount - 1) + duration;
  m_stats.averageDeltaTime = totalTime / deltaCount;

  // Update delta savings
  if (originalSize > 0) {
    double savings = 1.0 - static_cast<double>(deltaSize) / originalSize;
    m_stats.deltaSavings = (m_stats.deltaSavings * (deltaCount - 1) + savings) / deltaCount;
  }
}

EiClipboardSync::DataCharacteristics EiClipboardSync::analyzeData(const std::vector<uint8_t> &data) const
{
  DataCharacteristics characteristics;

  if (data.empty()) {
    return characteristics;
  }

  // Check if data is text
  size_t textBytes = 0;
  for (uint8_t byte : data) {
    if ((byte >= 32 && byte <= 126) || byte == '\t' || byte == '\n' || byte == '\r') {
      textBytes++;
    }
  }
  characteristics.isText = (static_cast<double>(textBytes) / data.size()) > 0.8;

  // Calculate entropy (simplified)
  std::map<uint8_t, size_t> frequency;
  for (uint8_t byte : data) {
    frequency[byte]++;
  }

  double entropy = 0.0;
  for (const auto &pair : frequency) {
    double p = static_cast<double>(pair.second) / data.size();
    entropy -= p * std::log2(p);
  }
  characteristics.entropy = entropy;

  // Check compressibility (low entropy = more compressible)
  characteristics.isCompressible = entropy < 7.0;

  // Calculate repetition factor
  size_t uniqueBytes = frequency.size();
  characteristics.repetitionFactor = static_cast<double>(data.size()) / uniqueBytes;

  return characteristics;
}

std::vector<SyncPacket> EiClipboardSync::createProgressivePackets(const SyncPacket &largePacket) const
{
  std::vector<SyncPacket> packets;

  if (largePacket.data.size() <= m_config.maxPacketSize) {
    packets.push_back(largePacket);
    return packets;
  }

  // Split into chunks
  size_t chunkSize = m_config.maxPacketSize;
  size_t totalChunks = (largePacket.data.size() + chunkSize - 1) / chunkSize;

  for (size_t i = 0; i < totalChunks; ++i) {
    SyncPacket chunk = largePacket;
    chunk.id = largePacket.id + "_" + std::to_string(i);

    size_t start = i * chunkSize;
    size_t end = std::min(start + chunkSize, largePacket.data.size());

    chunk.data = std::vector<uint8_t>(largePacket.data.begin() + start, largePacket.data.begin() + end);
    chunk.compressedSize = chunk.data.size();

    packets.push_back(chunk);
  }

  return packets;
}

SyncPacket EiClipboardSync::reassembleProgressivePackets(const std::vector<SyncPacket> &packets) const
{
  if (packets.empty()) {
    return SyncPacket{};
  }

  if (packets.size() == 1) {
    return packets[0];
  }

  SyncPacket result = packets[0];
  result.data.clear();

  // Reassemble data
  for (const auto &packet : packets) {
    result.data.insert(result.data.end(), packet.data.begin(), packet.data.end());
  }

  result.compressedSize = result.data.size();
  return result;
}

std::string EiClipboardSync::serializePacket(const SyncPacket &packet) const
{
  // Simple serialization - in production, use protobuf or similar
  std::stringstream ss;

  ss << packet.id << "|";
  ss << static_cast<int>(packet.format) << "|";
  ss << packet.mimeType << "|";
  ss << static_cast<int>(packet.compression) << "|";
  ss << static_cast<int>(packet.deltaMode) << "|";
  ss << packet.baseHash << "|";
  ss << packet.originalSize << "|";
  ss << packet.compressedSize << "|";
  ss << (packet.isEncrypted ? "1" : "0") << "|";

  // Encode binary data as hex
  for (uint8_t byte : packet.data) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }

  return ss.str();
}

SyncPacket EiClipboardSync::deserializePacket(const std::string &serialized) const
{
  SyncPacket packet;

  std::vector<std::string> parts;
  std::stringstream ss(serialized);
  std::string part;

  // Split by '|' delimiter
  for (int i = 0; i < 9 && std::getline(ss, part, '|'); ++i) {
    parts.push_back(part);
  }

  if (parts.size() < 9) {
    LOG_WARN("invalid serialized packet format");
    return packet;
  }

  try {
    packet.id = parts[0];
    packet.format = static_cast<IClipboard::EFormat>(std::stoi(parts[1]));
    packet.mimeType = parts[2];
    packet.compression = static_cast<CompressionAlgorithm>(std::stoi(parts[3]));
    packet.deltaMode = static_cast<DeltaMode>(std::stoi(parts[4]));
    packet.baseHash = parts[5];
    packet.originalSize = std::stoull(parts[6]);
    packet.compressedSize = std::stoull(parts[7]);
    packet.isEncrypted = (parts[8] == "1");

    // Decode hex data
    std::string hexData;
    std::getline(ss, hexData);

    for (size_t i = 0; i < hexData.length(); i += 2) {
      std::string byteString = hexData.substr(i, 2);
      uint8_t byte = static_cast<uint8_t>(std::strtol(byteString.c_str(), nullptr, 16));
      packet.data.push_back(byte);
    }

    packet.timestamp = std::chrono::system_clock::now();

  } catch (...) {
    LOG_ERR("failed to deserialize packet");
    packet = SyncPacket{};
  }

  return packet;
}

void EiClipboardSync::setProgressCallback(ProgressCallback callback)
{
  m_progressCallback = std::move(callback);
}

void EiClipboardSync::setCompletionCallback(CompletionCallback callback)
{
  m_completionCallback = std::move(callback);
}

std::chrono::milliseconds EiClipboardSync::estimateTransferTime(size_t dataSize) const
{
  std::lock_guard<std::mutex> lock(m_networkMutex);

  // Estimate based on current bandwidth and latency
  double transferTime = static_cast<double>(dataSize) / m_averageBandwidth; // seconds
  double totalTime = transferTime + (m_averageLatency.count() / 1000.0);    // add latency

  return std::chrono::milliseconds(static_cast<long long>(totalTime * 1000));
}

size_t EiClipboardSync::getOptimalPacketSize() const
{
  std::lock_guard<std::mutex> lock(m_networkMutex);

  // Adjust packet size based on network conditions
  if (m_averageLatency > std::chrono::milliseconds(100)) {
    // High latency - use larger packets
    return std::min(m_config.maxPacketSize, static_cast<size_t>(128 * 1024));
  } else if (m_averageLatency < std::chrono::milliseconds(10)) {
    // Low latency - can use smaller packets
    return std::min(m_config.maxPacketSize, static_cast<size_t>(32 * 1024));
  } else {
    // Medium latency - use default
    return m_config.maxPacketSize;
  }
}

} // namespace deskflow

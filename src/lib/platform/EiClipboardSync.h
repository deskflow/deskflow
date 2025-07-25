/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow {

//! Compression algorithm types
enum class CompressionAlgorithm
{
  None,  // No compression
  LZ4,   // LZ4 fast compression
  ZSTD,  // Zstandard compression
  GZIP,  // GZIP compression
  BROTLI // Brotli compression
};

//! Delta synchronization mode
enum class DeltaMode
{
  None,    // Full data transfer
  Binary,  // Binary delta (bsdiff-style)
  Text,    // Text-based delta
  Adaptive // Automatically choose best method
};

//! Synchronization packet
struct SyncPacket
{
  std::string id;                   // Unique packet ID
  IClipboard::EFormat format;       // Clipboard format
  std::string mimeType;             // MIME type
  std::vector<uint8_t> data;        // Packet data
  CompressionAlgorithm compression; // Compression used
  DeltaMode deltaMode;              // Delta mode used
  std::string baseHash;             // Hash of base data for delta
  size_t originalSize;              // Original uncompressed size
  size_t compressedSize;            // Compressed size
  std::chrono::system_clock::time_point timestamp;
  bool isEncrypted; // Whether data is encrypted

  SyncPacket()
      : format(IClipboard::kText),
        compression(CompressionAlgorithm::None),
        deltaMode(DeltaMode::None),
        originalSize(0),
        compressedSize(0),
        isEncrypted(false)
  {
  }
};

//! Synchronization configuration
struct SyncConfig
{
  CompressionAlgorithm compression = CompressionAlgorithm::LZ4;
  DeltaMode deltaMode = DeltaMode::Adaptive;
  size_t compressionThreshold = 1024;    // Minimum size to compress
  size_t deltaThreshold = 2048;          // Minimum size for delta sync
  double deltaSimilarityThreshold = 0.3; // Minimum similarity for delta
  size_t maxPacketSize = 64 * 1024;      // Maximum packet size (64KB)
  size_t maxHistorySize = 10;            // Number of previous versions to keep
  bool enableBandwidthOptimization = true;
  bool enableAdaptiveCompression = true;
  bool enableProgressiveTransfer = true; // Transfer large data in chunks
  std::chrono::milliseconds transferTimeout{5000};
};

//! Bandwidth optimization statistics
struct BandwidthStats
{
  size_t totalBytesOriginal = 0;
  size_t totalBytesTransferred = 0;
  size_t totalPackets = 0;
  double compressionRatio = 0.0;
  double deltaSavings = 0.0;
  std::chrono::microseconds averageCompressionTime{0};
  std::chrono::microseconds averageDecompressionTime{0};
  std::chrono::microseconds averageDeltaTime{0};
  std::map<CompressionAlgorithm, size_t> compressionUsage;
  std::map<DeltaMode, size_t> deltaUsage;
};

//! Clipboard synchronization optimizer
/*!
This class provides advanced synchronization optimizations for clipboard data
transfer between devices. It implements delta synchronization, compression,
and bandwidth optimization techniques to minimize network usage.
*/
class EiClipboardSync
{
public:
  //! Transfer progress callback
  using ProgressCallback = std::function<void(const std::string &id, size_t transferred, size_t total)>;

  //! Transfer completion callback
  using CompletionCallback = std::function<void(const std::string &id, bool success, const std::string &error)>;

  explicit EiClipboardSync(const SyncConfig &config = SyncConfig{});
  ~EiClipboardSync();

  //! Prepare data for synchronization
  SyncPacket prepareSync(
      IClipboard::EFormat format, const std::string &mimeType, const std::string &data, const std::string &baseHash = ""
  );

  //! Apply synchronized data
  std::string applySync(const SyncPacket &packet, const std::string &baseData = "");

  //! Compress data using specified algorithm
  std::vector<uint8_t> compress(const std::vector<uint8_t> &data, CompressionAlgorithm algorithm) const;

  //! Decompress data
  std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData, CompressionAlgorithm algorithm) const;

  //! Create binary delta between old and new data
  std::vector<uint8_t>
  createDelta(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData, DeltaMode mode) const;

  //! Apply binary delta to base data
  std::vector<uint8_t>
  applyDelta(const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta, DeltaMode mode) const;

  //! Calculate data similarity (0.0 = completely different, 1.0 = identical)
  double calculateSimilarity(const std::vector<uint8_t> &data1, const std::vector<uint8_t> &data2) const;

  //! Choose optimal compression algorithm for data
  CompressionAlgorithm chooseCompression(const std::vector<uint8_t> &data) const;

  //! Choose optimal delta mode for data
  DeltaMode chooseDeltaMode(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const;

  //! Split large data into progressive transfer chunks
  std::vector<SyncPacket> createProgressivePackets(const SyncPacket &largePacket) const;

  //! Reassemble progressive transfer chunks
  SyncPacket reassembleProgressivePackets(const std::vector<SyncPacket> &packets) const;

  //! Serialize packet for network transfer
  std::string serializePacket(const SyncPacket &packet) const;

  //! Deserialize packet from network data
  SyncPacket deserializePacket(const std::string &serialized) const;

  //! Add data to history for delta synchronization
  void addToHistory(const std::string &hash, const std::vector<uint8_t> &data);

  //! Get data from history by hash
  std::vector<uint8_t> getFromHistory(const std::string &hash) const;

  //! Calculate hash of data
  std::string calculateHash(const std::vector<uint8_t> &data) const;

  //! Get current configuration
  const SyncConfig &getConfig() const;

  //! Update configuration
  void setConfig(const SyncConfig &config);

  //! Get bandwidth statistics
  BandwidthStats getStatistics() const;

  //! Clear statistics and history
  void clear();

  //! Set progress callback
  void setProgressCallback(ProgressCallback callback);

  //! Set completion callback
  void setCompletionCallback(CompletionCallback callback);

  //! Estimate transfer time for data
  std::chrono::milliseconds estimateTransferTime(size_t dataSize) const;

  //! Get optimal packet size for current network conditions
  size_t getOptimalPacketSize() const;

private:
  //! Generate unique packet ID
  std::string generatePacketId() const;
  //! LZ4 compression implementation
  std::vector<uint8_t> compressLZ4(const std::vector<uint8_t> &data) const;
  std::vector<uint8_t> decompressLZ4(const std::vector<uint8_t> &compressed) const;

  //! ZSTD compression implementation
  std::vector<uint8_t> compressZSTD(const std::vector<uint8_t> &data) const;
  std::vector<uint8_t> decompressZSTD(const std::vector<uint8_t> &compressed) const;

  //! GZIP compression implementation
  std::vector<uint8_t> compressGZIP(const std::vector<uint8_t> &data) const;
  std::vector<uint8_t> decompressGZIP(const std::vector<uint8_t> &compressed) const;

  //! Binary delta implementation
  std::vector<uint8_t>
  createBinaryDelta(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const;
  std::vector<uint8_t> applyBinaryDelta(const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta) const;

  //! Text delta implementation
  std::vector<uint8_t> createTextDelta(const std::vector<uint8_t> &oldData, const std::vector<uint8_t> &newData) const;
  std::vector<uint8_t> applyTextDelta(const std::vector<uint8_t> &baseData, const std::vector<uint8_t> &delta) const;

  //! Update statistics
  void updateCompressionStats(
      CompressionAlgorithm algorithm, size_t originalSize, size_t compressedSize, std::chrono::microseconds duration
  );

  void updateDeltaStats(DeltaMode mode, size_t originalSize, size_t deltaSize, std::chrono::microseconds duration);

  //! Analyze data characteristics for optimization
  struct DataCharacteristics
  {
    bool isText;
    bool isCompressible;
    double entropy;
    size_t repetitionFactor;
  };

  DataCharacteristics analyzeData(const std::vector<uint8_t> &data) const;

  SyncConfig m_config;

  // History for delta synchronization
  mutable std::mutex m_historyMutex;
  std::map<std::string, std::vector<uint8_t>> m_dataHistory;
  std::deque<std::string> m_historyOrder; // For LRU cleanup

  // Statistics
  mutable std::mutex m_statsMutex;
  BandwidthStats m_stats;

  // Callbacks
  ProgressCallback m_progressCallback;
  CompletionCallback m_completionCallback;

  // Network adaptation
  mutable std::mutex m_networkMutex;
  std::chrono::milliseconds m_averageLatency{50};
  double m_averageBandwidth{1000000}; // bytes per second
  std::chrono::steady_clock::time_point m_lastNetworkUpdate;
};

} // namespace deskflow

#ifndef SDR_PROG_H
#define SDR_PROG_H

#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sndfile.hh>
#include <string>
#include <vector>

#include "acars_modulator.h"

// ==================== КИХ-Фильтр ====================
class FirFilter {
 private:
  std::vector<float> coeffs_;
  std::vector<std::complex<float>> history_;

 public:
  FirFilter(const std::vector<float>& coefficients);
  ~FirFilter();
  std::complex<float> process_sample(const std::complex<float> sample);
};

enum class State : uint8_t { HOLD = 0, SYNC = 1, PAYLOAD = 2, END = 3 };

// ==================== Парсер ACARS (FSM) ====================
class AcarsParser {
 private:
  AcarsConfig cfg_;
  size_t count_ = 0;
  uint8_t prev_bit_ = 1;
  std::vector<uint8_t> bits_;
  double samples_per_bit_;

 public:
  AcarsParser(const AcarsConfig& cfg)
      : cfg_(cfg),
        samples_per_bit_(static_cast<double>(cfg.sampleRate) / 2400.0) {}
  void recover_bits(uint8_t raw_bit);
  void parse_bitstream(const std::vector<uint8_t>& raw_bits);
};

// ==================== Утилиты ЦОС (Свободные функции) ====================

std::vector<uint8_t> extract_raw_bits(
    size_t size, const std::vector<std::complex<float>>& filtered1,
    const std::vector<std::complex<float>>& filtered2,
    const float squelch_threshold);

std::vector<std::complex<float>> downsample_signal(
    const std::vector<std::complex<float>>& input_signal, const int factor);

std::vector<float> load_coeffs_from_file(const std::string& filename);

std::vector<std::complex<float>> get_data_from_file(const std::string& filename,
                                                    size_t num_data);

std::vector<float> calculate_am_envelope(
    const std::vector<std::complex<float>>& in_data);

#endif
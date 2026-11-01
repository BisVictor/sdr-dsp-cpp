#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <sndfile.hh>
#include <string>
#include <vector>

// ==================== КИХ-Фильтр ====================
class FirFilter {
 private:
  std::vector<float> coeffs_;
  std::vector<std::complex<float>> history_;

 public:
  FirFilter(const std::vector<float>& coefficients);
  ~FirFilter();
  std::complex<float> process_sample(std::complex<float> sample);
};

// ==================== Парсер ACARS ====================
class AcarsParser {
 private:
  size_t sample_counter_ = 0;
  uint32_t shift_reg_ = 0;
  uint8_t bit_counter_ = 0;
  uint8_t current_byte_ = 0;
  bool is_capturing_ = false;
  bool invert_payload_ = false;  // Флаг автоматического переворота битов
  uint8_t last_raw_bit_ = 1;
  uint8_t last_stable_bit_ = 1;
  const int mid_sample_tick_ = 6;  // Проверенный центр бита

 public:
  AcarsParser() {};
  void parse_bitstream(const std::vector<uint8_t>& raw_bits);
};

// ==================== Утилиты ЦОС (Свободные функции) ====================

std::vector<uint8_t> extract_raw_bits(
    size_t size, const std::vector<std::complex<float>>& filtered1,
    const std::vector<std::complex<float>>& filtered2);

std::vector<std::complex<float>> downsample_signal(
    const std::vector<std::complex<float>>& input_signal, const int factor);

std::vector<float> load_coeffs_from_file(const std::string& filename);

std::vector<std::complex<float>> get_data_from_file(const std::string& filename,
                                                    size_t num_data);

std::vector<float> calculate_am_envelope(
    const std::vector<std::complex<float>>& in_data);
#include "SDR_prog.h"

#include "acars_modulator.h"

FirFilter::FirFilter(const std::vector<float>& coefficients)
    : coeffs_(coefficients),
      history_(coefficients.size(), std::complex<float>(0.0f, 0.0f)) {}

FirFilter::~FirFilter() {}

std::complex<float> FirFilter::process_sample(
    const std::complex<float> sample) {
  std::complex<float> acc = 0.0f;
  if (history_.empty()) {
    std::cout << "History is empty";
  }

  std::copy(history_.begin() + 1, history_.end(), history_.begin());
  history_[history_.size() - 1] = sample;
  for (size_t i = 0; i < coeffs_.size(); i++) {
    acc += history_[i] * coeffs_[i];
  }

  return acc;
}

/* class AcarsParser {
 private:
  size_t count_ = 0;
  uint8_t prev_bit_ = 1;
  std::vector<uint8_t> bits_;

 public:
  AcarsParser() {};
  void parse_bitstream(const std::vector<uint8_t>& raw_bits);

}; */

void AcarsParser::recover_bits(uint8_t raw_bit) {
  if (raw_bit == prev_bit_) {
    count_++;
    return;
  }
  // Завершаем предыдущую серию
  size_t bits_count =
      std::round(static_cast<double>(count_) / samples_per_bit_);

  for (size_t i = 0; i < bits_count; i++) {
    bits_.push_back(prev_bit_);
  }
  // Начинаем новую серию
  prev_bit_ = raw_bit;
  count_ = 1;
}

void AcarsParser::append_inverted_bit(std::vector<uint8_t>& raw_bits) {
  if (raw_bits.back() == 1) {
    raw_bits.push_back(0);
  } else {
    raw_bits.push_back(1);
  }
}

bool AcarsParser::check_preamble(size_t start_idx, size_t bit_count,
                                 uint32_t reference, int part) {
  uint32_t value = 0;
  for (size_t i = start_idx; i < start_idx + bit_count; i++) {
    value = (value << 1) | (bits_[i] & 1);
  }
  if (value == reference) {
    std::cout << "Check " << part << " part of preamble!!!\n";
    return true;
  } else {
    std::cout << "Don't see preamble part " << part << "! value = " << value
              << "\n";
    return false;
  }
}

std::vector<uint8_t> AcarsParser::parse_bitstream(
    std::vector<uint8_t>& raw_bits) {
  if (raw_bits.empty()) {
    std::cout << "Vector is empty\n";
    return bits_;
  }

  bits_.clear();
  count_ = 0;
  State state = State::HOLD;
  append_inverted_bit(raw_bits);

  for (uint64_t i = 0; i < raw_bits.size(); i++) {
    if (state == State::HOLD) {
      if (raw_bits[i] == 1) continue;
      state = State::PRMB_1;
      prev_bit_ = raw_bits[i];
      count_ = 0;
    }

    recover_bits(raw_bits[i]);

    if (state == State::PRMB_1 && bits_.size() >= 6) {
      state = check_preamble(0, 6, 20, 1) ? State::PRMB_2 : State::ERROR;
    } else if (state == State::PRMB_2 && bits_.size() >= 14) {
      state = check_preamble(6, 8, 85, 2) ? State::SYN_1 : State::ERROR;
    } else if (state == State::SYN_1 && bits_.size() >= 22) {
      state = check_preamble(14, 8, 104, 3) ? State::SYN_2 : State::ERROR;
    } else if (state == State::SYN_2 && bits_.size() >= 30) {
      state = check_preamble(22, 8, 104, 4) ? State::SOH : State::ERROR;
    } else if (state == State::SOH && bits_.size() >= 38) {
      state = check_preamble(30, 8, 129, 5) ? State::PAYLOAD : State::ERROR;
    }

    if (state == State::ERROR) {
      break;
    }
  }
  /* std::cout << std::endl << "вывод: " << std::endl;
  for (auto bit : bits_) std::cout << (bit ? 1 : 0) << " "; */

  return bits_;
}

std::vector<uint8_t> extract_raw_bits(
    size_t size, const std::vector<std::complex<float>>& filtered1,
    const std::vector<std::complex<float>>& filtered2,
    const float squelch_threshold) {
  std::vector<uint8_t> result;
  uint8_t temp = 1;
  result.reserve(size);

  std::vector<std::complex<float>> vector_1200;
  std::vector<std::complex<float>> vector_2400;

  for (size_t i = 0; i < size; i++) {
    float e1 = std::norm(filtered1[i]);
    float e2 = std::norm(filtered2[i]);

    vector_1200.emplace_back(e1);
    vector_2400.emplace_back(e2);

    if (e1 > squelch_threshold || e2 > squelch_threshold) {
      if (e1 > e2) {
        result.push_back(1);
        temp = 1;
      } else {
        result.push_back(0);
        temp = 0;
      }
    } else {
      result.push_back(temp);
    }
  }

  saveSignal(vector_1200, "tests/signal_data/test9_after_fir1.csv");
  saveSignal(vector_2400, "tests/signal_data/test9_after_fir2.csv");

  return result;
}

std::vector<std::complex<float>> downsample_signal(
    const std::vector<std::complex<float>>& input_signal, const int factor) {
  std::vector<std::complex<float>> result;

  if (input_signal.empty()) {
    std::cout << "Input signal is empty";
    return result;
  }
  size_t output_count = input_signal.size() / factor;
  result.reserve(output_count);
  for (size_t i = 0; i < input_signal.size(); i += factor) {
    result.push_back(input_signal[i]);
  }

  return result;
}

std::vector<float> load_coeffs_from_file(const std::string& filename) {
  std::vector<float> result;
  if (filename.empty()) {
    std::cout << "Filename is empty";
    return result;
  }
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cout << "File isn't open";
    return result;
  }
  float value;
  while (file >> value) {
    result.push_back(value);
  }

  return result;
}

std::vector<std::complex<float>> get_data_from_file(const std::string& filename,
                                                    size_t num_data) {
  std::vector<std::complex<float>> result;

  if (filename.empty()) {
    std::cout << "Filename is empty" << std::endl;
    return result;
  }
  SndfileHandle file(filename);
  if (file.error()) {
    std::cout << "Could not open file";
    return result;
  }
  std::vector<int16_t> temp_data(num_data);
  file.read(&temp_data[0], temp_data.size());
  size_t iq_size_count = num_data / 2;
  result.reserve(iq_size_count);

  for (size_t i = 0; i < num_data; i += 2) {
    float i_num = static_cast<float>(temp_data[i]) / 32768.0f;
    float q_num = static_cast<float>(temp_data[i + 1]) / 32768.0f;

    result.push_back(std::complex<float>(i_num, q_num));
  }

  return result;
}

std::vector<float> calculate_am_envelope(
    const std::vector<std::complex<float>>& in_data) {
  std::vector<float> result;
  if (in_data.empty()) {
    std::cout << "In data is empty";
    return result;
  }

  size_t data_size = in_data.size();
  result.reserve(data_size);
  for (size_t i = 0; i < data_size; i++) {
    result.push_back(std::abs(in_data[i]));
    // std::cout << std::abs(in_data[i]) << std::endl;
  }

  return result;
}

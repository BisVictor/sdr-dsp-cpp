#include "SDR_prog.h"

FirFilter::FirFilter(const std::vector<float>& coefficients)
    : coeffs_(coefficients),
      history_(coefficients.size(), std::complex<float>(0.0f, 0.0f)) {}

FirFilter::~FirFilter() {}

std::complex<float> FirFilter::process_sample(std::complex<float> sample) {
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

void AcarsParser::parse_bitstream(const std::vector<uint8_t>& raw_bits) {
  for (size_t i = 0; i < raw_bits.size(); i++) {
    sample_counter_++;

    // Синхронизация по фронту
    if (raw_bits[i] != last_raw_bit_) {
      sample_counter_ = 0;
      last_raw_bit_ = raw_bits[i];
    }

    // Стробирование строго в центре бита
    if (sample_counter_ == mid_sample_tick_) {
      // Декодер NRZI
      uint8_t nrzi_bit = (raw_bits[i] == last_stable_bit_) ? 1 : 0;
      last_stable_bit_ = raw_bits[i];

      // Коррекция базовой инверсии
      nrzi_bit = 1 - nrzi_bit;

      // Задвигаем бит в 24-битный регистр преамбулы
      shift_reg_ = ((shift_reg_ << 1) & 0xFFFFFF) + nrzi_bit;

      // Вырезаем 16 бит для проверки преамбулы
      uint32_t check_sync = shift_reg_ & 0xFFFF;

      // Проверяем ПРЯМУЮ преамбулу
      if (check_sync == 2694) {
        is_capturing_ = true;
        invert_payload_ = false;
        bit_counter_ = 0;
        current_byte_ = 0;
        std::cout << "\n[ACARS Packet Detected] -> ";
        continue;
      }
      // Проверяем ИНВЕРСНУЮ преамбулу (Автоподстройка под SDR)
      else if (check_sync == 62837) {
        is_capturing_ = true;
        invert_payload_ = true;  // Будем переворачивать биты текста
        bit_counter_ = 0;
        current_byte_ = 0;
        std::cout << "\n[ACARS Inverted Packet Detected] -> ";
        continue;
      }

      // Сборка байта из битов
      if (is_capturing_) {
        // Если преамбула зашла инверсной, переворачиваем бит на лету
        uint8_t final_bit = invert_payload_ ? (1 - nrzi_bit) : nrzi_bit;

        current_byte_ |= (final_bit << bit_counter_);
        bit_counter_++;

        if (bit_counter_ == 8) {
          uint8_t clean_ascii = current_byte_ & 0x7F;

          if (clean_ascii == 3) {  // Конец пакета (ETX)
            is_capturing_ = false;
            std::cout << " [End of Packet]" << std::endl;
          } else if (clean_ascii >= 32 && clean_ascii <= 126) {
            std::cout << static_cast<char>(clean_ascii);
            std::cout.flush();
          }

          bit_counter_ = 0;
          current_byte_ = 0;
        }
      }
    }

    if (sample_counter_ >= 40) {
      sample_counter_ = 0;
    }
  }
}

std::vector<uint8_t> extract_raw_bits(
    size_t size, const std::vector<std::complex<float>>& filtered1,
    const std::vector<std::complex<float>>& filtered2) {
  std::vector<uint8_t> result;
  result.reserve(size);
  const float squelch_threshold = 1e-11f;

  for (size_t i = 0; i < size; i++) {
    float temp1 = std::norm(filtered1[i]);
    float temp2 = std::norm(filtered2[i]);
    if ((temp1 + temp2) < squelch_threshold) {
      result.push_back(1);
    } else {
      if (temp1 > temp2) {
        result.push_back(1);
      } else {
        result.push_back(0);
      }
    }
  }

  return result;
}

std::vector<float> design_bandpass_fir(int num_taps, float sample_rate,
                                       float f_low, float f_high) {
  std::vector<float> coeffs(num_taps);

  float nyquist = sample_rate / 2.0f;
  float omega_low = M_PI * (f_low / nyquist);
  float omega_high = M_PI * (f_high / nyquist);

  int center = (num_taps - 1) / 2;

  for (int i = 0; i < num_taps; i++) {
    int n = i - center;

    if (n == 0) {
      coeffs[i] = (omega_high - omega_low) / M_PI;
    } else {
      coeffs[i] =
          (std::sin(omega_high * n) - std::sin(omega_low * n)) / (M_PI * n);
    }
    // наложение оконной функции Хемминга
    float window = 0.54f + 0.46f * std::cos((2.0f * M_PI * n) / (num_taps - 1));
    coeffs[i] *= window;
  }

  return coeffs;
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

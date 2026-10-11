#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <sndfile.hh>
#include <string>
#include <vector>

class FirFilter {
 private:
  std::vector<float> coeffs_;
  std::vector<std::complex<float>> history_;

 public:
  FirFilter(const std::vector<float>& coefficients);
  ~FirFilter();
  std::complex<float> process_sample(std::complex<float> sample);
};

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

class AcarsParser {
 private:
  size_t sample_counter_ = 0;
  uint32_t shift_reg_ = 0;
  uint8_t bit_counter_ = 0;
  uint8_t current_byte_ = 0;
  bool is_capturing_ = false;
  uint8_t last_raw_bit_ = 1;  // Начинаем с 1, так как в тишине летят единицы
  uint8_t last_stable_bit_ = 1;
  const int mid_sample_tick_ = 6;  // Центр бита при частоте 32 кГц (13.33 / 2)

 public:
  AcarsParser() {}

  void parse_bitstream(const std::vector<uint8_t>& raw_bits) {
    for (size_t i = 0; i < raw_bits.size(); i++) {
      sample_counter_++;

      // 1. Синхронизация по фронту (поиск смены 0->1 или 1->0)
      if (raw_bits[i] != last_raw_bit_) {
        sample_counter_ = 0;  // Сбрасываем фазу на границе бита
        last_raw_bit_ = raw_bits[i];
      }

      // 2. Стробирование (Делаем замер строго в центре бита)
      if (sample_counter_ == mid_sample_tick_) {
        // 3. Декодер NRZI
        uint8_t nrzi_bit = 0;
        if (raw_bits[i] == last_stable_bit_) {
          nrzi_bit = 1;  // Уровень не изменился -> логическая 1
        } else {
          nrzi_bit = 0;  // Уровень изменился -> логический 0
        }
        last_stable_bit_ = raw_bits[i];

        // 4. Коррекция аппаратной инверсии SDR#
        nrzi_bit = 1 - nrzi_bit;

        // 5. Задвигаем бит в 24-битный сдвиговый регистр преамбулы
        shift_reg_ = ((shift_reg_ << 1) & 0xFFFFFF) + nrzi_bit;

        // 6. Ищем маркер синхронизации пакета (два байта SYN подряд = 2694)
        uint32_t check_sync = shift_reg_ & 0xFFFF;
        if (check_sync == 2694) {
          is_capturing_ = true;
          bit_counter_ = 0;
          current_byte_ = 0;
          continue;  // Пропускаем сам префикс синхронизации
        }

        // 7. Сборка байта из битов (Младшим битом вперед - LSB)
        if (is_capturing_) {
          current_byte_ |= (nrzi_bit << bit_counter_);
          bit_counter_++;

          // Собрали полный символ (8 бит)!
          if (bit_counter_ == 8) {
            // Отрезаем верхний бит чётности ACARS, получая чистый 7-битный
            // ASCII
            uint8_t clean_ascii = current_byte_ & 0x7F;

            // Если символ управляющий конца текста (ETX = 0x03), закрываем
            // пакет
            if (clean_ascii == 3) {
              is_capturing_ = false;
              std::cout << std::endl << "[End of ACARS Packet]" << std::endl;
            }
            // Если символ читаемый — выводим на экран!
            else if (clean_ascii >= 32 && clean_ascii <= 126) {
              std::cout << static_cast<char>(clean_ascii);
              std::cout.flush();  // Принудительно выталкиваем букву на экран
            }

            // Сброс накопителей для следующего байта
            bit_counter_ = 0;
            current_byte_ = 0;
          }
        }
      }

      // Защита от долгой тишины
      if (sample_counter_ >= 40) {
        sample_counter_ = 0;
      }
    }
  }
};

std::vector<uint8_t> extract_raw_bits(
    size_t size, const std::vector<std::complex<float>>& filtered1,
    const std::vector<std::complex<float>>& filtered2) {
  std::vector<uint8_t> result;
  result.reserve(size);
  const float squelch_threshold = 0.0001f;

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

std::vector<std::complex<float>> get_data_from_file(const std::string filename,
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

// ----MAIN----
int main(void) {
  SndfileHandle file("17-35-54_131725000Hz.wav");

  // считаем количество семплов
  int channels = file.channels();
  sf_count_t frames = file.frames();
  sf_count_t total_samples = frames * channels;

  std::cout << "Всего значений в DATA : " << total_samples << " чисел"
            << std::endl;

  // открываем файл с записью IQ в vector
  std::vector<std::complex<float>> temp =
      get_data_from_file("17-35-54_131725000Hz.wav", total_samples);
  // std::vector<float> temp2 = calculate_am_envelope(temp);
  std::cout << "Успешно создано комплексных IQ-семплов: " << temp.size()
            << std::endl;

  // открываем файл с коэффициентами фильтра НЧ
  std::vector<float> my_coeffs = load_coeffs_from_file("filter.txt");

  FirFilter filter(my_coeffs);
  std::vector<std::complex<float>> temp2;
  temp2.reserve(temp.size());

  // фильтрация
  for (size_t i = 0; i < temp.size(); i++) {
    std::complex<float> result = filter.process_sample(temp[i]);
    temp2.push_back(result);
  }

  // Делаем прореживание сигнала в 32 раза
  std::vector<std::complex<float>> temp3;
  temp3 = downsample_signal(temp2, 32);

  // создаем коэффикциенты для полосового фильтра (bandpass)
  std::vector<float> coeffs_1200 =
      design_bandpass_fir(101, 32000.0f, 1000.0f, 1400.0f);

  // создаем НОВЫЙ, отдельный фильтр конкретно для аудио-тона 1200 Гц.
  //  назовем его filter_1200.
  FirFilter filter_1200(coeffs_1200);

  // создаем вектор, куда запишется результат аудио-фильтрации
  std::vector<std::complex<float>> temp_filtered_1200;
  temp_filtered_1200.reserve(temp3.size());

  // пропускаем наш легкий прореженный сигнал (temp3) через новый фильтр
  for (size_t i = 0; i < temp3.size(); i++) {
    std::complex<float> res = filter_1200.process_sample(temp3[i]);
    temp_filtered_1200.push_back(res);
  }

  std::vector<float> coeffs_2400 =
      design_bandpass_fir(101, 32000.0f, 2200.0f, 2600.0f);

  FirFilter filter_2400(coeffs_2400);

  std::vector<std::complex<float>> temp_filtered_2400;
  temp_filtered_2400.reserve(temp3.size());

  for (size_t i = 0; i < temp3.size(); i++) {
    std::complex<float> res = filter_2400.process_sample(temp3[i]);
    temp_filtered_2400.push_back(res);
  }

  std::vector<uint8_t> raw_bits;
  raw_bits.reserve(temp3.size());
  raw_bits =
      extract_raw_bits(temp3.size(), temp_filtered_1200, temp_filtered_2400);

  for (size_t i = 0; i < 1000; i++) {
    std::cout << static_cast<int>(raw_bits[i]);
  }
  std::cout << std::endl;

  return 0;
}
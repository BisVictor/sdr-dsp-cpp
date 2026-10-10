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

// MAIN

int main(void) {
  SndfileHandle file("17-35-54_131725000Hz.wav");

  int channels = file.channels();
  sf_count_t frames = file.frames();
  sf_count_t total_samples = frames * channels;

  std::cout << "Всего значений в DATA : " << total_samples << " чисел"
            << std::endl;

  std::vector<std::complex<float>> temp =
      get_data_from_file("17-35-54_131725000Hz.wav", 2000);
  // std::vector<float> temp2 = calculate_am_envelope(temp);
  std::cout << "Успешно создано комплексных IQ-семплов: " << temp.size()
            << std::endl;

  std::vector<float> my_coeffs = load_coeffs_from_file("filter.txt");
  FirFilter filter(my_coeffs);
  std::vector<std::complex<float>> temp2;
  temp2.reserve(1000);

  for (size_t i = 0; i < 1000; i++) {
    std::complex<float> result = filter.process_sample(temp[i]);
    temp2.push_back(result);
  }
  std::vector<std::complex<float>> temp3;
  temp3 = downsample_signal(temp2, 32);
  for (size_t i = 0; i < 100; i++) {
    std::cout << i << " " << temp3[i] << std::endl;
  }

  return 0;
}
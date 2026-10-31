#include "SDR_prog.h"
#include "acars_modulator.h"

// ---- MAIN ----
int main(void) {
  SndfileHandle file("IQ-files/17-35-54_131725000Hz.wav");

  // считаем количество семплов
  int channels = file.channels();
  sf_count_t frames = file.frames();
  sf_count_t total_samples = frames * channels;

  AcarsConfig cnfg;

  std::cout << "Всего значений в DATA : " << total_samples << " чисел"
            << std::endl;

  // открываем файл с записью IQ в vector
  std::vector<std::complex<float>> temp =
      get_data_from_file("IQ-files/17-35-54_131725000Hz.wav", total_samples);
  // std::vector<float> temp_env = calculate_am_envelope(temp);

  // saveSignal_float(temp_env, "test_env.csv");
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
  std::vector<std::complex<float>> temp3 = downsample_signal(temp2, 32);

  std::cout << "После прореживания: " << temp3.size() << std::endl;

  std::vector<float> temp_env = calculate_am_envelope(temp2);
  saveSignal_float(temp_env, "test_env.csv");

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

  AcarsParser parser;
  parser.parse_bitstream(raw_bits);

  return 0;
}
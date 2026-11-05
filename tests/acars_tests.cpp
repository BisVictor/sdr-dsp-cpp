#include <gtest/gtest.h>

#include "SDR_prog.h"
#include "acars_modulator.h"
#include "filter.h"

// g++ -I. tests/acars_tests.cpp SDR_prog.cpp acars_modulator.cpp -o run_tests
// -lgtest -lgtest_main -lpthread -lsndfile

// --- ТЕСТЫ ---

TEST(Acars_Test, Test1_GenerateBitLengthAndPhase) {
  AcarsConfig cfg;
  cfg.sampleRate = 48000.0;
  cfg.baudRate = 2400.0;

  // Вычисляем, сколько семплов должно быть в одном бите: 48000 / 2400 = 20
  // семплов
  size_t expected_samples = static_cast<size_t>(cfg.sampleRate / cfg.baudRate);

  double currentPhase = 0.0;
  bool test_bit = true;

  std::vector<double> signal = generateBit(test_bit, currentPhase, cfg);

  // размер вектора совпадает с расчетным?
  ASSERT_EQ(signal.size(), expected_samples);

  // фаза изменилась и не осталась нулевой?
  EXPECT_NE(currentPhase, 0.0);
}

TEST(Acars_Test, Test2_GenerateAcarsPacket) {
  AcarsConfig cfg;
  std::vector<bool> bits = {1, 1, 1, 0, 0, 0, 1, 1, 1};
  std::vector<double> test_vector = generateAcarsPacket(bits, cfg);
  saveSignal(test_vector, "tests/signal_data/test2.csv");
}

TEST(Acars_Test, Test3_downsample_signal) {
  AcarsConfig cfg;
  // cfg.sampleRate = 1024000;
  std::vector<bool> bits = {1, 1, 1, 0, 0, 0, 1, 1, 1};
  std::vector<double> test_vector = generateAcarsPacket(bits, cfg);
  saveSignal(test_vector, "tests/signal_data/test3_before.csv");

  std::vector<std::complex<float>> iq_signal;
  iq_signal.reserve(test_vector.size());
  for (double sample : test_vector) {
    iq_signal.emplace_back(static_cast<float>(sample), 0.0f);
  }
  std::vector<std::complex<float>> result = downsample_signal(iq_signal, 5);
  saveSignal(result, "tests/signal_data/test3_after.csv");
  // std::cout << iq_signal.size() << result.size();
  ASSERT_EQ(result.size(), (iq_signal.size() / 5));
}

TEST(Acars_Test, Test4_design_lowpass_fir) {
  AcarsConfig cfg;
  std::vector<float> test_vector;
  test_vector.reserve(101);
  float sample_rate = static_cast<float>(cfg.sampleRate);
  test_vector = design_lowpass_fir(101, sample_rate, 1200.0f);
  saveSignal(test_vector, "tests/signal_data/test4.csv");
  ASSERT_EQ(test_vector.size(), 101);
  EXPECT_NEAR(std::accumulate(test_vector.begin(), test_vector.end(), 0.0f),
              1.0f, 0.001f);
}

TEST(Acars_Test, Test5_lowpass_fir) {
  AcarsConfig cfg;
  std::vector<float> test_vector;
  test_vector.reserve(101);
  float sample_rate = static_cast<float>(cfg.sampleRate);
  test_vector = design_lowpass_fir(101, sample_rate, 1200.0f);

  std::vector<bool> bits = {1, 1, 1, 0, 0, 0, 1, 1, 1};
  std::vector<double> modulation = generateAcarsPacket(bits, cfg);
  std::vector<std::complex<float>> iq_signal;
  iq_signal.reserve(modulation.size());
  for (double sample : modulation) {
    iq_signal.emplace_back(static_cast<float>(sample), 0.0f);
  }
  saveSignal(iq_signal, "tests/signal_data/test5_before.csv");

  std::vector<std::complex<float>> vector_after_filtration;
  vector_after_filtration.reserve(iq_signal.size());
  FirFilter filter(test_vector);

  for (auto sample : iq_signal) {
    std::complex<float> temp = filter.process_sample(sample);
    vector_after_filtration.push_back(temp);
  }
  saveSignal(vector_after_filtration, "tests/signal_data/test5_after.csv");
  ASSERT_EQ(iq_signal.size(), vector_after_filtration.size());
}

TEST(Acars_Test, Test6_design_bandpass_fir) {
  AcarsConfig cfg;
  std::vector<float> coeffs_1200 =
      design_bandpass_fir(101, cfg.sampleRate, 1000.0f, 1400.0f);
  saveSignal(coeffs_1200, "tests/signal_data/test6.csv");
  ASSERT_EQ(coeffs_1200.size(), 101);
}

TEST(Acars_Test, Test7_bandpass_fir) {
  AcarsConfig cfg;
  cfg.sampleRate = 32000;
  std::vector<float> coeffs_1200 =
      design_bandpass_fir(7, cfg.sampleRate, 1000.0f, 1400.0f);

  std::vector<bool> bits = {1, 1, 1, 0, 0, 0, 1, 1, 1};
  std::vector<double> modulation = generateAcarsPacket(bits, cfg);
  std::vector<std::complex<float>> iq_signal;
  iq_signal.reserve(modulation.size());
  for (double sample : modulation) {
    iq_signal.emplace_back(static_cast<float>(sample), 0.0f);
  }
  saveSignal(iq_signal, "tests/signal_data/test7_before.csv");

  std::vector<std::complex<float>> vector_after_filtration;
  vector_after_filtration.reserve(iq_signal.size());
  FirFilter filter(coeffs_1200);

  for (auto sample : iq_signal) {
    std::complex<float> temp = filter.process_sample(sample);
    vector_after_filtration.push_back(temp);
  }
  saveSignal(vector_after_filtration, "tests/signal_data/test7_after.csv");
  ASSERT_EQ(iq_signal.size(), vector_after_filtration.size());
}
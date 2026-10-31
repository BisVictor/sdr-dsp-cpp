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
  saveSignal(test_vector, "test_vector.csv");
}

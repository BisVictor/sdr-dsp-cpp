CXX = g++
CXXFLAGS = -I. -Wall -Wextra

# Общие файлы проекта
COMMON_SRCS = SDR_prog.cpp acars_modulator.cpp filter.cpp

# Настройки для основной программы
MAIN_TARGET = app
MAIN_SRCS = main.cpp $(COMMON_SRCS)
MAIN_LDFLAGS = -lpthread -lsndfile

# Настройки для тестов
TEST_TARGET = run_tests
TEST_SRCS = tests/acars_tests.cpp $(COMMON_SRCS)
TEST_LDFLAGS = -lgtest -lgtest_main -lpthread -lsndfile

# Цель по умолчанию — сборка и запуск основной программы
all: run

# --- Основная программа ---
$(MAIN_TARGET): $(MAIN_SRCS)
	$(CXX) $(CXXFLAGS) $(MAIN_SRCS) -o $(MAIN_TARGET) $(MAIN_LDFLAGS)

run: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# --- Тесты ---
$(TEST_TARGET): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(TEST_LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# --- Очистка ---
clean:
	rm -f $(MAIN_TARGET) $(TEST_TARGET)

.PHONY: all run test clean

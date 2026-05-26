#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cuda_runtime.h>

using namespace std;

// Проверка ошибок CUDA
#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err = (call);                                              \
        if (err != cudaSuccess) {                                              \
            cerr << "CUDA error: " << cudaGetErrorString(err)                  \
                 << " (" << __FILE__ << ":" << __LINE__ << ")" << endl;       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

// Функция чтения матрицы из файла
vector<double> readMatrix(const string& filename, int size) {
    vector<double> matrix(static_cast<size_t>(size) * size);

    string fullPath = "matrix/" + filename;
    ifstream file(fullPath);

    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл: " + fullPath);
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file >> matrix[static_cast<size_t>(i) * size + j];
        }
    }
    file.close();
    return matrix;
}

// CUDA-ядро умножения матриц
__global__ void multiplyKernel(const double* A, const double* B, double* C, int n) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < n && col < n) {
        double sum = 0.0;
        for (int k = 0; k < n; ++k) {
            sum += A[row * n + k] * B[k * n + col];
        }
        C[row * n + col] = sum;
    }
}

// Запуск умножения на GPU с заданной конфигурацией блока
vector<double> multiplyMatricesCUDA(
    const vector<double>& A,
    const vector<double>& B,
    int size,
    int blockSize,
    double& kernelTimeMs) {

    vector<double> result(static_cast<size_t>(size) * size, 0.0);

    size_t bytes = static_cast<size_t>(size) * size * sizeof(double);

    double *dA = nullptr, *dB = nullptr, *dC = nullptr;
    CUDA_CHECK(cudaMalloc(&dA, bytes));
    CUDA_CHECK(cudaMalloc(&dB, bytes));
    CUDA_CHECK(cudaMalloc(&dC, bytes));

    CUDA_CHECK(cudaMemcpy(dA, A.data(), bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, B.data(), bytes, cudaMemcpyHostToDevice));

    dim3 block(blockSize, blockSize);
    dim3 grid((size + blockSize - 1) / blockSize,
              (size + blockSize - 1) / blockSize);

    cudaEvent_t startEv, stopEv;
    CUDA_CHECK(cudaEventCreate(&startEv));
    CUDA_CHECK(cudaEventCreate(&stopEv));

    CUDA_CHECK(cudaEventRecord(startEv));
    multiplyKernel<<<grid, block>>>(dA, dB, dC, size);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaEventRecord(stopEv));
    CUDA_CHECK(cudaEventSynchronize(stopEv));

    float elapsed = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&elapsed, startEv, stopEv));
    kernelTimeMs = static_cast<double>(elapsed);

    CUDA_CHECK(cudaMemcpy(result.data(), dC, bytes, cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaEventDestroy(startEv));
    CUDA_CHECK(cudaEventDestroy(stopEv));
    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    return result;
}

// Функция записи результирующей матрицы
void saveMatrix(const vector<double>& matrix, const string& filename, int size) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath);

    if (!file.is_open()) {
        throw runtime_error("Не удалось создать файл: " + fullPath);
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << static_cast<int>(matrix[static_cast<size_t>(i) * size + j]);
            if (j < size - 1) file << " ";
        }
        file << "\n";
    }
    file.close();
}

// Функция сохранения статистики
void saveStatistics(const string& filename, int size, double time_ms,
                    int blockSize, int gridSize) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath, ios::app);

    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл статистики");
    }

    file << "Размер матрицы: " << size << "x" << size << "\n";
    file << "Конфигурация блока: " << blockSize << "x" << blockSize << "\n";
    file << "Конфигурация сетки: " << gridSize << "x" << gridSize << "\n";
    file << "Время выполнения ядра: " << fixed << setprecision(3) << time_ms << " мс\n";
    file << "\n";
    file.close();
}

void printHeader() {
    cout << "\n" << string(70, '-') << "\n";
    cout << left << setw(10) << "Размер "
         << setw(14) << "    Блок     "
         << setw(14) << "   Сетка    "
         << setw(16) << "  Время (мс) "
         << setw(13) << " Ускорение "
         << "\n";
    cout << string(70, '-') << "\n";
}

// Вывод строки результатов
void printResult(int size, int blockSize, int gridSize, double time, double baseTime) {
    double speedup = (baseTime > 0) ? baseTime / time : 1.0;

    string blockStr = to_string(blockSize) + "x" + to_string(blockSize);
    string gridStr  = to_string(gridSize)  + "x" + to_string(gridSize);

    cout << left << setw(10) << size
         << setw(14) << blockStr
         << setw(14) << gridStr
         << setw(16) << fixed << setprecision(3) << time
         << setw(13) << fixed << setprecision(2) << speedup
         << "\n";
}

int main() {
    system("chcp 65001");

    vector<int> sizes = {200, 400, 800, 1200, 1600, 2000};
    vector<int> blockSizes = {8, 16, 32};

    // Информация о GPU
    int device = 0;
    cudaDeviceProp props{};
    CUDA_CHECK(cudaGetDevice(&device));
    CUDA_CHECK(cudaGetDeviceProperties(&props, device));
    cout << "CUDA устройство: " << props.name << "\n";
    cout << "Compute Capability: " << props.major << "." << props.minor << "\n";

    printHeader();

    for (int size : sizes) {
        double baseTime = 0.0;
        bool resultSaved = false;

        for (int blockSize : blockSizes) {
            try {
                string fileA = "first_" + to_string(size) + ".txt";
                string fileB = "second_" + to_string(size) + ".txt";
                string resultFile = "result_" + to_string(size) + ".txt";

                auto matrixA = readMatrix(fileA, size);
                auto matrixB = readMatrix(fileB, size);

                double kernelTime = 0.0;
                auto result = multiplyMatricesCUDA(matrixA, matrixB, size,
                                                   blockSize, kernelTime);

                int gridSize = (size + blockSize - 1) / blockSize;

                // Сохраняем результат только при первом запуске для этого размера
                if (!resultSaved) {
                    saveMatrix(result, resultFile, size);
                    resultSaved = true;
                }

                saveStatistics("statistics_cuda.txt", size, kernelTime,
                               blockSize, gridSize);

                if (blockSize == blockSizes.front()) {
                    baseTime = kernelTime;
                }

                printResult(size, blockSize, gridSize, kernelTime, baseTime);

                matrixA.clear();
                matrixB.clear();
                result.clear();

            } catch (const exception& e) {
                cerr << "Ошибка: " << e.what() << endl;
            }
        }
    }

    CUDA_CHECK(cudaDeviceReset());
    return 0;
}

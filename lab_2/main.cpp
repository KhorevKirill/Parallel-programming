#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <omp.h>
#include <algorithm>
#include <cstdlib>

using namespace std;

// Функция чтения матрицы из файла
vector<vector<double>> readMatrix(const string& filename, int size) {
    vector<vector<double>> matrix(size, vector<double>(size));
    
    string fullPath = "matrix/" + filename;
    ifstream file(fullPath);
    
    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл: " + fullPath);
    }
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file >> matrix[i][j];
        }
    }
    file.close();
    return matrix;
}

// Функция умножения матриц с OpenMP
vector<vector<double>> multiplyMatricesOMP_Safe(
    const vector<vector<double>>& A,
    const vector<vector<double>>& B,
    int size,
    int numThreads) {
    
    vector<vector<double>> result(size, vector<double>(size, 0.0));
    
    #pragma omp parallel for num_threads(numThreads) schedule(dynamic, 8)
    for (int i = 0; i < size; ++i) {
        for (int k = 0; k < size; ++k) {
            double a_ik = A[i][k];
            if (a_ik != 0) {
                for (int j = 0; j < size; ++j) {
                    result[i][j] += a_ik * B[k][j];
                }
            }
        }
    }
    return result;
}

// Функция записи результирующей матрицы
void saveMatrix(const vector<vector<double>>& matrix, 
                const string& filename, int size) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath);
    
    if (!file.is_open()) {
        throw runtime_error("Не удалось создать файл: " + fullPath);
    }
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << static_cast<int>(matrix[i][j]);
            if (j < size - 1) file << " ";
        }
        file << "\n";
    }
    file.close();
}

// Функция сохранения статистики
void saveStatistics(const string& filename, int size, double time_ms, int numThreads) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath, ios::app);
    
    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл статистики");
    }


    
    file << "Размер матрицы: " << size << "x" << size << "\n";
    file << "Количество потоков: " << numThreads << "\n";
    file << "Время выполнения: " << fixed << setprecision(3) << time_ms << " мс\n";
    file << "\n";
    file.close();
}

void printHeader() {
    cout << "\n" << string(60, '-') << "\n";
    cout << left << setw(10) << "Размер " 
         << setw(12) << "       Потоки     " 
         << setw(15) << "         Время (мс) " 
         << setw(13) << " Ускорение " 
         << "\n";
    cout << string(60, '-') << "\n";
}

// Вывод строки результатов
void printResult(int size, int threads, double time, double baseTime) {
    double speedup = (baseTime > 0) ? baseTime / time : 1.0;

    
    cout << left << setw(10) << size 
         << setw(12) << threads 
         << setw(15) << fixed << setprecision(3) << time 
         << setw(13) << fixed << setprecision(2) << speedup
         << "\n";
}

int main() {
    system("chcp 65001");
    
    vector<int> sizes = {200, 400, 800, 1200, 1600, 2000};
    vector<int> threadCounts = {1, 2, 4, 8};
    
    int maxCores = omp_get_num_procs();

    printHeader();
    
    for (int size : sizes) {
    double baseTime = 0.0;
    bool resultSaved = false; 
    
        for (int numThreads : threadCounts) {
            int actualThreads = min(numThreads, maxCores);
            
            try {
                cout << size << "x" << size 
                    << "    " << actualThreads;
                
                string fileA = "first_" + to_string(size) + ".txt";
                string fileB = "second_" + to_string(size) + ".txt";
                
                // Сохраняем результат только при первом запуске для этого размера
                string resultFile = "result_" + to_string(size) + ".txt";
                
                auto matrixA = readMatrix(fileA, size);
                auto matrixB = readMatrix(fileB, size);
                
                omp_set_num_threads(actualThreads);
                auto startMult = chrono::high_resolution_clock::now();                
                auto result = multiplyMatricesOMP_Safe(matrixA, matrixB, size, actualThreads);               
                auto endMult = chrono::high_resolution_clock::now();

                double multiplicationTime = chrono::duration<double, milli>(endMult - startMult).count();
                
                if (!resultSaved) {
                    saveMatrix(result, resultFile, size);
                    resultSaved = true;
                }
                
                // Сохраняем статистику 
                saveStatistics("statistics_omp.txt", size, multiplicationTime, actualThreads);
                
                if (actualThreads == 1) {
                    baseTime = multiplicationTime;
                }
                
                printResult(size, actualThreads, multiplicationTime, baseTime);
                
                matrixA.clear();
                matrixB.clear();
                result.clear();
                
            } catch (const exception& e) {
                cerr << "Ошибка: " << e.what() << endl;
            }
        }
    }
}
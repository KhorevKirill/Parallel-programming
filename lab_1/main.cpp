#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
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

// Функция умножения матриц
vector<vector<double>> multiplyMatrices(
    const vector<vector<double>>& A,
    const vector<vector<double>>& B,
    int size) {
    
    vector<vector<double>> result(size, vector<double>(size, 0.0));
    
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
void saveStatistics(const string& filename, int size, double time_ms) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath, ios::app);
    
    if (!file.is_open()) {
        throw runtime_error("Не удалось открыть файл статистики");
    }
    
    long long operations = static_cast<long long>(size) * size * size;
    
    file << "Размер матрицы: " << size << "x" << size << "\n";
    file << "Время выполнения: " << fixed << setprecision(3) << time_ms << " мс\n";
    file << "Объём задачи (операций): " << operations << "\n";
    file << "\n";
    file << "\n";
    file.close();
}

int main() {
    system("chcp 65001");
    
    vector<int> sizes = {200, 400, 800, 1200, 1600, 2000};
    
    for (int size : sizes) {
        try {
            cout << "Обработка матриц размера " << size << "x" << size << "..." << endl;
            
            string fileA = "first_" + to_string(size) + ".txt";
            string fileB = "second_" + to_string(size) + ".txt";
            string resultFile = "result_" + to_string(size) + ".txt";
            
            // Чтение матриц
            auto matrixA = readMatrix(fileA, size);
            auto matrixB = readMatrix(fileB, size);
            
            // Умножение матриц
            auto startMult = chrono::high_resolution_clock::now();
            auto result = multiplyMatrices(matrixA, matrixB, size);
            auto endMult = chrono::high_resolution_clock::now();
            
            double multiplicationTime = chrono::duration<double, milli>(
                endMult - startMult).count();
            
            cout << "Время умножения: " << multiplicationTime << " мс" << endl;
            
            saveMatrix(result, resultFile, size);
            saveStatistics("statistics.txt", size, multiplicationTime);
            
            // Освобождение памяти
            matrixA.clear();
            matrixB.clear();
            result.clear();
            
        } catch (const exception& e) {
            cerr << "Ошибка при обработке размера " << size << ": " << e.what() << endl << endl;
        }
    }
    
    return 0;
}
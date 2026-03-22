#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include <string>
#include <array>

using namespace std;

const array<int, 6> SIZES = {200, 400, 800, 1200, 1600, 2000};

// Функция для создания матрицы
vector<vector<int>> createMatrix(int size, mt19937& gen) {
    uniform_int_distribution<int> dist(0, 99);
    vector<vector<int>> matrix(size, vector<int>(size));
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            matrix[i][j] = dist(gen);
        }
    }
    
    return matrix;
}

// Функция для записи матрицы в файл
void saveMatrixToFile(const vector<vector<int>>& matrix, const string& filename) {
    ofstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Ошибка открытия файла: " << filename << endl;
        return;
    }
    
    int size = matrix.size();
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << setw(4) << matrix[i][j];
            if (j < size - 1) file << " ";
        }
        file << endl;
    }
    
    file.close();
}

int main() {
    setlocale(LC_ALL, "RUS");

    random_device rd;
    mt19937 gen(rd());
    
    // Генерация матриц
    for (int size : SIZES) {
        
        vector<vector<int>> matrix1 = createMatrix(size, gen);
        string filename1 = "first_" + to_string(size) + ".txt";
        saveMatrixToFile(matrix1, filename1);
        cout << "Сохранено:" << filename1 << endl;
        
        vector<vector<int>> matrix2 = createMatrix(size, gen);
        string filename2 = "second_" + to_string(size) + ".txt";
        saveMatrixToFile(matrix2, filename2);
        cout << "Сохранено:" << filename2 << endl;
        
        cout << endl;
    }
    
    cout << "Все матрицы созданы" << endl;
    
    return 0;
}
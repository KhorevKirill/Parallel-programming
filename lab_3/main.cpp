#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <numeric>
#include <mpi.h>

using namespace std;

// Функция чтения матрицы из файла
vector<double> readMatrix(const string& filename, int size) {
    vector<double> matrix(static_cast<size_t>(size) * size);

    string fullPath = "matrix/" + filename;
    ifstream file(fullPath);

    if (!file.is_open()) {
        cerr << "Не удалось открыть файл: " << fullPath << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file >> matrix[static_cast<size_t>(i) * size + j];
        }
    }
    file.close();
    return matrix;
}

// Функция записи результирующей матрицы
void saveMatrix(const vector<double>& matrix, const string& filename, int size) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath);

    if (!file.is_open()) {
        cerr << "Не удалось создать файл: " << fullPath << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << static_cast<long long>(matrix[static_cast<size_t>(i) * size + j]);
            if (j < size - 1) file << " ";
        }
        file << "\n";
    }
    file.close();
}

// Функция сохранения статистики
void saveStatistics(const string& filename, int size, double time_ms, int numProcs) {
    string fullPath = "result/" + filename;
    ofstream file(fullPath, ios::app);

    if (!file.is_open()) {
        cerr << "Не удалось открыть файл статистики" << endl;
        return;
    }

    file << "Размер матрицы: " << size << "x" << size << "\n";
    file << "Количество процессов: " << numProcs << "\n";
    file << "Время выполнения: " << fixed << setprecision(3) << time_ms << " мс\n";
    file << "\n";
    file.close();
}

// Распределение строк между процессами
void buildCounts(int n, int processes, vector<int>& counts, vector<int>& displs) {
    counts.assign(processes, 0);
    displs.assign(processes, 0);

    int baseRows = n / processes;
    int extraRows = n % processes;
    int offset = 0;

    for (int rank = 0; rank < processes; ++rank) {
        int rows = baseRows + (rank < extraRows ? 1 : 0);
        counts[rank] = rows * n;
        displs[rank] = offset;
        offset += counts[rank];
    }
}

// Умножение части строк A на всю B
void multiplyLocal(const vector<double>& localA,
                   const vector<double>& B,
                   vector<double>& localC,
                   int localRows,
                   int n) {
    for (int i = 0; i < localRows; ++i) {
        for (int k = 0; k < n; ++k) {
            double a = localA[static_cast<size_t>(i) * n + k];
            if (a != 0) {
                for (int j = 0; j < n; ++j) {
                    localC[static_cast<size_t>(i) * n + j] += a * B[static_cast<size_t>(k) * n + j];
                }
            }
        }
    }
}

void printHeader() {
    cout << "\n" << string(60, '-') << "\n";
    cout << left << setw(10) << "Размер "
         << setw(14) << "  Процессов "
         << setw(15) << "  Время (мс) "
         << setw(13) << "  Ускорение "
         << "\n";
    cout << string(60, '-') << "\n";
}

// Вывод строки результатов
void printResult(int size, int processes, double time, double baseTime) {
    double speedup = (baseTime > 0) ? baseTime / time : 1.0;

    cout << left << setw(10) << size
         << setw(14) << processes
         << setw(15) << fixed << setprecision(3) << time
         << setw(13) << fixed << setprecision(2) << speedup
         << "\n";
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, processes = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);

    if (rank == 0) {
        system("chcp 65001 > nul");
    }

    vector<int> sizes = {200, 400, 800, 1200, 1600, 2000};

    if (rank == 0) {
        printHeader();
    }

    // baseTime читается из файла statistics_mpi.txt косвенно — через переменную окружения
    // baseTime сохраняется в этом процессе только для текущего запуска (1 размер -> 1 значение)
    // Если запуск идёт с одним числом процессов, ускорение всегда = 1.
    double baseTime = 0.0;
    const char* baseEnv = getenv("LAB3_BASE_TIME_MS");

    for (int size : sizes) {
        int n = size;

        string fileA = "first_" + to_string(size) + ".txt";
        string fileB = "second_" + to_string(size) + ".txt";
        string resultFile = "result_" + to_string(size) + ".txt";

        vector<double> AFlat;
        vector<double> BFlat(static_cast<size_t>(n) * n);

        if (rank == 0) {
            AFlat = readMatrix(fileA, n);
            BFlat = readMatrix(fileB, n);
        }

        MPI_Bcast(BFlat.data(), n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        vector<int> counts, displs;
        buildCounts(n, processes, counts, displs);

        int localCount = counts[rank];
        int localRows = localCount / n;
        vector<double> localA(localCount);
        vector<double> localC(localCount, 0.0);

        MPI_Barrier(MPI_COMM_WORLD);
        double start = MPI_Wtime();

        MPI_Scatterv(rank == 0 ? AFlat.data() : nullptr,
                     counts.data(), displs.data(), MPI_DOUBLE,
                     localA.data(), localCount, MPI_DOUBLE,
                     0, MPI_COMM_WORLD);

        multiplyLocal(localA, BFlat, localC, localRows, n);

        vector<double> CFlat;
        if (rank == 0) {
            CFlat.assign(static_cast<size_t>(n) * n, 0.0);
        }

        MPI_Gatherv(localC.data(), localCount, MPI_DOUBLE,
                    rank == 0 ? CFlat.data() : nullptr,
                    counts.data(), displs.data(), MPI_DOUBLE,
                    0, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);
        double end = MPI_Wtime();
        double timeMs = (end - start) * 1000.0;

        if (rank == 0) {
            // Сохраняем результат и статистику только из процесса 0
            saveMatrix(CFlat, resultFile, n);
            saveStatistics("statistics_mpi.txt", n, timeMs, processes);

            if (processes == 1 && size == sizes.front()) {
                baseTime = timeMs;
            }
            if (baseEnv && baseTime == 0.0) {
                baseTime = atof(baseEnv);
            }

            printResult(size, processes, timeMs, baseTime);
        }
    }

    MPI_Finalize();
    return 0;
}

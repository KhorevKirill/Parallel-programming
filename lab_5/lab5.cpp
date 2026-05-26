#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>

// Распределение строк матрицы A между процессами:
// sendcounts[p] = число элементов (rows*N) для процесса p,
// displs[p]     = смещение в исходном буфере.
void buildCounts(int N, int processes,
                 std::vector<int>& sendcounts,
                 std::vector<int>& displs) {
    sendcounts.assign(processes, 0);
    displs.assign(processes, 0);

    int rowsPerProc = N / processes;
    int remainder   = N % processes;
    int offset      = 0;

    for (int p = 0; p < processes; ++p) {
        int rows = rowsPerProc + (p < remainder ? 1 : 0);
        sendcounts[p] = rows * N;
        displs[p]     = offset;
        offset       += sendcounts[p];
    }
}

// Локальное умножение part(A) * B -> part(C), порядок i-k-j ради кеша.
void multiplyLocal(const std::vector<long long>& local_A,
                   const std::vector<long long>& B,
                   std::vector<long long>& local_C,
                   int local_rows, int N) {
    for (int i = 0; i < local_rows; ++i) {
        for (int k = 0; k < N; ++k) {
            long long a = local_A[i * N + k];
            if (a != 0) {
                for (int j = 0; j < N; ++j) {
                    local_C[i * N + j] += a * B[k * N + j];
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<int> sizes = {200, 400, 800, 1200, 1600, 2000};

    for (int N : sizes) {
        std::vector<long long> A, B, C;

        // Процесс 0 генерирует исходные матрицы случайными значениями 0..99
        if (rank == 0) {
            A.resize(static_cast<size_t>(N) * N);
            B.resize(static_cast<size_t>(N) * N);
            C.assign(static_cast<size_t>(N) * N, 0);

            std::srand(static_cast<unsigned>(std::time(nullptr)) + N);
            for (int i = 0; i < N * N; ++i) {
                A[i] = std::rand() % 100;
                B[i] = std::rand() % 100;
            }
        }

        // Рассылаем B всем процессам целиком
        if (rank != 0) B.resize(static_cast<size_t>(N) * N);
        MPI_Bcast(B.data(), N * N, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

        // Считаем разбиение строк A
        std::vector<int> sendcounts, displs;
        buildCounts(N, size, sendcounts, displs);

        int local_rows = sendcounts[rank] / N;
        std::vector<long long> local_A(sendcounts[rank]);
        std::vector<long long> local_C(sendcounts[rank], 0);

        // Раздаём строки A
        MPI_Scatterv(
            rank == 0 ? A.data() : nullptr,
            sendcounts.data(), displs.data(),
            MPI_LONG_LONG,
            local_A.data(), sendcounts[rank], MPI_LONG_LONG,
            0, MPI_COMM_WORLD);

        // Замер времени локального умножения
        MPI_Barrier(MPI_COMM_WORLD);
        double start_time = MPI_Wtime();

        multiplyLocal(local_A, B, local_C, local_rows, N);

        double local_time = MPI_Wtime() - start_time;
        double max_time = 0.0;
        MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        // Собираем результат на процессе 0
        MPI_Gatherv(
            local_C.data(), sendcounts[rank], MPI_LONG_LONG,
            rank == 0 ? C.data() : nullptr,
            sendcounts.data(), displs.data(),
            MPI_LONG_LONG,
            0, MPI_COMM_WORLD);

        if (rank == 0) {
            std::cout << "N=" << N
                      << " processes=" << size
                      << " time=" << std::fixed << std::setprecision(7)
                      << max_time
                      << " seconds" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}

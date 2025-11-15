#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstdio>

using namespace std;

bool is_prime(int n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (int i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

int main() {
    const int MAXN = 10000;//limita superioară
    const int PROC = 10;//numărul de procese copil
    const int CHUNK = MAXN / PROC;  // câte numere primește fiecare proces - 1000

    int parent_to_child[PROC][2];//parinte-copil
    int child_to_parent[PROC][2];//copil-parinte

    // 1. Creăm pipe-urile
    for (int i = 0; i < PROC; i++) {
        if (pipe(parent_to_child[i]) < 0 || pipe(child_to_parent[i]) < 0) {
            perror("pipe");
            return 1;
        }
    }

    // 2. Creăm cele 10 procese copil
    for (int i = 0; i < PROC; i++) {
        pid_t pid = fork();

	//eroare
        if (pid < 0) {
            perror("fork");
            return 1;
        }

	//ne aflăm în procesul copil
        if (pid == 0) {
               // Închidem pipe-urile care NU sunt ale acestui copil
            for (int j = 0; j < PROC; j++) {
                if (j != i) {
                    close(parent_to_child[j][0]);
                    close(parent_to_child[j][1]);
                    close(child_to_parent[j][0]);
                    close(child_to_parent[j][1]);
                }
            }

            // Copilul citește intervalul primit
            close(parent_to_child[i][1]);   // copilul doar citește
            close(child_to_parent[i][0]);   // copilul doar scrie

            FILE* in  = fdopen(parent_to_child[i][0], "r");
            FILE* out = fdopen(child_to_parent[i][1], "w");

            int start, end;
            fscanf(in, "%d %d", &start, &end);

            // Copilul calculează numere prime și le trimite înapoi
            for (int x = start; x <= end; x++) {
                if (is_prime(x)) {
                    fprintf(out, "%d\n", x);
                }
            }

            fclose(in);
            fclose(out);

            _exit(0); // copilul se oprește aici
        }
    }

    // 3.Trimite intervalele
    for (int i = 0; i < PROC; i++) {
        close(parent_to_child[i][0]); // părintele doar scrie

        FILE* out = fdopen(parent_to_child[i][1], "w");

        int start = i * CHUNK;
        int end   = (i + 1) * CHUNK - 1;
        if (i == PROC - 1) end = MAXN;

        fprintf(out, "%d %d\n", start, end);
        fclose(out); // Închidem ca să primească copilul EOF
    }

    // 4. Părintele citește rezultatele de la copii
    for (int i = 0; i < PROC; i++) {
        close(child_to_parent[i][1]); // părintele doar citește
        FILE* in = fdopen(child_to_parent[i][0], "r");

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), in)) {
            cout << "Child " << i << " prime: " << buffer;
        }
        fclose(in);
    }

    // 5. Așteptăm toți copiii
    for (int i = 0; i < PROC; i++)
        wait(NULL);

    return 0;
}

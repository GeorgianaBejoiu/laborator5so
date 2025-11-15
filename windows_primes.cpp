#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

bool isPrime(int n) {
    if (n < 2) return false;
    for (int i = 2; i*i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int main() {
    // Citim intervalul de la procesul părinte prin stdin (pipe)
    int start, end;
    std::cin >> start >> end;

    std::vector<int> primes;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) primes.push_back(i);
    }

    // Trimitem rezultatele prin stdout (pipe)
    for (int p : primes) {
        std::cout << p << " ";
    }
    std::cout.flush();

    return 0;
}

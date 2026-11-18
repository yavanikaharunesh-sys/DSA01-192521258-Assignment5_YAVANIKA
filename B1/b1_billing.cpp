// B1.1 - one bill of each kind with each payer, checked against hand-calculated values
#include "billing.h"
int main() {
    int failed = runBaseTests();
    cout << (failed == 0 ? "All 12 tests PASS\n" : "Some tests FAILED\n");
    return failed;
}

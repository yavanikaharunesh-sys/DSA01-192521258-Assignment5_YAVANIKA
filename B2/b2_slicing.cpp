// throw;  rethrows the ORIGINAL object.   throw e;  throws a COPY of the static type of e.
#include <iostream>
#include <stdexcept>
using namespace std;
struct ClaimError : runtime_error { using runtime_error::runtime_error; };

void withRethrow() { try { throw ClaimError("exceeds cover"); } catch (const exception& e) { throw;   } }
void withCopy()    { try { throw ClaimError("exceeds cover"); } catch (const exception& e) { throw e; } }

int main() {
    try { withRethrow(); }
    catch (const ClaimError& e)  { cout << "throw;   -> caught as ClaimError, what() = " << e.what() << "\n"; }
    catch (const exception& e)   { cout << "throw;   -> caught only as std::exception\n"; }
    try { withCopy(); }
    catch (const ClaimError& e)  { cout << "throw e; -> caught as ClaimError, what() = " << e.what() << "\n"; }
    catch (const exception& e)   { cout << "throw e; -> caught only as std::exception, what() = " << e.what() << "   (sliced!)\n"; }
}

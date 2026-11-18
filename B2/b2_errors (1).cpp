// B2 - deliberately create every error and show that the program keeps running and objects are destroyed
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
using namespace std;

class BillingError : public runtime_error { public: using runtime_error::runtime_error; };
class NegativeAmountError : public BillingError { public: using BillingError::BillingError; };
class ClaimError : public BillingError { public: using BillingError::BillingError; };
class UnknownPackageError : public BillingError { public: using BillingError::BillingError; };

struct Tracker {                                    // prints when created / destroyed (shows stack unwinding)
    string n;
    explicit Tracker(const string& s) : n(s) { cout << "    + " << n << " created\n"; }
    ~Tracker() { cout << "    ~ " << n << " destroyed\n"; }
};

void checkCharge(double x)        { Tracker t("charge check");  if (x < 0) throw NegativeAmountError("negative charge"); }
void checkClaim(double b, double c){ Tracker t("claim check");  if (b > c) throw ClaimError("claim exceeds cover"); }
void checkPackage(const string& code) {
    Tracker t("package lookup");
    static map<string, double> rates = {{"OP", 1000}, {"IP", 30000}};
    if (!rates.count(code)) throw UnknownPackageError("unknown package " + code);
}
void parseNumber(const string& s) { Tracker t("number parse"); stod(s); }          // throws invalid_argument
void hugeArray()                  { Tracker t("huge array");   double* p = new double[1ull << 50]; delete[] p; }
void throwInt()                   { Tracker t("int thrower");  throw 7; }

int main() {
    struct Test { const char* name; void (*run)(); };
    Test tests[] = {
        {"negative charge (-500)",         [] { checkCharge(-500); }},
        {"claim 60000 above cover 50000",  [] { checkClaim(60000, 50000); }},
        {"unknown package code XYZ",       [] { checkPackage("XYZ"); }},
        {"text 'abc' where number expected", [] { parseNumber("abc"); }},
#ifndef NO_HUGE   // AddressSanitizer aborts on a huge new instead of throwing, so this test is skipped in the ASan run
        {"impossible 8 PB allocation",     hugeArray},
#endif
        {"unexpected type (int)",          throwInt},
    };
    Tracker top("main-level object");
    for (const Test& t : tests) {
        cout << "TEST: " << t.name << "\n";
        try { t.run(); }
        catch (const BillingError& e)     { cout << "  caught BillingError: " << e.what() << "\n"; }
        catch (const invalid_argument& e) { cout << "  caught invalid_argument: " << e.what() << "\n"; }
        catch (const bad_alloc& e)        { cout << "  caught bad_alloc: " << e.what() << "\n"; }
        catch (...)                       { cout << "  top-level catch(...) caught an unknown exception\n"; }
        cout << "  -> program still running\n";
    }
    cout << "All tests finished\n";
}

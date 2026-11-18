// Task B4 (all parts in one program)
// Tests + sanitizer: g++ -std=c++17 -Wall -Wextra -O2 -fsanitize=address -g B4.cpp -o B4 && ./B4
// Timing (B4.3)    : g++ -std=c++17 -Wall -Wextra -O2 B4.cpp -o B4 && ./B4
//  PART 1 = EXISTING classes from B1 (not edited)   PART 2 = NEW bill type + NEW payer (new code only)
//  B4.1 extension + earlier tests | B4.2 exceptions under AddressSanitizer | B4.3 alternative: rules from a config file
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// ======================= PART 1 : EXISTING CODE (same as B1, NOT edited) =======================


// ---------- exceptions (A4) ----------
class BillingError : public runtime_error { public: using runtime_error::runtime_error; };
class NegativeAmountError : public BillingError { public: using BillingError::BillingError; };
class ClaimError : public BillingError { public: using BillingError::BillingError; };
class UnknownPackageError : public BillingError { public: using BillingError::BillingError; };

inline double nonNeg(double x) { if (x < 0) throw NegativeAmountError("negative charge"); return x; }

// ---------- bills (A1) ----------
class Bill {                                         // abstract base class
protected:
    string patient;
public:
    explicit Bill(const string& p) : patient(p) {}
    virtual ~Bill() = default;                       // virtual destructor
    virtual double total() const = 0;                // pure virtual
    virtual string type() const { return "Bill"; }   // virtual
    void print() const { cout << type() << ' ' << patient << ": " << total() << '\n'; }
};
class OutPatientBill : public Bill {
    double consultation, tests;
public:
    OutPatientBill(const string& p, double c, double t) : Bill(p), consultation(nonNeg(c)), tests(nonNeg(t)) {}
    double total() const override { return consultation + tests; }
    string type() const override { return "OP"; }
};
class InPatientBill : public Bill {
    int days; double roomRate, procedures, medicines;
public:
    InPatientBill(const string& p, int d, double r, double pr, double m)
        : Bill(p), days(d), roomRate(nonNeg(r)), procedures(nonNeg(pr)), medicines(nonNeg(m)) { nonNeg(d); }
    double total() const override { return days * roomRate + procedures + medicines; }
    string type() const override { return "IP"; }
};
class DayCareBill : public Bill {
    double packagePrice;
public:
    DayCareBill(const string& p, double pk) : Bill(p), packagePrice(nonNeg(pk)) {}
    double total() const override { return packagePrice; }
    string type() const override { return "DC"; }
};
class EmergencyBill : public Bill {
    double base, hours, ratePerHour;
public:
    EmergencyBill(const string& p, double b, double h, double r)
        : Bill(p), base(nonNeg(b)), hours(nonNeg(h)), ratePerHour(nonNeg(r)) {}
    double total() const override { return base + hours * ratePerHour; }
    string type() const override { return "ER"; }
};

// ---------- payers (A3) ----------
class Payer {                                        // abstract base class
public:
    virtual ~Payer() = default;
    virtual double amountCovered(const Bill& b) const = 0;   // pure virtual
    virtual string name() const = 0;
};
class SelfPay : public Payer {
public:
    double amountCovered(const Bill& b) const override { return b.total(); }
    string name() const override { return "SelfPay"; }
};
class Insurance : public Payer {
    double coverLimit, coPay;                        // coPay in percent paid by patient
public:
    Insurance(double limit, double copay) : coverLimit(limit), coPay(copay) {}
    double amountCovered(const Bill& b) const override { return min(b.total(), coverLimit) * (100 - coPay) / 100; }
    string name() const override { return "Insurance"; }
};
class GovernmentScheme : public Payer {
    map<string, double> rates;                       // fixed package rate for each bill type
public:
    explicit GovernmentScheme(const map<string, double>& r) : rates(r) {}
    double amountCovered(const Bill& b) const override {
        auto it = rates.find(b.type());
        if (it == rates.end()) throw UnknownPackageError("no package for " + b.type());
        return min(b.total(), it->second);
    }
    string name() const override { return "Government"; }
};

// a claim larger than the payer will cover is rejected
inline double approveClaim(const Bill& b, const Payer& p, double claimed) {
    if (claimed > p.amountCovered(b)) throw ClaimError("claim exceeds cover");
    return claimed;
}

// ---------- earlier tests: 4 bills x 3 payers, checked against hand-calculated values ----------
inline int runBaseTests() {
    OutPatientBill op("Ravi", 500, 1200);            // 1700
    InPatientBill  ip("Meena", 3, 2500, 40000, 2500);// 7500 + 40000 + 2500 = 50000
    DayCareBill    dc("Arun", 30000);                // 30000
    EmergencyBill  er("Kavya", 2000, 4, 1500);       // 2000 + 6000 = 8000
    const Bill* bills[4] = { &op, &ip, &dc, &er };
    SelfPay self; Insurance ins(50000, 10);
    GovernmentScheme gov({{"OP", 1000}, {"IP", 30000}, {"DC", 25000}, {"ER", 5000}});
    const Payer* payers[3] = { &self, &ins, &gov };
    double expected[4][3] = { {1700, 1530, 1000}, {50000, 45000, 30000},
                              {30000, 27000, 25000}, {8000, 7200, 5000} };
    int failed = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j) {
            double got = payers[j]->amountCovered(*bills[i]);
            bool ok = got == expected[i][j];
            if (!ok) ++failed;
            cout << bills[i]->type() << " total=" << bills[i]->total() << "  " << payers[j]->name()
                 << " covers " << got << "  expected " << expected[i][j] << (ok ? "  PASS\n" : "  FAIL\n");
        }
    return failed;
}

// ======================= PART 2 : NEW CODE (the extension) =======================
class TeleConsultBill : public Bill {                // new bill type
    double minutes, perMinute, platformFee;
public:
    TeleConsultBill(const string& p, double m, double r, double f)
        : Bill(p), minutes(nonNeg(m)), perMinute(nonNeg(r)), platformFee(nonNeg(f)) {}
    double total() const override { return minutes * perMinute + platformFee; }
    string type() const override { return "TC"; }
};
class CorporatePayer : public Payer {                // new payer
    double limit, sharePercent;
public:
    CorporatePayer(double l, double s) : limit(l), sharePercent(s) {}
    double amountCovered(const Bill& b) const override { return min(b.total(), limit) * sharePercent / 100; }
    string name() const override { return "Corporate"; }
};

// ======================= B4.3 alternative: bill types described in a configuration file =======================
struct Rule { double units, perUnit, fixedFee; };      // one generic rule: fixed + units * perUnit
struct DemoBill { virtual ~DemoBill() = default; virtual double total() const = 0; };   // class way, for timing
struct DemoTC : DemoBill { double total() const override { return 30 * 20 + 100; } };

// ======================= main =======================
int main() {
    cout << "===== B4.1 earlier tests (existing classes, unchanged) =====\n";
    int failed = runBaseTests();
    cout << (failed == 0 ? "Earlier tests: all 12 PASS\n" : "Earlier tests: FAILED\n");

    cout << "\n===== B4.1 new bill type + new payer =====\n";
    TeleConsultBill tc("Divya", 30, 20, 100);                       // 30*20 + 100 = 700
    SelfPay self; Insurance ins(50000, 10); CorporatePayer corp(5000, 80);
    GovernmentScheme gov({{"OP", 1000}, {"IP", 30000}, {"DC", 25000}, {"ER", 5000}, {"TC", 500}});
    const Payer* payers[4] = { &self, &ins, &gov, &corp };
    double expected[4] = { 700, 630, 500, 560 };                    // hand-calculated
    for (int j = 0; j < 4; ++j) {
        double got = payers[j]->amountCovered(tc);
        cout << "TC total=" << tc.total() << "  " << payers[j]->name() << " covers " << got
             << "  expected " << expected[j] << (got == expected[j] ? "  PASS\n" : "  FAIL\n");
    }
    Bill* all[3] = { new OutPatientBill("Ravi", 500, 1200), new TeleConsultBill("Divya", 30, 20, 100),
                     new EmergencyBill("Kavya", 2000, 4, 1500) };
    for (Bill* b : all) b->print();                                  // one loop, all kinds

    cout << "\n===== B4.2 thrown and caught exceptions (run under AddressSanitizer) =====\n";
    try { OutPatientBill bad("Test", -500, 100); }
    catch (const BillingError& e) { cout << "caught: " << e.what() << "\n"; }
    try { approveClaim(*all[0], ins, 5000); }
    catch (const BillingError& e) { cout << "caught: " << e.what() << "\n"; }
    try { GovernmentScheme noTC({{"OP", 1000}}); noTC.amountCovered(tc); }
    catch (const BillingError& e) { cout << "caught: " << e.what() << "\n"; }
    try { stod("abc"); }
    catch (const exception& e) { cout << "caught: invalid number (" << e.what() << ")\n"; }
    for (Bill* b : all) delete b;
    cout << "(AddressSanitizer prints a report here if there is any leak or memory error - nothing below means clean)\n";

    cout << "\n===== B4.3 alternative: rules read from a configuration text =====\n";
    istringstream config("# type units perUnit fixedFee\nTC 30 20 100\nLAB 5 300 50\n");   // same as a rates.txt file
    unordered_map<string, Rule> rules; string line;
    while (getline(config, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line); string type; Rule r;
        if (ss >> type >> r.units >> r.perUnit >> r.fixedFee) rules[type] = r;
    }
    for (auto& [type, r] : rules) cout << type << " -> " << r.fixedFee + r.units * r.perUnit << "\n";
#ifdef __SANITIZE_ADDRESS__
    cout << "timing skipped under AddressSanitizer (it slows string work); compile WITHOUT -fsanitize=address to time it\n";
#else
    const int N = 10000000;
    vector<string> names; for (int i = 0; i < 1000; ++i) names.push_back(i % 2 ? "TC" : "LAB");
    vector<unique_ptr<DemoBill>> objs; for (int i = 0; i < 1000; ++i) objs.push_back(make_unique<DemoTC>());
    auto t1 = chrono::steady_clock::now();
    double s1 = 0; for (int i = 0; i < N; ++i) { const Rule& r = rules.find(names[i % 1000])->second; s1 += r.fixedFee + r.units * r.perUnit; }
    auto t2 = chrono::steady_clock::now();
    double s2 = 0; for (int i = 0; i < N; ++i) s2 += objs[i % 1000]->total();
    auto t3 = chrono::steady_clock::now();
    cout << N << " bills, rule interpretation (name lookup): " << chrono::duration<double, milli>(t2 - t1).count() << " ms\n";
    cout << N << " bills, virtual call                     : " << chrono::duration<double, milli>(t3 - t2).count() << " ms\n";
    cout << "(check values " << s1 << " / " << s2 << ")\n";
#endif
    return failed;
}

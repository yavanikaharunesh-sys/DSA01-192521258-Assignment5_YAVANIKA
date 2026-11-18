// Task B3 (all parts in one program) - compile with: g++ -std=c++17 -Wall -Wextra -O2 b3_all.cpp -o b3_all
//  B3.1 correctness | B3.2 speed (virtual vs switch vs variant) | B3.3 cost of exceptions
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <variant>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;
double msSince(Clock::time_point t) { return chrono::duration<double, milli>(Clock::now() - t).count(); }

struct Spec { int type; double a, b, c; };           // 0=OP 1=IP 2=DC 3=ER  (also the "switch" version of a bill)

// ---- version (a): virtual functions ----
struct Bill { virtual ~Bill() = default; virtual double total() const = 0; };
struct OP : Bill { double c, t;    OP(double c, double t) : c(c), t(t) {}                       double total() const override { return c + t; } };
struct IP : Bill { double d, r, x; IP(double d, double r, double x) : d(d), r(r), x(x) {}       double total() const override { return d * r + x; } };
struct DC : Bill { double p;       explicit DC(double p) : p(p) {}                              double total() const override { return p; } };
struct ER : Bill { double b, h, r; ER(double b, double h, double r) : b(b), h(h), r(r) {}       double total() const override { return b + h * r; } };
unique_ptr<Bill> make(const Spec& s) {
    switch (s.type) {
        case 0:  return make_unique<OP>(s.a, s.b);
        case 1:  return make_unique<IP>(s.a, s.b, s.c);
        case 2:  return make_unique<DC>(s.a);
        default: return make_unique<ER>(s.a, s.b, s.c);
    }
}
// ---- version (b): one struct + switch ----
double totalSwitch(const Spec& s) {
    switch (s.type) { case 0: return s.a + s.b; case 1: return s.a * s.b + s.c; case 2: return s.a; default: return s.a + s.b * s.c; }
}
// ---- version (c): std::variant ----
struct OPv { double c, t;    double total() const { return c + t; } };
struct IPv { double d, r, x; double total() const { return d * r + x; } };
struct DCv { double p;       double total() const { return p; } };
struct ERv { double b, h, r; double total() const { return b + h * r; } };
using VBill = variant<OPv, IPv, DCv, ERv>;
VBill makeV(const Spec& s) {
    switch (s.type) { case 0: return OPv{s.a, s.b}; case 1: return IPv{s.a, s.b, s.c}; case 2: return DCv{s.a}; default: return ERv{s.a, s.b, s.c}; }
}

// ---- error reporting styles for B3.3 ----
enum Status { OK, NEGATIVE };
Status validateRC(double x, double& out) { if (x < 0) return NEGATIVE; out = x; return OK; }
double validateEx(double x)              { if (x < 0) throw invalid_argument("negative charge"); return x; }

void part1() {
    cout << "===== B3.1 correctness: 10 bills, virtual vs switch =====\n";
    Spec specs[10] = { {0,500,1200,0}, {1,3,2500,40000}, {2,30000,0,0}, {3,2000,4,1500}, {0,300,0,0},
                       {1,5,2000,15000}, {2,25000,0,0}, {3,1500,2,1000}, {0,700,2500,0}, {1,1,3000,0} };
    double sumV = 0, sumS = 0; bool agree = true;
    for (int i = 0; i < 10; ++i) {
        double v = make(specs[i])->total(), s = totalSwitch(specs[i]);
        cout << "bill " << i + 1 << ": virtual = " << v << "  switch = " << s << (v == s ? "  same\n" : "  DIFFERENT\n");
        sumV += v; sumS += s; agree = agree && (v == s);
    }
    cout << "Day total: virtual = " << sumV << ", switch = " << sumS << (agree ? "  -> both agree\n" : "  -> MISMATCH\n");
}

void part2() {
    cout << "\n===== B3.2 speed: ms per pass over n bills (-O2, random bill types) =====\n";
    cout << "sizeof: switch struct = " << sizeof(Spec) << ", variant = " << sizeof(VBill) << ", virtual OP object = " << sizeof(OP) << " (+8-byte pointer)\n";
    cout << "         n     virtual      switch     variant   check\n";
    for (long n : {1000L, 1000000L, 10000000L}) {
        mt19937 rng(1);
        vector<Spec> specs(n);
        for (Spec& s : specs) s = { int(rng() % 4), double(rng() % 100 + 1), double(rng() % 10 + 1), double(rng() % 1000) };
        long reps = 10000000 / n;                              // same total work for every n
        double sv = 0, ss = 0, sx = 0, tv, ts, tx;
        {   vector<unique_ptr<Bill>> v; v.reserve(n);
            for (const Spec& s : specs) v.push_back(make(s));
            auto t0 = Clock::now();
            for (long r = 0; r < reps; ++r) { sv = 0; for (const auto& b : v) sv += b->total(); }
            tv = msSince(t0) / reps; }
        {   auto t0 = Clock::now();
            for (long r = 0; r < reps; ++r) { ss = 0; for (const Spec& s : specs) ss += totalSwitch(s); }
            ts = msSince(t0) / reps; }
        {   vector<VBill> v; v.reserve(n);
            for (const Spec& s : specs) v.push_back(makeV(s));
            auto t0 = Clock::now();
            for (long r = 0; r < reps; ++r) { sx = 0; for (const auto& b : v) sx += visit([](const auto& x) { return x.total(); }, b); }
            tx = msSince(t0) / reps; }
        cout.width(10); cout << n; cout.width(12); cout << tv; cout.width(12); cout << ts; cout.width(12); cout << tx
             << "   " << (sv == ss && ss == sx ? "match" : "MISMATCH") << "\n";
    }
}

void part3() {
    cout << "\n===== B3.3 cost of exceptions: validate 1,000,000 bills (ms) =====\n";
    cout << "invalid%   return codes   exceptions   exceptions are slower by\n";
    const int N = 1000000;
    mt19937 rng(1);
    for (int pct : {0, 1, 50}) {
        vector<double> bills(N);
        for (double& b : bills) b = (int(rng() % 100) < pct) ? -1.0 : 100.0;
        auto t1 = Clock::now();
        double s1 = 0; int bad1 = 0;
        for (double b : bills) { double v; if (validateRC(b, v) == OK) s1 += v; else ++bad1; }
        double rc = msSince(t1);
        auto t2 = Clock::now();
        double s2 = 0; int bad2 = 0;
        for (double b : bills) { try { s2 += validateEx(b); } catch (const invalid_argument&) { ++bad2; } }
        double ex = msSince(t2);
        cout.width(5); cout << pct << "%"; cout.width(15); cout << rc; cout.width(13); cout << ex
             << "      x" << (rc > 0 ? ex / rc : 0) << "   (rejected " << bad1 << "/" << bad2 << ")\n";
    }
}

int main() { part1(); part2(); part3(); }

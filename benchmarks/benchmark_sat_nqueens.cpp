/***************************************************************************
 *            benchmark_sat_nqueens.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne SAT.
 *
 *  Ariadne SAT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne SAT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne SAT.  If not, see <https://www.gnu.org/licenses/>.
 */

/*! \file benchmark_sat_nqueens.cpp
 *  \brief Comparative N-queens SAT benchmark for BDD and CDCL.
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "logging/logging.hpp"
#include "logging/logging_command_line_interface.hpp"
#include "sat/sat_solver.hpp"
#include "utility/command_line_interface.hpp"

using namespace Ariadne;

namespace {

class BenchmarkThreadRegistry final : public ThreadRegistryInterface {
  public:
    Bool has_threads_registered() const override { return false; }
};

Void initialize_logging()
{
    static BenchmarkThreadRegistry registry;
    Logger& logger=Logger::instance();
    if(not logger.has_thread_registry_attached()) {
        logger.attach_thread_registry(&registry);
    }
    logger.configuration().set_verbosity(1u);
}

unsigned parse_unsigned(String const& value)
{
    const std::string input(value);
    if(input.empty() or input.front()=='-') {
        throw std::invalid_argument("expected a nonnegative integer");
    }
    std::size_t used=0u;
    const unsigned long long parsed=std::stoull(input,&used);
    if(used!=input.size() or parsed>std::numeric_limits<unsigned>::max()) {
        throw std::invalid_argument("integer outside unsigned range");
    }
    return static_cast<unsigned>(parsed);
}

enum class BackendSelection { BOTH, BDD, CDCL };

struct Selection {
    unsigned n=4u;
    unsigned samples=5u;
    unsigned warmups=1u;
    BackendSelection backend=BackendSelection::BOTH;
};

List<ArgumentParser> benchmark_parsers(Selection& s)
{
    List<ArgumentParser> parsers;
    parsers.append(make_valued_argument_parser(
        "N","n","Chessboard size (positive, default: 4)",
        [&s](String const& text)->VoidFunction {
            const unsigned value=parse_unsigned(text);
            if(value==0u or
               static_cast<SizeType>(value)>
                   static_cast<SizeType>(std::numeric_limits<Int>::max())/
                   static_cast<SizeType>(value)) {
                throw std::invalid_argument("n is zero or exceeds Boolean variable range");
            }
            return [&s,value] { s.n=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "n","samples","Odd number of samples between 3 and 101 (default: 5)",
        [&s](String const& text)->VoidFunction {
            const unsigned value=parse_unsigned(text);
            if(value<3u or value>101u or value%2u==0u) {
                throw std::invalid_argument("samples must be odd, between 3 and 101");
            }
            return [&s,value] { s.samples=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "w","warmups","Number of unmeasured warm-up rounds (default: 1)",
        [&s](String const& text)->VoidFunction {
            const unsigned value=parse_unsigned(text);
            if(value>20u) throw std::invalid_argument("warmups must not exceed 20");
            return [&s,value] { s.warmups=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "b","backend","Backend selection [ both | bdd | cdcl ]",
        [&s](String const& text)->VoidFunction {
            BackendSelection value;
            if(text=="both") value=BackendSelection::BOTH;
            else if(text=="bdd") value=BackendSelection::BDD;
            else if(text=="cdcl") value=BackendSelection::CDCL;
            else throw std::invalid_argument("backend must be both, bdd or cdcl");
            return [&s,value] { s.backend=value; };
        }));
    return parsers;
}

Bool requests_help(int argc,char const* argv[])
{
    for(int i=1;i<argc;++i) {
        if(std::string(argv[i])=="-h" or std::string(argv[i])=="--help") return true;
    }
    return false;
}

SizeType square(SizeType n,SizeType row,SizeType column)
{
    return row*n+column;
}

// Exactly one queen per row and per column (pairwise at-most-one).
// All diagonally attacking square pairs are forbidden. The CNF is built
// once per board size, outside all timed benchmark regions.
std::vector<SatClause> make_problem(SizeType n)
{
    std::vector<SatClause> clauses;
    for(SizeType row=0u;row<n;++row) {
        SatClause at_least_one;
        at_least_one.reserve(n);
        for(SizeType col=0u;col<n;++col) {
            at_least_one.emplace_back(square(n,row,col));
            for(SizeType other=col+1u;other<n;++other) {
                clauses.push_back({
                    SatLiteral(square(n,row,col),false),
                    SatLiteral(square(n,row,other),false)
                });
            }
        }
        clauses.push_back(std::move(at_least_one));
    }
    for(SizeType col=0u;col<n;++col) {
        SatClause at_least_one;
        at_least_one.reserve(n);
        for(SizeType row=0u;row<n;++row) {
            at_least_one.emplace_back(square(n,row,col));
            for(SizeType other=row+1u;other<n;++other) {
                clauses.push_back({
                    SatLiteral(square(n,row,col),false),
                    SatLiteral(square(n,other,col),false)
                });
            }
        }
        clauses.push_back(std::move(at_least_one));
    }
    for(SizeType row=0u;row<n;++row) {
        for(SizeType col=0u;col<n;++col) {
            for(SizeType distance=1u;distance<n-row;++distance) {
                if(distance<n-col) {
                    clauses.push_back({
                        SatLiteral(square(n,row,col),false),
                        SatLiteral(square(n,row+distance,col+distance),false)
                    });
                }
                if(distance<=col) {
                    clauses.push_back({
                        SatLiteral(square(n,row,col),false),
                        SatLiteral(square(n,row+distance,col-distance),false)
                    });
                }
            }
        }
    }
    return clauses;
}

Bool satisfies(std::vector<SatClause> const& clauses,BinaryWord const& model)
{
    for(SatClause const& clause:clauses) {
        Bool satisfied=false;
        for(SatLiteral const& literal:clause) {
            if(model[literal.variable()]==literal.positive()) {
                satisfied=true;
                break;
            }
        }
        if(not satisfied) return false;
    }
    return true;
}

// Independently validate the N-queens chessboard, not just the CNF.
Bool valid_queens(SizeType n,BinaryWord const& model)
{
    if(model.size()!=n*n) return false;
    std::vector<SizeType> columns(n,0u);
    std::vector<SizeType> queen_columns(n,n);
    for(SizeType row=0u;row<n;++row) {
        SizeType row_count=0u;
        for(SizeType col=0u;col<n;++col) {
            if(model[square(n,row,col)]) {
                ++row_count;
                ++columns[col];
                queen_columns[row]=col;
            }
        }
        if(row_count!=1u) return false;
    }
    for(SizeType col=0u;col<n;++col) {
        if(columns[col]!=1u) return false;
    }
    for(SizeType row=0u;row<n;++row) {
        for(SizeType other=row+1u;other<n;++other) {
            SizeType first=queen_columns[row],second=queen_columns[other];
            SizeType diff=first<second ? second-first : first-second;
            if(diff==other-row) return false;
        }
    }
    return true;
}

using Clock=std::chrono::steady_clock;
double elapsed_ms(Clock::time_point begin,Clock::time_point end)
{
    return std::chrono::duration<double,std::milli>(end-begin).count();
}

struct Trial {
    double build_ms;
    double solve_ms;
    double total_ms;
    SizeType bdd_nodes;
};

Trial measure(SatBackendKind backend,SizeType n,
              std::vector<SatClause> const& clauses,Bool expected_sat)
{
    auto begin=Clock::now();
    SatSolver solver(n*n,backend);
    for(SatClause const& clause:clauses) {
        solver.add_clause(clause);
    }
    auto built=Clock::now();
    SatResult result=solver.solve();
    auto finished=Clock::now();

    // The oracle and model checks run outside the measured interval.
    if(result.is_sat()!=expected_sat) {
        throw std::runtime_error("Incorrect N-queens SAT/UNSAT answer");
    }
    if(result.is_sat() and
       (not satisfies(clauses,result.model()) or
        not valid_queens(n,result.model()))) {
        throw std::runtime_error("N-queens witness violates CNF or chessboard constraints");
    }
    if(solver.clause_count()!=clauses.size()) {
        throw std::runtime_error("Solver modified N-queens input clause count");
    }
    const SizeType nodes=backend==SatBackendKind::BDD
        ? solver.bdd_node_count() : 0u;
    return {elapsed_ms(begin,built),elapsed_ms(built,finished),
            elapsed_ms(begin,finished),nodes};
}

double median(std::vector<double> values)
{
    if(values.empty() or values.size()%2u==0u) {
        throw std::logic_error("Median requires an odd number of samples");
    }
    std::sort(values.begin(),values.end());
    return values[values.size()/2u];
}

struct Summary {
    double build_ms;
    double solve_ms;
    double total_ms;
    SizeType bdd_nodes;
};

Summary summarize(std::vector<Trial> const& trials)
{
    std::vector<double> build,solve,total;
    build.reserve(trials.size());
    solve.reserve(trials.size());
    total.reserve(trials.size());
    for(Trial const& t:trials) {
        build.push_back(t.build_ms);
        solve.push_back(t.solve_ms);
        total.push_back(t.total_ms);
    }
    return {median(std::move(build)),median(std::move(solve)),
            median(std::move(total)),trials.back().bdd_nodes};
}

Void print_summary(SizeType n,SizeType clauses,Bool sat,
                   SatBackendKind backend,Summary const& s)
{
    LOGGING_PRINTLN(std::left
        <<std::setw(6)<<n
        <<std::setw(9)<<(sat ? "SAT" : "UNSAT")
        <<std::setw(10)<<(backend==SatBackendKind::BDD ? "BDD" : "CDCL")
        <<std::setw(9)<<n*n
        <<std::setw(11)<<clauses
        <<std::right<<std::fixed<<std::setprecision(6)
        <<std::setw(14)<<s.build_ms
        <<std::setw(14)<<s.solve_ms
        <<std::setw(14)<<s.total_ms
        <<std::setw(13)<<(backend==SatBackendKind::BDD
            ? std::to_string(s.bdd_nodes) : "-"))
}

Void benchmark(Selection const& selection)
{
    SizeType n=selection.n;
    const std::vector<SatClause> clauses=make_problem(n);
    const Bool expected_sat=(n==1u or n>=4u);
    std::vector<Trial> bdd_trials,cdcl_trials;
    bdd_trials.reserve(selection.samples);
    cdcl_trials.reserve(selection.samples);

    const unsigned rounds=selection.warmups+selection.samples;
    for(unsigned sample=0u;sample<rounds;++sample) {
        // Alternate run order to mitigate drift / warm-cache bias.
        std::array<SatBackendKind,2> order=sample%2u==0u
            ? std::array<SatBackendKind,2>{SatBackendKind::BDD,SatBackendKind::CDCL}
            : std::array<SatBackendKind,2>{SatBackendKind::CDCL,SatBackendKind::BDD};
        for(SatBackendKind backend:order) {
            if((selection.backend==BackendSelection::BDD and backend!=SatBackendKind::BDD)
                or (selection.backend==BackendSelection::CDCL and backend!=SatBackendKind::CDCL)) {
                continue;
            }
            Trial trial=measure(backend,n,clauses,expected_sat);
            if(sample>=selection.warmups) {
                if(backend==SatBackendKind::BDD) bdd_trials.push_back(trial);
                else cdcl_trials.push_back(trial);
            }
        }
    }

    if(not bdd_trials.empty()) {
        print_summary(n,clauses.size(),expected_sat,
                      SatBackendKind::BDD,summarize(bdd_trials));
    }
    if(not cdcl_trials.empty()) {
        print_summary(n,clauses.size(),expected_sat,
                      SatBackendKind::CDCL,summarize(cdcl_trials));
    }
    if(not bdd_trials.empty() and not cdcl_trials.empty()) {
        const double bdd_total=summarize(bdd_trials).total_ms;
        const double cdcl_total=summarize(cdcl_trials).total_ms;
        if(not std::isfinite(bdd_total) or not std::isfinite(cdcl_total)
           or cdcl_total<=0.0) {
            throw std::runtime_error("Invalid N-queens comparative timing");
        }
        LOGGING_PRINTLN("  BDD/CDCL total time (ratio >1 favors CDCL): "
            <<std::fixed<<std::setprecision(3)<<bdd_total/cdcl_total<<"x")
    }
}

} // namespace

int main(int argc,char const* argv[])
{
    initialize_logging();
    try {
        Selection selection;
        LoggingCommandLineInterface cli;
        cli.add(CommandLineContribution(
            "sat-nqueens-benchmark",benchmark_parsers(selection)));
        if(not cli.acquire(argc,argv)) {
            return requests_help(argc,argv) ? 0 : 1;
        }

        LOGGING_PRINTLN("N-queens benchmark | N="<<selection.n
            <<" | samples="<<selection.samples
            <<" | warmups="<<selection.warmups)
        LOGGING_PRINTLN("Build: initialize solver + insert same CNF clauses; "
            "solve: one SAT/UNSAT search; medians in milliseconds")
        LOGGING_PRINTLN(std::left
            <<std::setw(6)<<"N"
            <<std::setw(9)<<"Result"
            <<std::setw(10)<<"Backend"
            <<std::setw(9)<<"Vars"
            <<std::setw(11)<<"Clauses"
            <<std::right<<std::setw(14)<<"Build (ms)"
            <<std::setw(14)<<"Solve (ms)"
            <<std::setw(14)<<"Total (ms)"
            <<std::setw(13)<<"BDD nodes")
        LOGGING_PRINTLN(std::string(100u,'-'))
        benchmark(selection);
        return 0;
    } catch(std::exception const& error) {
        LOGGING_PRINTLN("N-queens benchmark: "<<error.what())
        return 1;
    }
}

/***************************************************************************
 *            benchmark_sat_pigeonhole.cpp
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

/*! \file benchmark_sat_pigeonhole.cpp
 *  \brief Pigeonhole principle benchmark for the SAT solver.
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
    unsigned min_holes=1u;
    unsigned max_holes=3u;
    unsigned samples=5u;
    unsigned warmups=1u;
    BackendSelection backend=BackendSelection::BOTH;
};

List<ArgumentParser> benchmark_parsers(Selection& s)
{
    List<ArgumentParser> parsers;
    parsers.append(make_valued_argument_parser(
        "m","min-holes","Minimum positive hole count (default: 1)",
        [&s](String const& text)->VoidFunction {
            unsigned value=parse_unsigned(text);
            if(value==0u) throw std::invalid_argument("min-holes must be positive");
            return [&s,value] { s.min_holes=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "M","max-holes","Maximum hole count (default: 3)",
        [&s](String const& text)->VoidFunction {
            unsigned value=parse_unsigned(text);
            if(value==0u or value==std::numeric_limits<unsigned>::max()) {
                throw std::invalid_argument("max-holes outside supported range");
            }
            return [&s,value] { s.max_holes=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "n","samples","Odd sample count between 3 and 101 (default: 5)",
        [&s](String const& text)->VoidFunction {
            unsigned value=parse_unsigned(text);
            if(value<3u or value>101u or value%2u==0u) {
                throw std::invalid_argument("samples must be odd, between 3 and 101");
            }
            return [&s,value] { s.samples=value; };
        }));
    parsers.append(make_valued_argument_parser(
        "w","warmups","Number of unmeasured warm-up rounds (default: 1)",
        [&s](String const& text)->VoidFunction {
            unsigned value=parse_unsigned(text);
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

SizeType index(SizeType holes,SizeType pigeon,SizeType hole)
{
    return pigeon*holes+hole;
}

// Generate once per case. The vector is shared by both backends and is never
// included in the timed region.
std::vector<SatClause> make_problem(SizeType pigeons,SizeType holes)
{
    if(holes==0u or pigeons==0u or
       holes>std::numeric_limits<SizeType>::max()/pigeons) {
        throw std::out_of_range("pigeonhole variable count overflow");
    }
    std::vector<SatClause> clauses;
    for(SizeType pigeon=0u;pigeon<pigeons;++pigeon) {
        SatClause at_least_one;
        for(SizeType hole=0u;hole<holes;++hole) {
            at_least_one.emplace_back(index(holes,pigeon,hole));
            for(SizeType other=hole+1u;other<holes;++other) {
                clauses.push_back({SatLiteral(index(holes,pigeon,hole),false),
                                   SatLiteral(index(holes,pigeon,other),false)});
            }
        }
        clauses.push_back(std::move(at_least_one));
    }
    for(SizeType hole=0u;hole<holes;++hole) {
        for(SizeType p=0u;p<pigeons;++p) {
            for(SizeType q=p+1u;q<pigeons;++q) {
                clauses.push_back({SatLiteral(index(holes,p,hole),false),
                                   SatLiteral(index(holes,q,hole),false)});
            }
        }
    }
    return clauses;
}

Bool satisfies(std::vector<SatClause> const& clauses,BinaryWord const& model)
{
    for(SatClause const& clause:clauses) {
        Bool valid=false;
        for(SatLiteral const& literal:clause) {
            if(model[literal.variable()]==literal.positive()) {
                valid=true;
                break;
            }
        }
        if(not valid) return false;
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

Trial measure(SatBackendKind kind,SizeType variables,
              std::vector<SatClause> const& clauses,Bool expected_sat)
{
    auto start=Clock::now();
    SatSolver solver(variables,kind);
    for(SatClause const& clause:clauses) solver.add_clause(clause);
    auto built=Clock::now();
    SatResult result=solver.solve();
    auto finished=Clock::now();

    // Correctness checks are deliberately outside the measured intervals.
    if(result.is_sat()!=expected_sat) {
        throw std::runtime_error("Incorrect pigeonhole SAT/UNSAT answer");
    }
    if(result.is_sat() and
       (result.model().size()!=variables or not satisfies(clauses,result.model()))) {
        throw std::runtime_error("Pigeonhole SAT witness does not satisfy CNF");
    }
    if(solver.clause_count()!=clauses.size()) {
        throw std::runtime_error("SAT backend changed input clause count");
    }
    SizeType nodes=kind==SatBackendKind::BDD ? solver.bdd_node_count() : 0u;
    return {elapsed_ms(start,built),elapsed_ms(built,finished),
            elapsed_ms(start,finished),nodes};
}

double median(std::vector<double> values)
{
    if(values.empty() or values.size()%2u==0u)
        throw std::logic_error("Expected an odd number of measured samples");
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

Void print_result(SizeType holes,SizeType pigeons,Bool sat,SizeType clause_count,
                  SatBackendKind kind,Summary const& s)
{
    LOGGING_PRINTLN(std::left<<std::setw(7)<<holes<<std::setw(9)<<pigeons
        <<std::setw(8)<<(sat ? "SAT" : "UNSAT")
        <<std::setw(9)<<(kind==SatBackendKind::BDD ? "BDD" : "CDCL")
        <<std::setw(9)<<clause_count
        <<std::right<<std::fixed<<std::setprecision(6)
        <<std::setw(14)<<s.build_ms<<std::setw(14)<<s.solve_ms
        <<std::setw(14)<<s.total_ms
        <<std::setw(13)<<(kind==SatBackendKind::BDD
            ? std::to_string(s.bdd_nodes) : "-"))
}

Void run_case(SizeType holes,SizeType pigeons,Selection const& selection)
{
    if(pigeons>std::numeric_limits<SizeType>::max()/holes)
        throw std::out_of_range("Pigeonhole variable count overflow");
    SizeType variables=pigeons*holes;
    std::vector<SatClause> clauses=make_problem(pigeons,holes);
    Bool expected_sat=pigeons<=holes;
    std::vector<Trial> bdd,cdcl;
    bdd.reserve(selection.samples);
    cdcl.reserve(selection.samples);
    unsigned rounds=selection.samples+selection.warmups;
    for(unsigned sample=0u;sample<rounds;++sample) {
        // Alternate execution order to reduce warm-cache and drift bias.
        std::array<SatBackendKind,2> order=sample%2u==0u
            ? std::array<SatBackendKind,2>{SatBackendKind::BDD,SatBackendKind::CDCL}
            : std::array<SatBackendKind,2>{SatBackendKind::CDCL,SatBackendKind::BDD};
        for(SatBackendKind kind:order) {
            if((selection.backend==BackendSelection::BDD and kind!=SatBackendKind::BDD)
                or (selection.backend==BackendSelection::CDCL and kind!=SatBackendKind::CDCL))
                continue;
            Trial trial=measure(kind,variables,clauses,expected_sat);
            if(sample>=selection.warmups) {
                if(kind==SatBackendKind::BDD) bdd.push_back(trial);
                else cdcl.push_back(trial);
            }
        }
    }
    if(not bdd.empty()) {
        print_result(holes,pigeons,expected_sat,clauses.size(),
                     SatBackendKind::BDD,summarize(bdd));
    }
    if(not cdcl.empty()) {
        print_result(holes,pigeons,expected_sat,clauses.size(),
                     SatBackendKind::CDCL,summarize(cdcl));
    }
    if(not bdd.empty() and not cdcl.empty()) {
        double bdd_total=summarize(bdd).total_ms;
        double cdcl_total=summarize(cdcl).total_ms;
        if(not std::isfinite(bdd_total) or not std::isfinite(cdcl_total)
           or cdcl_total<=0.0)
            throw std::runtime_error("Invalid comparative benchmark timing");
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
            "sat-pigeonhole-benchmark",benchmark_parsers(selection)));
        if(not cli.acquire(argc,argv))
            return requests_help(argc,argv) ? 0 : 1;
        if(selection.min_holes>selection.max_holes)
            throw std::invalid_argument("min-holes must not exceed max-holes");

        LOGGING_PRINTLN("Pigeonhole SAT benchmark | samples="
            <<selection.samples<<" | warmups="<<selection.warmups)
        LOGGING_PRINTLN("Build: solver initialization + clause insertion; "
            "solve: SAT/UNSAT search; median timings in ms")
        LOGGING_PRINTLN(std::left
            <<std::setw(7)<<"Holes"<<std::setw(9)<<"Pigeons"
            <<std::setw(8)<<"Result"<<std::setw(9)<<"Backend"
            <<std::setw(9)<<"Clauses"
            <<std::right<<std::setw(14)<<"Build (ms)"
            <<std::setw(14)<<"Solve (ms)"
            <<std::setw(14)<<"Total (ms)"
            <<std::setw(13)<<"BDD nodes")
        LOGGING_PRINTLN(std::string(97u,'-'))
        for(SizeType holes=selection.min_holes;holes<=selection.max_holes;++holes) {
            run_case(holes,holes,selection);
            run_case(holes,holes+1u,selection);
        }
        return 0;
    } catch(std::exception const& e) {
        LOGGING_PRINTLN("Pigeonhole benchmark: "<<e.what())
        return 1;
    }
}

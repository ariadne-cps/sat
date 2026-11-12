/***************************************************************************
 *            benchmark_sat_pigeonhole.cpp
 *
 *  Copyright  2026  Luca Geretti
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

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sat_benchmarking_common.hpp"
#include "threading/thread_manager.hpp"
#include "threading/threading_command_line_interface.hpp"

using namespace Ariadne;
namespace Bench = Ariadne::Benchmarking;

namespace {
struct Selection {
    unsigned min_holes = 1u;
    unsigned max_holes = 3u;
    unsigned samples = 5u;
    SatBenchmark::BackendSelection backend = SatBenchmark::BackendSelection::BOTH;
};

List<ArgumentParser> benchmark_parsers(Selection& s) {
    List<ArgumentParser> parsers;

    parsers.append(make_valued_argument_parser(
        "m", "min-holes", "Minimum positive hole count (default: 1)",
        [&s](const String& text) -> VoidFunction {
            const unsigned value = Bench::parse_unsigned<unsigned>(text);
            if (value == 0u)
                throw std::invalid_argument("min-holes must be positive");
            return [&s, value] { s.min_holes = value; };
        }));
    parsers.append(make_valued_argument_parser(
        "M", "max-holes", "Maximum hole count (default: 3)",
        [&s](const String& text) -> VoidFunction {
            const unsigned value = Bench::parse_unsigned<unsigned>(text);
            if (value == 0u || value == std::numeric_limits<unsigned>::max())
                throw std::invalid_argument("max-holes outside supported range");
            return [&s, value] { s.max_holes = value; };
        }));

    parsers.append(make_valued_argument_parser(
        "n", "samples", "Odd sample count between 3 and 101 (default: 5)",
        [&s](const String& text) -> VoidFunction {
            const unsigned value = SatBenchmark::odd_sample_count(text);
            return [&s, value] { s.samples = value; };
        }));
    for (const auto& parser: SatBenchmark::backend_parser(s.backend))
        parsers.append(parser);
    return parsers;
}

SizeType index(SizeType holes, SizeType pigeon, SizeType hole) {
    return pigeon * holes + hole;
}

// Generate once per case. The vector is shared by both backends and is never
// included in the timed region.
std::vector<SatClause> make_problem(SizeType pigeons, SizeType holes) {
    if (holes == 0u or pigeons == 0u or
        holes > std::numeric_limits<SizeType>::max() / pigeons) {
        throw std::out_of_range("pigeonhole variable count overflow");
    }
    std::vector<SatClause> clauses;
    for (SizeType pigeon = 0u; pigeon < pigeons; ++pigeon) {
        SatClause at_least_one;
        for (SizeType hole = 0u; hole < holes; ++hole) {
            at_least_one.emplace_back(index(holes, pigeon, hole));
            for (SizeType other = hole + 1u; other < holes; ++other) {
                clauses.push_back({SatLiteral(index(holes, pigeon, hole), false),
                                   SatLiteral(index(holes, pigeon, other), false)});
            }
        }
        clauses.push_back(std::move(at_least_one));
    }
    for (SizeType hole = 0u; hole < holes; ++hole) {
        for (SizeType p = 0u; p < pigeons; ++p) {
            for (SizeType q = p + 1u; q < pigeons; ++q) {
                clauses.push_back({SatLiteral(index(holes, p, hole), false),
                                   SatLiteral(index(holes, q, hole), false)});
            }
        }
    }
    return clauses;
}

Bool satisfies(const std::vector<SatClause>& clauses, const BinaryWord& model) {
    for (const SatClause& clause: clauses) {
        Bool valid = false;
        for (const SatLiteral& literal: clause) {
            if (model[literal.variable()] == literal.positive()) {
                valid = true;
                break;
            }
        }
        if (not valid)
            return false;
    }
    return true;
}

} // namespace

int main(int argc, const char* argv[]) {
    try {
        Selection selection;
        Bench::CommandLine cli;
        ThreadManager::instance();
        cli.add(threading_command_line_contribution());
        cli.add(CommandLineContribution(
            "sat-pigeonhole-benchmark",
            benchmark_parsers(selection)));
        if (not cli.acquire(Bench::arguments(argc, argv)))
            return 0;

        if (selection.min_holes > selection.max_holes)
            throw std::invalid_argument("min-holes must not exceed max-holes");

        Bench::Runner runner(cli, 2u * (selection.max_holes - selection.min_holes + 1u));
        Bench::Report report("SAT Pigeonhole BDD/CDCL (nanoseconds)", cli.summary_fields());
        report.set_dimension_columns({
            {"Holes", 0u, Bench::TableAlignment::Right},
            {"Pigeons", 0u, Bench::TableAlignment::Right},
            {"Result"},
            {"Clauses", 0u, Bench::TableAlignment::Right},
            {"Backend"},
            {"BDD nodes", 0u, Bench::TableAlignment::Right},
            {"Phase"}
        });
        report.set_table_options(Bench::TableOptions::ruled());
        report.set_parameter_header(
            "CNF generated outside timing; build=solver creation + clause insertion; "
            "solve=SAT/UNSAT search; total=build + solve. Warm-ups excluded.");
        for (SizeType holes = selection.min_holes; holes <= selection.max_holes; ++holes) {
            for (SizeType pigeons: {holes, holes + 1u}) {
                const SizeType variables = pigeons * holes;
                const std::vector<SatClause> clauses = make_problem(pigeons, holes);
                const Bool expected_sat = pigeons <= holes;
                const SatBenchmark::Comparison results = SatBenchmark::run_comparison(
                    runner, "pigeonhole/" + std::to_string(holes) + "/" + std::to_string(pigeons),
                    selection.samples, variables, clauses, expected_sat,
                    selection.backend, [&](const BinaryWord& model) {
                        return satisfies(clauses, model);
                    });
                SatBenchmark::add_rows(report, {std::to_string(holes), std::to_string(pigeons), expected_sat ? "SAT" : "UNSAT", std::to_string(clauses.size())}, results);
                if (auto ratio = SatBenchmark::bdd_cdcl_total_ratio(results))
                    LOGGING_PRINTLN("Pigeonhole holes=" << holes << " pigeons=" << pigeons
                                                        << " BDD/CDCL median total ratio: "
                                                        << Bench::Table::number(*ratio) << "x")
            }
        }
        report.print();

        return 0;
    } catch (const std::exception& error) {
        LOGGING_PRINTLN("Pigeonhole benchmark: " << error.what())
        return 1;
    }
}

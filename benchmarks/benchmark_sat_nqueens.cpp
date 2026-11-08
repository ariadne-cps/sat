/***************************************************************************
 *            benchmark_sat_nqueens.cpp
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

/*! \file benchmark_sat_nqueens.cpp
 *  \brief Comparative N-queens SAT benchmark for BDD and CDCL.
 */

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sat_benchmarking_common.hpp"

using namespace Ariadne;
namespace Bench=Ariadne::Benchmarking;

namespace {
struct Selection {
    unsigned n=4u;
    unsigned samples=5u;
    SatBenchmark::BackendSelection backend=SatBenchmark::BackendSelection::BOTH;
};

List<ArgumentParser> benchmark_parsers(Selection& s) {
    List<ArgumentParser> parsers;

    parsers.append(make_valued_argument_parser(
        "N","n","Chessboard size (positive, default: 4)",
        [&s](String const& text)->VoidFunction {
            unsigned const value=Bench::parse_unsigned<unsigned>(text);
            if(value==0u ||
               static_cast<SizeType>(value)>
                   static_cast<SizeType>(std::numeric_limits<Int>::max())/
                   static_cast<SizeType>(value))
                throw std::invalid_argument("n is zero or exceeds Boolean variable range");
            return [&s,value] { s.n=value; };
        }));

    parsers.append(make_valued_argument_parser(
        "n","samples","Odd sample count between 3 and 101 (default: 5)",
        [&s](String const& text)->VoidFunction {
            unsigned const value=SatBenchmark::odd_sample_count(text);
            return [&s,value] { s.samples=value; };
        }));
    for(auto const& parser:SatBenchmark::backend_parser(s.backend))
        parsers.append(parser);
    return parsers;
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


} // namespace

int main(int argc,char const* argv[]) {
    try {
        Selection selection;
        Bench::CommandLine cli;
        cli.add(CommandLineContribution(
            "sat-nqueens-benchmark",
            benchmark_parsers(selection)));
        if(not cli.acquire(Bench::arguments(argc,argv))) return 0;

        Bench::Runner runner(cli,1u);
        Bench::Report report("SAT N-Queens BDD/CDCL (nanoseconds)",cli.summary_fields());
        report.set_dimension_columns({
            {"N",0u,Bench::TableAlignment::Right},
            {"Result"},{"Variables",0u,Bench::TableAlignment::Right},
            {"Clauses",0u,Bench::TableAlignment::Right},
            {"Backend"},{"BDD nodes",0u,Bench::TableAlignment::Right},
            {"Phase"}
        });
        report.set_table_options(Bench::TableOptions::ruled());
        report.set_parameter_header(
            "CNF generated outside timing; build=solver creation + clause insertion; "
            "solve=SAT/UNSAT search; total=build + solve. Warm-ups excluded.");
        SizeType const n=selection.n;
        std::vector<SatClause> const clauses=make_problem(n);
        Bool const expected_sat=n==1u || n>=4u;
        SatBenchmark::Comparison const results=SatBenchmark::run_comparison(
            runner,"nqueens",selection.samples,n*n,clauses,expected_sat,
            selection.backend,[&](BinaryWord const& model) {
                return satisfies(clauses,model) && valid_queens(n,model);
            });
        SatBenchmark::add_rows(report,{
            std::to_string(n),expected_sat?"SAT":"UNSAT",
            std::to_string(n*n),std::to_string(clauses.size())},results);
        report.print();
        if(auto ratio=SatBenchmark::bdd_cdcl_total_ratio(results))
            LOGGING_PRINTLN("BDD/CDCL median total time (ratio >1 favors CDCL): "
                           <<Bench::Table::number(*ratio)<<"x")

        return 0;
    } catch(std::exception const& error) {
        LOGGING_PRINTLN("N-queens benchmark: "<<error.what())
        return 1;
    }
}

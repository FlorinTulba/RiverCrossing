/*
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
*/

$("document").ready(function() {
    if(solution.length > 0 && solution != '{{.}}') {
        sessionStorage.foundSolution = 1;
  
        if(solution.match(/^('")/) != null && solution.match(/('")$/) != null)
          solution = solution.slice(1, solution.length-1) // remove wrapping ''
        solution = solution
            .replace(/\t/g, '&nbsp;&nbsp;&nbsp;&nbsp;') // replace tabs with 4 spaces
            .replace(/(\n)/g, '<br>') // replace \n by <br>
        $("#theSolution").html(solution)
    }
});

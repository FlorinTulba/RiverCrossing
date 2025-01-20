/*
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
*/

/*
  The description has a few introductory phrases,
  then a part describing the constraints
  as a list.
*/
function extractPuzzleDescr(descr) {
  var lines = descr.length;
  var out = '', foundList = false;
  for(var i=0; i<lines; ++i) {
    var line = descr[i];
    if(line == "Constraints:") {
      foundList = true;
      out += "<p>" + line + "<ul>";
      continue;
    }

    if(foundList) {
      out += "<li>" + line.replace(/^-\s*/, '') + "</li>";
      continue;
    }

    out += line;
  }
  if(foundList)
    out += "</ul><p>";

  return out;
}
